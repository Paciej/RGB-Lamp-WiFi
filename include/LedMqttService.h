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

    // TOPICS
    // ---------------------------------------------
    // Subscribe
    const char* topicColor = "lamp/cmd/color";
    const char* topicMode =  "lamp/cmd/mode";

    const char* device_id = "ESP32Client";
    // MQTT clients
    WiFiClient espClient;
    PubSubClient* client;
    
    void callback(char* topic, byte* payload, unsigned int length);
    void reconnect();
public:
    // TOPICS
    // ---------------------------------------------
    // Publish
    const char* topicMessage =      "lamp/message";
    const char* topicConnection =   "lamp/status/connection";
    const char* topicTemp =         "lamp/status/temperature";
    const char* topicTempMessage =  "lamp/status/temperature/message";
    const char* topicColorStatus =  "lamp/status/color";
    const char* topicColorMessage = "lamp/status/color/message";
    const char* topicModeStatus =   "lamp/status/mode";
    const char* topicModeMessage =  "lamp/status/mode/message";

    LedMqttService() {
        client = new PubSubClient(espClient);
    }

    void init(const char* ssid, const char* password);
    void update();
    void sendData(String data, const char* topic, bool retained = false);
    bool isConnected();
    void onColorChange(ColorChangeCallback callback);
    void onModeChange(ModeChangeCallback callback);
};

#endif