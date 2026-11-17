#include "LedMqttService.h"
#include "Secrets.h"

// ********* Private Functions ***********/
// ----------------------------------------
void LedMqttService::reconnect() {
    while (!client->connected()) {
        Serial.println("Attempting MQTT connection...");
        if (client->connect(device_id)) {
            client->subscribe(topicColor);
            client->subscribe(topicMode);
            client->subscribe(topicMessage);
            sendData("Connected", topicConnection);
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
    client->setServer("192.168.202.78", 1883);
    client->setKeepAlive(90);
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

void LedMqttService::sendData(String data, const char* topic, bool retained) {
    client->publish(topic, data.c_str(), (boolean)retained);
}

bool LedMqttService::isConnected() {
    return client->connected();
}

void LedMqttService::onColorChange(ColorChangeCallback callback) {
    this->colorCallback = callback;
}

void LedMqttService::onModeChange(ModeChangeCallback callback) {
    this->modeCallback = callback;
}