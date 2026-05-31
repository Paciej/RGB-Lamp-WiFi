#ifndef LED_MQTT_SERVICE_H_ 
#define LED_MQTT_SERVICE_H_ 

#include <WiFi.h>
#include <PubSubClient.h>

class LedMqttService {
private:
    // CALLBACKS NAMES
    const char* color = "light/cmd/color";
    const char* mode = "light/cmd/mode";

    const char* mqtt_server = "mqtt broker ip";

    WiFiClient espClient;
    PubSubClient client;
    
    void callback(char* topic, byte* payload, unsigned int length);
    void reconnect();
public:
    LedMqttService() {}

    void init(const char* ssid, const char* password);
    void update();
};

#endif