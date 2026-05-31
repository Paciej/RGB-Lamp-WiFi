#include "LedBLEService.h"

// --- Callback definitions ---

class LedBLEServerCallbacks: public BLEServerCallbacks {
    LedBLEService* ledService;
public:
    LedBLEServerCallbacks(LedBLEService* ls) : ledService(ls) {}
    
    void onConnect(BLEServer* pServer) {
        ledService->isDeviceConnected = true;
        Serial.println("BLE: Device connected");
    };

    void onDisconnect(BLEServer* pServer) {
        ledService->isDeviceConnected = false;
        Serial.println("BLE: Device disconnected");
    }
};

class LedBLECharacteristicCallbacks: public BLECharacteristicCallbacks {
    LedBLEService* ledService;
public:
    LedBLECharacteristicCallbacks(LedBLEService* ls) : ledService(ls) {}

    void onWrite(BLECharacteristic *pCharacteristic) {
        String rxValue = pCharacteristic->getValue().c_str();
        
        if (rxValue.length() > 0) {
            String receivedData = String(rxValue.c_str());
            receivedData.trim();
            Serial.printf("BLE Received: %s\n", receivedData.c_str());

            // Logika parsowania (przeniesiona z main)
            if (receivedData == "Wave" || receivedData == "Pulse" || receivedData == "Candle") {
                if (ledService->modeCallback) {
                    ledService->modeCallback(receivedData);
                }
            } else {
                // Próba parsowania koloru R,G,B lub R.G.B
                int firstComma = receivedData.indexOf(',');
                int secondComma = receivedData.indexOf(',', firstComma + 1);
                
                if (firstComma > 0 && secondComma > firstComma) {
                    String redStr = receivedData.substring(0, firstComma);
                    String greenStr = receivedData.substring(firstComma + 1, secondComma);
                    String blueStr = receivedData.substring(secondComma + 1);
                    
                    int r = constrain(redStr.toInt(), 0, 255);
                    int g = constrain(greenStr.toInt(), 0, 255);
                    int b = constrain(blueStr.toInt(), 0, 255);

                    if (ledService->colorCallback) {
                        ledService->colorCallback(r, g, b);
                    }
                }
            }
        }
    }
};

// ----- LedBLEService -----

void LedBLEService::init(String deviceName) {
  BLEDevice::init(deviceName.c_str());
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new LedBLEServerCallbacks(this));

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
                          CHARACTERISTIC_UUID_TX,
                          BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
                                            CHARACTERISTIC_UUID_RX,
                                            BLECharacteristic::PROPERTY_WRITE
                                        );
  pRxCharacteristic->setCallbacks(new LedBLECharacteristicCallbacks(this));

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();
  Serial.println("BLE: Ready and advertising");
}

void LedBLEService::update() {
  if (!isDeviceConnected && isOldDeviceConnected) {
    delay(500); 
    pServer->startAdvertising(); 
    Serial.println("BLE: Restart advertising");
    isOldDeviceConnected = isDeviceConnected;
  }
  if (isDeviceConnected && !isOldDeviceConnected) {
    isOldDeviceConnected = isDeviceConnected;
  }
}

void LedBLEService::sendData(String message) {
  if (isDeviceConnected && pTxCharacteristic) {
    pTxCharacteristic->setValue(message.c_str());
    pTxCharacteristic->notify();
  }
}

bool LedBLEService::isConnected() {
  return isDeviceConnected;
}

void LedBLEService::onColorChange(ColorChangeCallback callback) {
    this->colorCallback = callback;
}

void LedBLEService::onModeChange(ModeChangeCallback callback) {
    this->modeCallback = callback;
}