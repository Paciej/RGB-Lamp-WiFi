#include <Arduino.h>
#include <esp_system.h>
#include "LightController.h"
#include "ThermalGuard.h"
#include "Effects.h"
#include "LedMqttService.h"
#include "Secrets.h"

/********* Hardware *********/
  // Pins
const uint8_t PIN_RED = 25;
const uint8_t PIN_GREEN = 26; 
const uint8_t PIN_BLUE = 27;
const uint8_t PIN_BUTTON = 21;
const uint8_t PIN_TEMP = 18;

/********* Objects *********/
LightController light(LightPins(PIN_RED, PIN_GREEN, PIN_BLUE));
ThermalGuard guard(light, PIN_TEMP);
EffectEngine effects(light);
LedMqttService mqtt;

/****** Variables *******/
  // button
const uint32_t BTN_DEBOUNCE_DELAY = 50;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;

  // mqtt
bool wasMqttConnected = false;

/****** MQTT handlers *******/
void handleBrightnessFromMqtt(uint8_t brightness) { light.setBrightness(brightness); }
void handlePowerFromMqtt(bool power) { light.setPower(power); }
void handleColorFromMqtt(uint8_t r, uint8_t g, uint8_t b) { light.setColor(Rgb{r, g, b}); }
void handleModeFromMqtt(const char* mode) { 
  LightMode m;
  if (!modeFromString(mode, m)) return;
  if (guard.isCritial() && m != LightMode::None) {
    mqtt.sendData("Effect blocked due to critical temperature!", mqtt.topicModeMessage);
    return;
  }
  effects.start(m);
}

void updateButton() {
  int reading = digitalRead(PIN_BUTTON);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > BTN_DEBOUNCE_DELAY) {
    static int stableState = HIGH;
    if (reading != stableState) {
      stableState = reading;
      if (stableState == LOW) {
        light.setColor(Rgb{
          (uint8_t)(constrain(esp_random() % 256, 0, 255)),
          (uint8_t)(constrain(esp_random() % 256, 0, 255)),
          (uint8_t)(constrain(esp_random() % 256, 0, 255))
        });
      }
    }
  }
  lastButtonState = reading;
}

void publishChanges() {
  uint8_t ch = light.takeChanges();
  if (!ch) return;
  const LightState& s = light.state();

  if (ch && LightController::CH_COLOR) {
    char msg[16];
    snprintf(msg, sizeof(msg), "%u,%u,%u", s.color.r, s.color.g, s.color.b);
    mqtt.sendData(msg, mqtt.topicColorStatus, true);
  }
  if (ch & LightController::CH_BRIGHTNESS) mqtt.sendData(String(s.brightness).c_str(), mqtt.topicBrightnessStatus, true);
  if (ch & LightController::CH_POWER)      mqtt.sendData(s.power ? "ON" : "OFF", mqtt.topicPowerStatus, true);
  if (ch & LightController::CH_MODE)       mqtt.sendData(modeToString(s.mode), mqtt.topicModeStatus);
}

void publishThermal() {
  uint8_t ev = guard.takeEvents();
  if (!ev || !mqtt.isConnected()) return;

  if (ev & ThermalGuard::EV_TEMP_CHANGED) {
    mqtt.sendData(String(guard.temperature()).c_str() , mqtt.topicTemp);
  }
  if (ev & ThermalGuard::EV_ALARM) {
    char msg[64];
    snprintf(msg, sizeof(msg), "%s %f%c", "ALARM TEMP: Temperature is over ", guard.critTemp(), 'C');
    mqtt.sendData(msg, mqtt.topicTempMessage);
  }
  if (ev & ThermalGuard::EV_RECOVERED) {
    mqtt.sendData("Temperature cooled down. All functions are available" , mqtt.topicTempMessage);
  }
  if (ev & ThermalGuard::EV_SENSOR_ERROR) {
    mqtt.sendData("Temperature sensor does not work properly" , mqtt.topicTempMessage);
  }
  if (ev & ThermalGuard::EV_SENSOR_OK) {
    mqtt.sendData("Temperature sensor is working again" , mqtt.topicTempMessage);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  light.begin();

  guard.begin();

  mqtt.onPowerChange(handlePowerFromMqtt);
  mqtt.onColorChange(handleColorFromMqtt);
  mqtt.onBrightnessChange(handleBrightnessFromMqtt);
  mqtt.onModeChange(handleModeFromMqtt);
  mqtt.init(WIFI_SSID, WIFI_PASSWORD);
}

void loop() {
  mqtt.update();

  bool connected = mqtt.isConnected();
  if (connected && !wasMqttConnected) light.markAllChanged();
  wasMqttConnected = connected;

  guard.update();

  if (light.state().power) {
    updateButton(); 
    effects.update();
  }

  light.update();
  publishChanges();
  publishThermal();
  delay(10);
}