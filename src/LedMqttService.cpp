#include "LedMqttService.h"
#include "Secrets.h"
#include <ESPmDNS.h>

// ********* Private Functions ***********/
// ----------------------------------------

String LedMqttService::readDeviceId() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char buf[13];
  snprintf(buf, sizeof(buf), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.printf("esp32 MAC address: %s", String(buf));
  return String(buf);
}

void LedMqttService::reconnect() {
    while (!client->connected()) {
        Serial.println("Attempting MQTT connection...");
        if (client->connect(device_id, MQTT_USERNAME, MQTT_PASSWORD, topicConnection, 1, true, "Offline")) {
            client->subscribe(topicPower);
            client->subscribe(topicColor);
            client->subscribe(topicBrightness);
            client->subscribe(topicMode);
            sendData("Online", topicConnection, true);
            Serial.println("Successfully connected to broker");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client->state()); 
            Serial.println("");
            delay(5000);
        }
    } 
}

bool LedMqttService::resolveMqttBroker() {

    IPAddress brokerIp = IPAddress(0, 0, 0, 0);

    if (!MDNS.begin(this->device_id)) {
        Serial.println("MDNS ERROR: Couldn't initialize mDNS");
        Serial.printf("MQTT broker IP: %s\n", MQTT_BROKER);
        client->setServer(MQTT_BROKER, MQTT_BROKER_PORT);
        return false;
    } 

    uint8_t tries = 0;

    while (++tries < 3) {
        brokerIp = MDNS.queryHost(MQTT_BROKER_HOST, 3000);
        if (brokerIp != IPAddress(0, 0, 0, 0)) { break; }
    }

    (brokerIp != IPAddress(0, 0, 0, 0)) ? client->setServer(brokerIp, MQTT_BROKER_PORT) : client->setServer(MQTT_BROKER, MQTT_BROKER_PORT);
    Serial.printf("MQTT broker IP: %s", brokerIp);
    return true;
}

//********* Public Functions ***********/
// ----------------------------------------
void LedMqttService::init(const char* ssid, const char* password) {
    Serial.begin(115120);

    readDeviceId();

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    WiFi.setAutoReconnect(true);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Setup MQTT
    resolveMqttBroker();    // find broker IP and set server
    client->setKeepAlive(60);
    client->setCallback([this](char* topic, byte* payload, unsigned int length){callback(topic, payload, length);});
    reconnect();
}

void LedMqttService::callback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
        Serial.print((char)payload[i]);
    }
    Serial.println();

    if (strcmp(topic, topicColor) == 0) {
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
    else if (strcmp(topic, topicBrightness) == 0) {
        brightnessCallback(constrain(message.toInt(), 0, 255));
    }
    else if (strcmp(topic, topicMode) == 0) {
        if (message == "Wave" || message == "Pulse" || message == "Candle" || message == "None") {
            if (modeCallback) {
                modeCallback(message);
            }
        }
    }
    else if (strcmp(topic, topicPower) == 0) {
        if (message == "ON" || message == "1") {
            powerCallback(true);
        } else if (message == "OFF" || message == "0") {
            powerCallback(false);
        }
    }
}

void LedMqttService::update() {
    if (!client->connected()) {
        resolveMqttBroker();
        reconnect();
    }

    client->loop();
}

void LedMqttService::sendData(String data, const char* topic, bool retained) {
    client->publish(topic, data.c_str(), (boolean)retained);
}

bool LedMqttService::isConnected() {
    return client->connected();
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