#ifndef LED_MQTT_SERVICE_H
#define LED_MQTT_SERVICE_H

#include <WiFi.h>
#include <PubSubClient.h>

typedef void (*ColorChangeCallback)(int, int, int);
typedef void (*ModeChangeCallback)(String);

class LedMqttService {
private:
    ColorChangeCallback colorCallback = nullptr;
    ModeChangeCallback modeCallback = nullptr;

    const char* device_id = "ESP32Client";
    // MQTT clients
    WiFiClient espClient;
    PubSubClient* client;
    
    void callback(char* topic, byte* payload, unsigned int length);
    void reconnect();
public:
    // TOPICS
    // ---------------------------------------------
    const char* topicColor =        "lamp/cmd/color";
    const char* topicMode =         "lamp/cmd/mode";
    const char* topicMessage =      "lamp/message";
    const char* topicConnection =   "lamp/status/connection";
    const char* topicTemp =         "lamp/status/temperature";

    LedMqttService() {
        client = new PubSubClient(espClient);
    }

    void init(const char* ssid, const char* password);
    void update();
    void sendData(String data, const char* topic);
    bool isConnected();
    void onColorChange(ColorChangeCallback callback);
    void onModeChange(ModeChangeCallback callback);
};

#endif