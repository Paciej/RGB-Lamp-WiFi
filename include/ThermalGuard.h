#ifndef THERMAL_GUARD_H
#define THERMAL_GUARD_H

#include <OneWire.h>
#include <DallasTemperature.h>
#include "LightController.h"

struct ThermalConfig {
    float critTemp;
    float revertTemp;
    uint32_t measureInterval;
    uint8_t outputLimit;

    ThermalConfig(float crit = 31.f, float revert = 27.f, uint32_t interval = 5000, uint8_t limit = 100)
        : critTemp(crit), revertTemp(revert), measureInterval(interval), outputLimit(limit) {}
};

class ThermalGuard {
private:
    LightController& light;
    OneWire oneWire;
    DallasTemperature sensors;
    const ThermalConfig cfg;

    static const uint32_t CONVERSION_DELAY = 750;

    bool critical = false;
    bool sensorFailed = false;
    bool requestPending = false;
    uint32_t lastRequest = 0;
    uint32_t lastMeasure = 0;
    float lastTemp = 0;
    uint8_t events = 0;

public:
    enum Event : uint8_t {
        EV_TEMP_CHANGED = 1,
        EV_ALARM = 2,
        EV_RECOVERED = 4,
        EV_SENSOR_ERROR = 8,
        EV_SENSOR_OK = 16,
    };

    ThermalGuard(LightController& light, uint8_t pin, const ThermalConfig& cfg = ThermalConfig())
        : light(light), oneWire(pin), sensors(&oneWire), cfg(cfg) {}
    
    void begin();
    void update();

    uint8_t takeEvents();
    bool isCritial() { return critical; }
    float temperature() { return lastTemp; }
    float critTemp() { return cfg.critTemp; } 
};

#endif