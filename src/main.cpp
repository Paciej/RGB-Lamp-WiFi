#include <OneWire.h>
#include <DallasTemperature.h>
#include <esp_system.h>
#include "LedMqttService.h"
#include "Secrets.h"


/********* Function declarations *********/
void updateLED();

/********* Const variables *********/
  // Pins
const unsigned int PIN_RED = 25;
const unsigned int PIN_GREEN = 26; 
const unsigned int PIN_BLUE = 27;
const unsigned int PIN_BUTTON = 21;
const unsigned int PIN_TEMP = 18;

  // LEDC PWM channels for ESP32 Arduino Core 2.x
const uint8_t PWM_CH_RED = 0;
const uint8_t PWM_CH_GREEN = 1;
const uint8_t PWM_CH_BLUE = 2;
const uint32_t PWM_FREQ = 5000;
const uint8_t PWM_RESOLUTION = 8;

  // Other
const unsigned int LED_CRIT_MAX_INTENSITY = 100;
const unsigned long TEMP_MEASURE_INTERVAL = 5000;
const unsigned long TEMP_CONVERSION_DELAY = 750;
const unsigned long BTN_DEBOUNCE_DELAY = 50;
const float TEMP_CRIT = 85.f;
const float TEMP_CRIT_REVERT = 70.f;

/****** Variables *******/
enum class ColorModes {
  None,
  Wave,
  Pulse,
  Candle
};
ColorModes currentMode = ColorModes::None;

// RGB LEDs [0..255]
int ledRed = 0;
int ledGreen = 0;
int ledBlue = 0;

// MQTT server and connection
LedMqttService mqtt;

// state variables
bool isLedTempCrit = false;
bool tempRequestPending = false;
unsigned long lastTempRequest = 0;
unsigned long lastTempCheckTime = 0;

// Temperature variables
float lastWrittenTemp = 0;

// button variables
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;

// mode variables 
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

OneWire oneWire(PIN_TEMP);
DallasTemperature tempSensors(&oneWire);

/********** LOGIC **********/
void writeLedStatus() {
  if (!mqtt.isConnected()) return;
  String response = "RGB: " + String(ledRed) + "," + String(ledGreen) + "," + String(ledBlue);
  mqtt.sendData(response, mqtt.topicColor);
  //Serial.printf("RGB values: R=%d G=%d B=%d \n", ledRed, ledGreen, ledBlue);
}

// handle getting color from callback
void handleColorFromMqtt(int r, int g, int b) {
  ledRed = r;
  ledGreen = g;
  ledBlue = b;
  currentMode = ColorModes::None;
  updateLED();
}

// handle getting mode from callback
void handleModeFromMqtt(String mode) {
  mqtt.sendData(mode, mqtt.topicMode);
  if (mode == "Wave") {
      currentMode = ColorModes::Wave;
  } else if (mode == "Pulse") {
      currentMode = ColorModes::Pulse;
      baseRed = ledRed;
      baseGreen = ledGreen;
      baseBlue = ledBlue;
  } else if (mode == "Candle") {
      currentMode = ColorModes::Candle;
  }
}

#pragma region led modes
void waveMode() {
  float angle = fmod((millis() * waveSpeed) / 1000.0, 360.0);
  
  ledRed = (sin(angle * PI / 180.0) + 1.0) * 127.5;
  ledGreen = (sin((angle + 120) * PI / 180.0) + 1.0) * 127.5;
  ledBlue = (sin((angle + 240) * PI / 180.0) + 1.0) * 127.5;
  updateLED();
}

void pulseMode() {
  float angle = (millis() * pulseSpeed) / 1000.0;
  float brightnessFactor = (sin(angle) + 1.0) / 2.0; 
  
  ledRed = baseRed * brightnessFactor;
  ledGreen = baseGreen * brightnessFactor;
  ledBlue = baseBlue * brightnessFactor;

  updateLED();
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

  ledRed   = currentBrightness;
  ledGreen = constrain(((currentBrightness * 140) / 255) + greenFlicker, 0, 255);
  ledBlue  = (currentBrightness * 10) / 255; 
  updateLED();
}
#pragma endregion

void handleModes() {

  if (currentMode == ColorModes::None) return;

  if (isLedTempCrit) {
    currentMode = ColorModes::None;
    mqtt.sendData("Effect turned off due to critical temperature!", mqtt.topicMode);
    // Warning color
    ledRed = LED_CRIT_MAX_INTENSITY;
    ledGreen = 0;
    ledBlue = 0;
    updateLED();
    return;
  }

  switch (currentMode) {
    case ColorModes::None :
      return;
    case ColorModes::Wave :
      waveMode();
      break;
    case ColorModes::Pulse :
      pulseMode();
      break;
    case ColorModes::Candle :
      candleMode();
      break;
  }
}

void updateLED() {
  if (!isLedTempCrit || 
     (ledRed <= LED_CRIT_MAX_INTENSITY && ledGreen <= LED_CRIT_MAX_INTENSITY && ledBlue <= LED_CRIT_MAX_INTENSITY)) {
      ledcWrite(PWM_CH_RED, ledRed);
      ledcWrite(PWM_CH_GREEN, ledGreen);
      ledcWrite(PWM_CH_BLUE, ledBlue);
      writeLedStatus();
  } else {
      mqtt.sendData("Temperature is too high", mqtt.topicTemp);
  }
}

void updateTemp() {
  unsigned long currentMillis = millis();

  if (!tempRequestPending && (currentMillis - lastTempCheckTime >= TEMP_MEASURE_INTERVAL)) {
    tempSensors.requestTemperatures();
    tempRequestPending = true;
    lastTempRequest = currentMillis;
    lastTempCheckTime = currentMillis;
  }

  if (tempRequestPending && (currentMillis - lastTempRequest >= TEMP_CONVERSION_DELAY)) {
    float tempC = tempSensors.getTempCByIndex(0);
    tempRequestPending = false;

    if (tempC == -127.00) {
      return; 
      Serial.println("ERROR127: temperature scanner does not work properly");
    }

    if (tempC - lastWrittenTemp != 0) {
      String response = String(tempC) + "C";
      mqtt.sendData(response, mqtt.topicTemp);
      lastWrittenTemp = tempC;
    }

    if (tempC > TEMP_CRIT && !isLedTempCrit) {
      isLedTempCrit = true;
      currentMode = ColorModes::None;
      
      String response = "ALARM TEMP: Temperature is over " + String(TEMP_CRIT) + "C";
      mqtt.sendData(response, mqtt.topicMessage);
      Serial.printf("%s", response);

      ledRed = (ledRed > LED_CRIT_MAX_INTENSITY) ? LED_CRIT_MAX_INTENSITY : ledRed;
      ledGreen = (ledGreen > LED_CRIT_MAX_INTENSITY) ? LED_CRIT_MAX_INTENSITY : ledGreen;
      ledBlue = (ledBlue > LED_CRIT_MAX_INTENSITY) ? LED_CRIT_MAX_INTENSITY : ledBlue;
      
      updateLED();
    } else if (isLedTempCrit && tempC < TEMP_CRIT_REVERT) {
      isLedTempCrit = false;
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
        
        currentMode = ColorModes::None;
        
        ledRed = constrain(esp_random() % 256, 0, 255);
        ledGreen = constrain(esp_random() % 256, 0, 255);
        ledBlue = constrain(esp_random() % 256, 0, 255);
        
        if (isLedTempCrit) {
            if(ledRed > LED_CRIT_MAX_INTENSITY) ledRed = LED_CRIT_MAX_INTENSITY;
            if(ledGreen > LED_CRIT_MAX_INTENSITY) ledGreen = LED_CRIT_MAX_INTENSITY;
            if(ledBlue > LED_CRIT_MAX_INTENSITY) ledBlue = LED_CRIT_MAX_INTENSITY;
        }

        updateLED();
        writeLedStatus();
      }
    }
  }
  lastButtonState = reading;
}

void setup() {
  Serial.begin(115200);
  
  ledcSetup(PWM_CH_RED, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CH_GREEN, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CH_BLUE, PWM_FREQ, PWM_RESOLUTION);

  ledcAttachPin(PIN_RED, PWM_CH_RED);
  ledcAttachPin(PIN_GREEN, PWM_CH_GREEN);
  ledcAttachPin(PIN_BLUE, PWM_CH_BLUE);
  
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  
  tempSensors.begin();
  tempSensors.setWaitForConversion(false);

  ledRed = 100; ledGreen = 100; ledBlue = 100;
  currentMode = ColorModes::None;
  updateLED();

  mqtt.onColorChange(handleColorFromMqtt);
  mqtt.onModeChange(handleModeFromMqtt);
  mqtt.init(WIFI_SSID, WIFI_PASSWORD);
  writeLedStatus();
  mqtt.sendData("Test message", mqtt.topicMessage);
  mqtt.sendData("None", mqtt.topicMode);
}

void loop() {

  mqtt.update();
  //updateTemp();    
  //updateButton(); 
  //handleModes(); 

  delay(10);
}