#include <OneWire.h>
#include <DallasTemperature.h>
#include <esp_system.h>
#include "LightController.h"
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
LedMqttService mqtt;
OneWire oneWire(PIN_TEMP);
DallasTemperature tempSensors(&oneWire);

/********* Const variables *********/
const uint8_t LED_CRIT_MAX_INTENSITY = 100;
const uint32_t TEMP_MEASURE_INTERVAL = 5000;
const uint32_t TEMP_CONVERSION_DELAY = 750;
const uint32_t BTN_DEBOUNCE_DELAY = 50;
const float TEMP_CRIT = 31.f;
const float TEMP_CRIT_REVERT = 29.f;

/****** Variables *******/
  // temperature
bool isLedTempCrit = false;
bool tempRequestPending = false;
unsigned long lastTempRequest = 0;
unsigned long lastTempCheckTime = 0;
float lastWrittenTemp = 0;

  // button
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;

  // mqtt
bool wasMqttConnected = false;

// mode 
  // wave
float waveSpeed = 60.0;    
  // pulse   
float pulseSpeed = 3.0;   
int baseRed = 0, baseGreen = 0, baseBlue = 0; // for pulse mode remember state
  // candle
unsigned long lastFlicker = 0;
int flickerInterval = 100;
int targetBrightness = 255;
int currentBrightness = 255;

/****** MQTT handlers *******/
void handleBrightnessFromMqtt(int brightness) { light.setBrightness(brightness); }
void handlePowerFromMqtt(bool power) { light.setPower(power); }
void handleColorFromMqtt(int r, int g, int b) { light.setColor(Rgb{r, g, b}); }
void handleModeFromMqtt(String mode) { 
  LightMode m;
  if (modeFromString(mode.c_str(), m)) light.setMode(m);
}

#pragma region led modes
void waveMode() {
  float angle = fmod((millis() * waveSpeed) / 1000.0, 360.0);
  
  light.setEffectColor(Rgb{
      (uint8_t)((sin(angle * PI / 180.0) + 1.0) * 127.5),
      (uint8_t)((sin((angle + 120) * PI / 180.0) + 1.0) * 127.5),
      (uint8_t)((sin((angle + 240) * PI / 180.0) + 1.0) * 127.5)
    });
}

void pulseMode() {
  float angle = (millis() * pulseSpeed) / 1000.0;
  float f = (sin(angle) + 1.0) / 2.0; 
  const Rgb& base = light.state().color;

  light.setEffectColor(Rgb{
    (uint8_t)(base.r * f),
    (uint8_t)(base.g * f),
    (uint8_t)(base.b * f)
  });
}

void candleMode() {
  if (millis() - lastFlicker > flickerInterval) {
    lastFlicker = millis();
    targetBrightness = random(120, 256);
    flickerInterval = random(50, 150); 
  }

  if (currentBrightness < targetBrightness) currentBrightness += 5;
  if (currentBrightness > targetBrightness) currentBrightness -= 5;
  currentBrightness = constrain(currentBrightness, 0, 255);
  int greenFlicker = random(-20, 20);

  light.setEffectColor(Rgb{
    (uint8_t)(currentBrightness),
    (uint8_t)(constrain(((currentBrightness * 140) / 255) + greenFlicker, 0, 255)),
    (uint8_t)((currentBrightness * 10) / 255)
  });
}
#pragma endregion

void handleModes() {
  LightMode mode = light.state().mode;
  if (mode == LightMode::None) return;

  if (isLedTempCrit) {
    light.setMode(LightMode::None);
    light.setColor(Rgb{LED_CRIT_MAX_INTENSITY, 0, 0});
    mqtt.sendData("Effect turned off due to critical temperature!", mqtt.topicModeMessage);
    return;
  }

  switch (mode) {
    case LightMode::Wave : waveMode(); break;
    case LightMode::Pulse : pulseMode(); break;
    case LightMode::Candle : candleMode(); break;
    default: break;
  }
}

void updateTemp() {
  unsigned long now = millis();

  if (!tempRequestPending && (now - lastTempCheckTime >= TEMP_MEASURE_INTERVAL)) {
    tempSensors.requestTemperatures();
    tempRequestPending = true;
    lastTempRequest = now;
    lastTempCheckTime = now;
  }

  if (tempRequestPending && (now - lastTempRequest >= TEMP_CONVERSION_DELAY)) {
    float tempC = tempSensors.getTempCByIndex(0);
    tempRequestPending = false;

    if (tempC == -127.00) {
      Serial.println("ERROR127: temperature scanner does not work properly");
      mqtt.sendData(String("Temperature sensor does not work properly"), mqtt.topicTempMessage);
      return; 
    }

    if (tempC - lastWrittenTemp != 0) {
      mqtt.sendData(String(tempC), mqtt.topicTemp);
      lastWrittenTemp = tempC;
    }

    if (tempC > TEMP_CRIT && !isLedTempCrit) {
      isLedTempCrit = true;
      light.setMode(LightMode::None);
      mqtt.sendData("TEMP: Temperature is over" + String(TEMP_CRIT) + "C", mqtt.topicTempMessage);
    }
    else if (isLedTempCrit && tempC < TEMP_CRIT_REVERT) {
      isLedTempCrit = false;
      mqtt.sendData("Temperature cool down. All functions are avaiable", mqtt.topicTempMessage);
    }
  }
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
    mqtt.sendData((String)(s.color.r) + "," + (String)(s.color.g) + "," + (String)(s.color.b), mqtt.topicColorStatus, true);
  }
  if (ch & LightController::CH_BRIGHTNESS) mqtt.sendData(String(s.brightness), mqtt.topicBrightnessStatus, true);
  if (ch & LightController::CH_POWER)      mqtt.sendData(s.power ? "ON" : "OFF", mqtt.topicPowerStatus, true);
  if (ch & LightController::CH_MODE)       mqtt.sendData(modeToString(s.mode), mqtt.topicModeStatus);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  tempSensors.begin();
  tempSensors.setWaitForConversion(false);

  light.begin();

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

  if (light.state().power) {
    updateTemp();    
    updateButton(); 
    handleModes(); 
  }

  light.update();
  publishChanges();
  delay(10);
}