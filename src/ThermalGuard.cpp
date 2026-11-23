#include "ThermalGuard.h"

void ThermalGuard::begin() {
    sensors.begin();
    sensors.setWaitForConversion(false);
    lastMeasure = millis() - cfg.measureInterval;
}

void ThermalGuard::update() {
    uint32_t now = millis();

    if (!requestPending && (now - lastMeasure >= cfg.measureInterval)) {
        sensors.requestTemperatures();
        requestPending = true;
        lastRequest = now;
        lastMeasure = now;
        return;
    }

    if (!requestPending || (now - lastRequest < CONVERSION_DELAY)) return;
    requestPending = false;

    float t = sensors.getTempCByIndex(0);

    if (t == DEVICE_DISCONNECTED_C) {
        if (!sensorFailed) {
        sensorFailed = true;
        events |= EV_SENSOR_ERROR;
        }
        return;
    }
    if (sensorFailed) {
        sensorFailed = false;
        events |= EV_SENSOR_OK;
    }

    if (t != lastTemp) {
        lastTemp = t;
        events |= EV_TEMP_CHANGED;
    }

    if (!critical && t > cfg.critTemp) {
        critical = true;
        light.setMode(LightMode::None);
        light.setOutputLimit(cfg.outputLimit);
        events |= EV_ALARM;
    } else if (critical && t < cfg.revertTemp) {
        critical = false;
        light.setOutputLimit(255);
        events |= EV_RECOVERED;
    }
}

uint8_t ThermalGuard::takeEvents() {
    uint8_t e = events;
    events = 0;
    return e;
}