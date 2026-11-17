#ifndef LED_MQTT_SERVICE_H_ 
#define LED_MQTT_SERVICE_H_ 

#include <WiFi.h>
#include <PubSubClient.h>

typedef void (*ColorChangeCallback)(int, int, int);
typedef void (*ModeChangeCallback)(String);

class LedMqttService {
private:
    ColorChangeCallback colorCallback = nullptr;
    ModeChangeCallback modeCallback = nullptr;

    // CALLBACKS NAMES
    const char* topicColor = "lamp/cmd/color";
    const char* topicMode = "lamp/cmd/mode";
    const char* topicMessage = "lamp/message";

    const char* device_id = "ESP32Client";
    const char* mqtt_server = "mqtt broker ip";

    WiFiClient espClient;
    PubSubClient* client;
    
    void callback(char* topic, byte* payload, unsigned int length);
    void reconnect();
public:
    LedMqttService() {
       client = new PubSubClient(espClient);
    }

    void init(const char* ssid, const char* password);
    void update();
    void sendData(String data);
    void onColorChange(ColorChangeCallback callback);
    void onModeChange(ModeChangeCallback callback);
};

#endif