#ifndef LED_BLE_SERVICE_H
#define LED_BLE_SERVICE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UUID
#define SERVICE_UUID            "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


typedef void (*ColorChangeCallback)(int, int, int);
typedef void (*ModeChangeCallback)(String);

class LedBLEService {

private:
  friend class LedBLEServerCallbacks;
  friend class LedBLECharacteristicCallbacks;

  ColorChangeCallback colorCallback = nullptr;
  ModeChangeCallback modeCallback = nullptr;

  BLEServer* pServer = nullptr;
  BLECharacteristic* pTxCharacteristic = nullptr;
  bool isDeviceConnected = false;
  bool isOldDeviceConnected = false;

public:

  LedBLEService() {}

  void init(String deviceName = "ESP32_RGB_LAMP");
  void update();  
  void sendData(String message);
  bool isConnected();
  void onColorChange(ColorChangeCallback callback);
  void onModeChange(ModeChangeCallback callback);
};

#endif