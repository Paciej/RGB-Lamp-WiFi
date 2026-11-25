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
    char tConnect[statusLen + strlen(topicConnection) + 1];
    snprintf(tConnect, sizeof(tConnect), "%s%s", pf_status, topicConnection);

    while (!mqttClient->connected()) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("ESP32 is not connected to WiFi. Reconnecting...");
            WiFi.reconnect();
        } else {
            Serial.printf("Wifi signal: %d\n", WiFi.RSSI());
        }
        Serial.println("Attempting MQTT connection...");
        if (mqttClient->connect(device_id, MQTT_USERNAME, MQTT_PASSWORD, tConnect, 1, true, "offline")) {
            mqttClient->subscribe(this->pf_cmd, 1);
            mqttClient->subscribe(this->pf_cfg, 1);

            if (pf_room[0]) {
                mqttClient->subscribe(this->pf_room, 1);
            }
            sendInfo();
            sendData("online", topicConnection, true);
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
        return false;
    } 

    uint8_t tries = 0;

    while (++tries < 3) {
        brokerIp = MDNS.queryHost(MQTT_BROKER_HOST, 3000);
        if (brokerIp != IPAddress(0, 0, 0, 0)) { break; }
    }

    if (brokerIp == IPAddress(0, 0, 0, 0)) {
        Serial.println("Couldn't find address via MDNS. Setting hardocded IP address");
        if (!brokerIp.fromString(MQTT_BROKER)) {
            Serial.println("Invalid given IP Address");
            return false;
        }
    }
    this->mqttAddress = brokerIp;
    Serial.printf("MQTT broker IP: %s\n", mqttAddress.toString().c_str());
    return true;
}

//********* Public Functions ***********/
// ----------------------------------------
void LedMqttService::init(const char* ssid, const char* password) {
    Serial.begin(115200);

    // Read mac and create prefixes
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(device_id, sizeof(device_id), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    cmdLen = snprintf(this->pf_cmd, 48, "candelum/%s/cmd/#", device_id) - 1; 
    statusLen = snprintf(this->pf_status, 48, "candelum/%s/status/", device_id);
    cfgLen = snprintf(this->pf_cfg, 48, "candelum/%s/config/room", device_id);

    Serial.printf("\nConnecting to %s", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setSleep(false);
    WiFi.begin(ssid, password);
    WiFi.setAutoReconnect(true);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.printf("\nWiFi Connected\n IP address: %s\n", (String)WiFi.localIP().toString().c_str());

    // Setup MQTT
    //resolveMqttBroker();    // find broker IP and set server
    this->mqttAddress.fromString(MQTT_BROKER);
    Serial.printf("Broker ip address: %s\n", this->mqttAddress.toString().c_str());
    mqttClient->setServer(this->mqttAddress, MQTT_BROKER_PORT);
    mqttClient->setBufferSize(512);
    mqttClient->setKeepAlive(20);
    mqttClient->setCallback([this](char* topic, byte* payload, unsigned int length){callback(topic, payload, length);});
    reconnect();
}

void LedMqttService::callback(char* topic, byte* payload, unsigned int length) {
    if (length > MAX_SIZE) {
        Serial.printf("Payload too long: %u\n", length);
    }

    const char* name = commandOf(topic);
    if (!name) {
        Serial.print("\nThis topic has wrong prefix - light won't process it");
        return;
    }

    char buf[MAX_SIZE + 1];
    memcpy(buf, payload, length);
    buf[length] = '\0';
    Serial.printf("Received: %s\n", buf);

    if (strcmp(name, topicColor) == 0) {
        char* p = buf;
        char *end;
        long v[3];
        bool ok = true;

        for (int i = 0; i < 3 && ok; i++) {
            v[i] = strtol(p, &end, 10);
            if (end == p) { ok = false; break; }
            if (i < 2) {
                if (*end != ',') { ok = false; break; } 
                p = end + 1;
            }
        }

        if (ok && colorCallback) {
            colorCallback(
                (uint8_t)constrain(v[0], 0, 255),
                (uint8_t)constrain(v[1], 0, 255),
                (uint8_t)constrain(v[2], 0, 255)
            );
        }

    }
    else if (strcmp(name, topicBrightness) == 0) {
        brightnessCallback(constrain(atoi(buf), 0, 255));
    }
    else if (strcmp(name, topicMode) == 0) {
        ModeCommand cmd;
        if (parseModeCommand(buf, cmd) && modeCallback) {
            modeCallback(cmd);
        }
    }
    else if (strcmp(name, topicPower) == 0) {
        if (strcmp(buf, "ON") == 0) {
            powerCallback(true);
        } else if (strcmp(buf, "OFF") == 0) {
            powerCallback(false);
        }
    }
}

void LedMqttService::update() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.print("ESP32 is disconnected from wifi. Reconnecting...\n");
        WiFi.reconnect();
    }
    if (!mqttClient->connected()) {
        resolveMqttBroker();
        reconnect();
    }

    mqttClient->loop();
}

void LedMqttService::sendData(const char* data, const char* topic, bool retained) {
    char t[MAX_SIZE];
    snprintf(t, sizeof(t), "%s%s", pf_status, topic);
    mqttClient->publish(t, data, retained);
}

void LedMqttService::setRoom(const char* roomId) {
    char newRoom[sizeof(pf_room)] = {0};
    if (roomId && roomId[0]) {
        int n = snprintf(newRoom, sizeof(newRoom), "candelum/group/%s/cmd/#", roomId);
        if (n < 0 || n >= (int)sizeof(newRoom)) newRoom[0] = 0;
    }
    if (strcmp(newRoom, pf_room) == 0) return;

    if (mqttClient->connected()) {
        if (pf_room[0]) {
            mqttClient->unsubscribe(pf_room);
        }
        if (newRoom[0]) {
            mqttClient->subscribe(newRoom, 1);
        }
    }
    strlcpy(pf_room, newRoom, sizeof(pf_room));
    roomLen = pf_room[0] ? strlen(pf_room) - 1 : 0;
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

void LedMqttService::onRoomChange(RoomChangeCallback callback) {
    this->roomCallback = callback;
}