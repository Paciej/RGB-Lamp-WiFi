#include "LedMqttService.h"
#include "Secrets.h"
#include <esp_mac.h>

// ********* Constructors **********/
// ---------------------------------
LedMqttService::LedMqttService() {
    this->mqttClient = new PubSubClient(this->wifiClient);
}

// ********* Private Functions ***********/
// ----------------------------------------
const char* LedMqttService::commandOf(const char* topic) const {
    return strncmp(topic, pf_cmd, cmdLen) == 0 ? topic + cmdLen : nullptr;
}

void LedMqttService::sendInfo() {
    char topic[48];
    int n = snprintf(topic, sizeof(topic), "candelum/%s/info", device_id);
    if (n < 0 || n >= (int)sizeof(topic)) return;

    bool ok = mqttClient->publish(topic, infoJson, true);
    Serial.printf("Info publish %s (%d B)\n", ok ? "Ok" : "Failed", (unsigned)strlen(infoJson));
}


void LedMqttService::startWifi() {
    Serial.printf("Connecting to WiFi: %s\n", cfg.ssid);
    WiFi.begin(cfg.ssid, cfg.wifiPass);
    state = NetState::WifiConnecting;
    stateSince = millis();
}

void LedMqttService::enterMqttConnecting(uint32_t now) {
    state = NetState::MqttConnecting;
    backoff = MQTT_BACKOFF_MIN;
    nextAttempt = now;
}

void LedMqttService::tryMqttConnect() {
    Serial.println("Attempting MQTT connection...");
    const char* user = cfg.user[0] ? cfg.user : nullptr;
    const char* pass = cfg.user[0] ? cfg.mqttPass : nullptr;

    if (mqttClient->connect(device_id, user, pass, willTopic, 1, true, "offline")) {
        mqttClient->subscribe(pf_cmd, 1);
        mqttClient->subscribe(pf_cfg, 1);
        if (pf_room[0]) mqttClient->subscribe(pf_room, 1);
        sendInfo();
        sendData("online", topicConnection, true);
        state = NetState::Online;
        backoff = MQTT_BACKOFF_MIN;
        Serial.println("Successfully connected to broker");
    } else {
        Serial.printf("MQTT failed, rc=%d, retry in %lu ms\n", mqttClient->state(), (unsigned long)backoff);
        nextAttempt = millis() + backoff;
        backoff = min(backoff * 2, MQTT_BACKOFF_MAX);
    }
}

void LedMqttService::callback(char* topic, byte* payload, unsigned int length) {
    if (length > MAX_SIZE) {
        Serial.printf("Payload too long: %u\n", length);
        return;
    }

    char buf[MAX_SIZE + 1];
    memcpy(buf, payload, length);
    buf[length] = '\0';
    Serial.printf("Received [%s]: %s\n", topic, buf);

    if (strcmp(topic, pf_cfg) == 0) {
        size_t n = length;
        while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == ' ')) {
            buf[--n] = '\0';
        }
        if (roomCallback) roomCallback(buf);     
        return;
    }

    bool fromGroup = false;
    const char* name = commandOf(topic);

    if (!name && roomLen > 0 && strncmp(topic, pf_room, roomLen) == 0) {
        name = topic + roomLen;
        fromGroup = true;
    }
    if (!name) {
        Serial.println("This topic has wrong prefix - light won't process it");
        return;
    }
    if (fromGroup && strcmp(name, topicMode) != 0) return;

    if (strcmp(name, topicColor) == 0) {
        char* p = buf;
        char* end;
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
        if (brightnessCallback) brightnessCallback(constrain(atoi(buf), 0, 255));
    }
    else if (strcmp(name, topicMode) == 0) {
        ModeCommand cmd;
        if (parseModeCommand(buf, cmd) && modeCallback) {
            modeCallback(cmd);
        }
    }
    else if (strcmp(name, topicPower) == 0) {
        if (!powerCallback) return;
        if (strcmp(buf, "ON") == 0) powerCallback(true);
        else if (strcmp(buf, "OFF") == 0) powerCallback(false);
    }
}

//********* Public Functions ***********/
// ----------------------------------------
void LedMqttService::begin(const NetConfig& config) {
    this->cfg = config;

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(device_id, sizeof(device_id), "%02x%02x%02x%02x%02x%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    cmdLen = snprintf(pf_cmd, sizeof(pf_cmd), "candelum/%s/cmd/#", device_id) - 1;
    statusLen = snprintf(pf_status, sizeof(pf_status), "candelum/%s/status/", device_id);
    cfgLen = snprintf(pf_cfg, sizeof(pf_cfg), "candelum/%s/config/room", device_id);
    snprintf(willTopic, sizeof(willTopic), "%s%s", pf_status, topicConnection);

    mqttClient->setServer(cfg.host, cfg.port);
    mqttClient->setBufferSize(512);
    mqttClient->setKeepAlive(30);
    mqttClient->setSocketTimeout(2);
    mqttClient->setCallback([this](char* topic, byte* payload, unsigned int length) {
        callback(topic, payload, length);
    });

    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);

    offlineSince = millis();
    startWifi();
}

void LedMqttService::update() {
   uint32_t now = millis();

    switch (state) {
    case NetState::Idle:
        return;

    case NetState::WifiConnecting:
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("WiFi connected, IP: %s, RSSI: %d\n",
                          WiFi.localIP().toString().c_str(), WiFi.RSSI());
            enterMqttConnecting(now);
        } else if (now - stateSince >= WIFI_ATTEMPT_TIMEOUT) {
            Serial.println("WiFi attempt timed out, retrying");
            WiFi.disconnect();
            startWifi();
        }
        break;

    case NetState::MqttConnecting:
        if (WiFi.status() != WL_CONNECTED) {
            state = NetState::WifiConnecting;
            stateSince = now;
            break;
        }
        if ((int32_t)(now - nextAttempt) >= 0) tryMqttConnect();
        break;

    case NetState::Online:
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi lost");
            mqttClient->disconnect();
            offlineSince = now;
            state = NetState::WifiConnecting;
            stateSince = now;
        } else if (!mqttClient->connected()) {
            Serial.printf("MQTT lost, rc=%d\n", mqttClient->state());
            offlineSince = now;
            enterMqttConnecting(now);
        } else {
            mqttClient->loop();
        }
        break;
    }
}

void LedMqttService::sendData(const char* data, const char* topic, bool retained) {
    if (state != NetState::Online) return;
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
    return state == NetState::Online && mqttClient->connected();
}

uint32_t LedMqttService::offlineMs() const {
    return state == NetState::Online ? 0 : millis() - offlineSince;
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