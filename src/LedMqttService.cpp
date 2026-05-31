#include "LedMqttService.h"

//********* Private Functions ***********/
void LedMqttService::reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
    } 

    delay(5000);
}

//********* Public Functions ***********/
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
}

void LedMqttService::callback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length, i++) {
        message += (char)payload[i];
    }

    if (strcmp(message.c_str(), this->color) == 0) {

    }

    if (strcmp(message.c_str(), this->mode) == 0) {

    }
}

void LedMqttService::update() {
    if (!client.connected()) {
        // reconnect
    }

    client.loop();
}