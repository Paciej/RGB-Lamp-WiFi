#include "LedMqttService.h"
#include "Secrets.h"

// ********* Private Functions ***********/
// ----------------------------------------
void LedMqttService::reconnect() {
    while (!client->connected()) {
        Serial.println("Attempting MQTT connection...");
        if (client->connect(device_id, MQTT_USERNAME, MQTT_PASSWORD, topicConnection, 1, true, "Offline")) {
            client->subscribe(topicPower);
            client->subscribe(topicColor);
            client->subscribe(topicMode);
            sendData("Online", topicConnection, true);
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client->state()); 
            Serial.println("");
            delay(5000);
        }
    } 
}

bool LedMqttService::resolveMqttBroker() {
    IPAddress brokerIp;

    if (!WiFi.hostByName(MQTT_BROKER_HOST, brokerIp)) {
        Serial.println("Could not resolve MQTT broker address");
        return false;
    }

    Serial.printf("MQTT broker IP: %s", brokerIp);
    client->setServer(brokerIp, MQTT_BROKER_PORT);
    return true;
}

//********* Public Functions ***********/
// ----------------------------------------
void LedMqttService::init(const char* ssid, const char* password) {
    Serial.begin(115120);

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

    if (strcmp(topic, topicMode) == 0) {
        if (message == "Wave" || message == "Pulse" || message == "Candle" || message == "None") {
            if (modeCallback) {
                modeCallback(message);
            }
        }
    }

    if (strcmp(topic, topicPower) == 0) {
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

void LedMqttService::onModeChange(ModeChangeCallback callback) {
    this->modeCallback = callback;
}