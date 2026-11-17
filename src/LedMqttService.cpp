#include "LedMqttService.h"
#include "Secrets.h"
#include <esp_mac.h>
#include <ESPmDNS.h>

// ********* Constructors **********/
// ---------------------------------
LedMqttService::LedMqttService() {
    this->mqttClient = new PubSubClient(this->wifiClient);
}

// ********* Private Functions ***********/
// ----------------------------------------

String LedMqttService::readDeviceId() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(device_id, sizeof(device_id), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.printf("esp32 MAC address: %s", device_id);
  return String(device_id);
}

const char* LedMqttService::commandOf(const char* topic) const {
    return strncmp(topic, pf_cmd, cmdLen) == 0 ? topic + cmdLen : nullptr;
}

void LedMqttService::sendInfo() {
    char topic[48];
    int n = snprintf(topic, sizeof(topic), "candelum/%s/info", device_id);
    if (n < 0 || n >= (int)sizeof(topic)) return;

    char payload[200];
    n = snprintf(payload, sizeof(payload), infoJson);

    bool ok = mqttClient->publish(topic, payload, true);
    Serial.printf("Info publish %s (%d B)\n", ok ? "Ok" : "Failed", n);
}

void LedMqttService::reconnect() {
    char filter[MAX_SIZE + 2];
    snprintf(filter, sizeof(filter), "%s#", this->pf_cmd);
    char tConnect[statusLen + strlen(topicConnection) + 1];
    snprintf(tConnect, sizeof(tConnect), "%s%s", pf_status, topicConnection);
    Serial.printf("topics to subscribe: %s\n topic connection: %s\n", filter, tConnect);

    while (!mqttClient->connected()) {
        Serial.println("Attempting MQTT connection...");
        if (mqttClient->connect(device_id, MQTT_USERNAME, MQTT_PASSWORD, tConnect, 1, true, "Offline")) {
            mqttClient->subscribe(filter, 1);
            sendInfo();
            sendData("Online", topicConnection, true);
            Serial.println("Successfully connected to broker");
        } else {
            Serial.printf("Failed, rc=%d\n", mqttClient->state());
            delay(5000);
        }
    } 
}

bool LedMqttService::resolveMqttBroker() {

    IPAddress brokerIp = IPAddress(0, 0, 0, 0);

    if (!MDNS.begin(this->device_id)) {
        Serial.println("MDNS ERROR: Couldn't initialize mDNS");
        Serial.printf("MQTT broker IP: %s\n", MQTT_BROKER);
        mqttClient->setServer(MQTT_BROKER, MQTT_BROKER_PORT);
        return false;
    } 

    uint8_t tries = 0;

    while (++tries < 3) {
        brokerIp = MDNS.queryHost(MQTT_BROKER_HOST, 3000);
        if (brokerIp != IPAddress(0, 0, 0, 0)) { break; }
    }

    (brokerIp != IPAddress(0, 0, 0, 0)) ? mqttClient->setServer(brokerIp, MQTT_BROKER_PORT) : mqttClient->setServer(MQTT_BROKER, MQTT_BROKER_PORT);
    Serial.printf("MQTT broker IP: %s", brokerIp);
    return true;
}

//********* Public Functions ***********/
// ----------------------------------------
void LedMqttService::init(const char* ssid, const char* password) {
    Serial.begin(115120);

    // Read mac and create prefixes
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(device_id, sizeof(device_id), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    cmdLen = snprintf(this->pf_cmd, 48, "candelum/%s/cmd/", device_id);
    statusLen = snprintf(this->pf_status, 48, "candelum/%s/status/", device_id);

    Serial.printf("\nConnecting to %s", ssid);
    WiFi.begin(ssid, password);
    WiFi.setAutoReconnect(true);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.printf("\nWiFi Connected\n IP address: %s\n", (String)WiFi.localIP());

    // Setup MQTT
    //resolveMqttBroker();    // find broker IP and set server
    mqttClient->setServer(MQTT_BROKER, MQTT_BROKER_PORT);
    mqttClient->setBufferSize(512);
    mqttClient->setKeepAlive(60);
    mqttClient->setCallback([this](char* topic, byte* payload, unsigned int length){callback(topic, payload, length);});
    reconnect();
}

void LedMqttService::callback(char* topic, byte* payload, unsigned int length) {
    const char* name = commandOf(topic);
    if (!name) {
        Serial.print("\nThis topic has wrong prefix - light won't process it");
        return;
    }

    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
        Serial.print((char)payload[i]);
    }
    Serial.println();

    if (strcmp(name, topicColor) == 0) {
        char comma = (message.indexOf(',') == -1) ? '.' : ',';
        int firstComma = message.indexOf(comma);
        int secondComma = message.indexOf(comma, firstComma + 1);
        
        if (firstComma > 0 && secondComma > firstComma) {
            String redStr = message.substring(0, firstComma);
            String greenStr = message.substring(firstComma + 1, secondComma);
            String blueStr = message.substring(secondComma + 1);
            
            int r = constrain(redStr.toInt(), 0, 255);
            int g = constrain(greenStr.toInt(), 0, 255);
            int b = constrain(blueStr.toInt(), 0, 255);

            if (colorCallback) {
                colorCallback(r, g, b);
            }
        }
    }
    else if (strcmp(name, topicBrightness) == 0) {
        brightnessCallback(constrain(message.toInt(), 0, 255));
    }
    else if (strcmp(name, topicMode) == 0) {
        if (message == "Wave" || message == "Pulse" || message == "Candle" || message == "None") {
            if (modeCallback) {
                modeCallback(message);
            }
        }
    }
    else if (strcmp(name, topicPower) == 0) {
        if (message == "ON" || message == "1") {
            powerCallback(true);
        } else if (message == "OFF" || message == "0") {
            powerCallback(false);
        }
    }
}

void LedMqttService::update() {
    if (!mqttClient->connected()) {
        resolveMqttBroker();
        reconnect();
    }

    mqttClient->loop();
}

void LedMqttService::sendData(String data, const char* topic, bool retained) {
    char t[MAX_SIZE];
    snprintf(t, sizeof(t), "%s%s", pf_status, topic);
    Serial.printf("send topic: %s\t data: %s\t retain: %d\n", t, data, retained);
    mqttClient->publish(t, data.c_str(), retained);
}

bool LedMqttService::isConnected() {
    return mqttClient->connected();
}

void LedMqttService::onPowerChange(PowerChangeCallback callback) {
    this->powerCallback = callback;
}

void LedMqttService::onColorChange(ColorChangeCallback callback) {
    this->colorCallback = callback;
}

void LedMqttService::onBrightnessChange(BrightnessChangeCallback callback) {
    this->brightnessCallback = callback;
}

void LedMqttService::onModeChange(ModeChangeCallback callback) {
    this->modeCallback = callback;
}