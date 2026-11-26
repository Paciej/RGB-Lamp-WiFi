#ifndef LED_MQTT_SERVICE_H
#define LED_MQTT_SERVICE_H

#include <WiFi.h>
#include <PubSubClient.h>
#include "ModeCommand.h"
#include "NetConfig.h"

typedef void (*PowerChangeCallback)(bool);
typedef void (*ColorChangeCallback)(uint8_t, uint8_t, uint8_t);
typedef void (*BrightnessChangeCallback)(uint8_t);
typedef void (*ModeChangeCallback)(const ModeCommand&);
using RoomChangeCallback = void (*)(const char* roomId);

static constexpr size_t MAX_SIZE = 64;
constexpr const char* infoJson = 
"{\"schema\":1,"
"\"fw\":\"0.2.0\","
"\"capabilities\":{"
    "\"color\":true,"
    "\"brightness\":true,"
    "\"modes\":[\"None\",\"Wave\",\"Candle\",\"Pulse\"],"
    "\"temperature\":true}}";

class LedMqttService {
private:
    enum class NetState : uint8_t { Idle, WifiConnecting, MqttConnecting, Online };

    static const uint32_t WIFI_ATTEMPT_TIMEOUT = 20000;
    static const uint32_t MQTT_BACKOFF_MIN = 2000;
    static const uint32_t MQTT_BACKOFF_MAX = 30000;


    PowerChangeCallback powerCallback = nullptr;
    ColorChangeCallback colorCallback = nullptr;
    BrightnessChangeCallback brightnessCallback = nullptr;
    ModeChangeCallback modeCallback = nullptr;
    RoomChangeCallback roomCallback = nullptr;
    
    // MQTT clients
    WiFiClient wifiClient;
    PubSubClient* mqttClient;
    IPAddress mqttAddress;
    NetConfig cfg;

    NetState state = NetState::Idle;
    uint32_t stateSince = 0;
    uint32_t nextAttempt = 0;
    uint32_t backoff = MQTT_BACKOFF_MIN;
    uint32_t offlineSince = 0;

    char device_id[13] = {0};

    char room_id[MAX_SIZE] = {0};
    char pf_cmd[MAX_SIZE] = {0};
    char pf_status[MAX_SIZE] = {0};
    char pf_cfg[MAX_SIZE] = {0};
    char pf_room[MAX_SIZE] = {0};
    char willTopic[MAX_SIZE] = {0};
    size_t cmdLen = 0;
    size_t statusLen = 0;
    size_t cfgLen = 0;
    size_t roomLen = 0;

    // TOPICS
    // ---------------------------------------------
    // Subscribe
    const char* topicPower = "power";
    const char* topicColor = "color";
    const char* topicBrightness = "brightness";
    const char* topicMode =  "mode";
    const char* topicRoom = "room";

    void callback(char* topic, byte* payload, unsigned int length);
    const char* commandOf(const char* topic) const;
    void startWifi();
    void enterMqttConnecting(uint32_t now);
    String readDeviceId();
    void reconnect();
    bool resolveMqttBroker();
    void tryMqttConnect();
    void sendInfo();

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

    void begin(const NetConfig& config);
    void init(const char* ssid, const char* password);
    void update();
    void sendData(const char* data, const char* topic, bool retained = false);
    void setRoom(const char* roomId);
    bool isConnected();
    uint32_t offlineMs() const;
    void onPowerChange(PowerChangeCallback callback);
    void onColorChange(ColorChangeCallback callback);
    void onBrightnessChange(BrightnessChangeCallback callback);
    void onModeChange(ModeChangeCallback callback);
    void onRoomChange(RoomChangeCallback callback);
};

#endif