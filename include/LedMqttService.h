#ifndef LED_MQTT_SERVICE_H
#define LED_MQTT_SERVICE_H

#include <WiFi.h>
#include <PubSubClient.h>

typedef void (*PowerChangeCallback)(bool);
typedef void (*ColorChangeCallback)(int, int, int);
typedef void (*BrightnessChangeCallback)(int);
typedef void (*ModeChangeCallback)(String);

static constexpr size_t MAX_SIZE = 48;

class LedMqttService {
private:
    PowerChangeCallback powerCallback = nullptr;
    ColorChangeCallback colorCallback = nullptr;
    BrightnessChangeCallback brightnessCallback = nullptr;
    ModeChangeCallback modeCallback = nullptr;
    
    // MQTT clients
    WiFiClient wifiClient;
    PubSubClient* mqttClient;

    char device_id[13];
    char pf_cmd[MAX_SIZE];
    char pf_status[MAX_SIZE];
    size_t cmdLen = 0;
    size_t statusLen = 0;

    // TOPICS
    // ---------------------------------------------
    // Subscribe
    const char* topicPower = "power";
    const char* topicColor = "color";
    const char* topicBrightness = "brightness";
    const char* topicMode =  "mode";

    void callback(char* topic, byte* payload, unsigned int length);
    const char* commandOf(const char* topic) const;
    String readDeviceId();
    void reconnect();
    bool resolveMqttBroker();
public:
    // TOPICS
    // ---------------------------------------------
    // Publish
    const char* topicPowerStatus =  "power";        
    const char* topicConnection =   "connection";   
    const char* topicTemp =         "temperature";  
    const char* topicTempMessage =  "temperature/message";
    const char* topicColorStatus =  "color";        
    const char* topicColorMessage = "color/message";
    const char* topicBrightnessStatus = "brightness";
    const char* topicModeStatus =   "mode";         
    const char* topicModeMessage =  "mode/message"; 

    LedMqttService();

    void init(const char* ssid, const char* password);
    void update();
    void sendData(String data, const char* topic, bool retained = false);
    bool isConnected();
    void onPowerChange(PowerChangeCallback callback);
    void onColorChange(ColorChangeCallback callback);
    void onBrightnessChange(BrightnessChangeCallback callback);
    void onModeChange(ModeChangeCallback callback);
};

#endif