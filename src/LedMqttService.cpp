#include "LedMqttService.h"

// ********* Private Functions ***********/
// ----------------------------------------
void LedMqttService::reconnect() {
    while (!client->connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client->connect(device_id)) {
            Serial.println("Connected");
            client->subscribe(topicColor);
            client->subscribe(topicMode);
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client->state());
            delay(5000);
        }
    } 
}

//********* Public Functions ***********/
// ----------------------------------------
void LedMqttService::init(const char* ssid, const char* password) {
    Serial.begin(115120);

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Setup MQTT
    client->setServer(mqtt_server, 1883);
    client->setCallback([this](char* topic, byte* payload, unsigned int length){callback(topic, payload, length);});
}

void LedMqttService::callback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    if (strcmp(message.c_str(), topicColor) == 0) {
        int firstComma = message.indexOf(',');
        int secondComma = message.indexOf(',', firstComma + 1);
        
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

    if (strcmp(message.c_str(), topicMode) == 0) {
        if (message == "Wave" || message == "Pulse" || message == "Candle") {
            if (modeCallback) {
                modeCallback(message);
            }
        }
    }
}

void LedMqttService::update() {
    if (!client->connected()) {
        reconnect();
    }

    client->loop();
}

void LedMqttService::sendData(String data) {
    client->publish(topicMessage, data.c_str());
    Serial.printf("Sent message: %s\n", data);
}

void LedMqttService::onColorChange(ColorChangeCallback callback) {
    this->colorCallback = callback;
}

void LedMqttService::onModeChange(ModeChangeCallback callback) {
    this->modeCallback = callback;
}