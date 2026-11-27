#ifndef COMMAND_ROUTER_H
#define COMMAND_ROUTER_H

#include "ModeCommand.h"

typedef void (*PowerChangeCallback)(bool);
typedef void (*ColorChangeCallback)(uint8_t, uint8_t, uint8_t);
typedef void (*BrightnessChangeCallback)(uint8_t);
typedef void (*ModeChangeCallback)(const ModeCommand&);

class CommandRouter {
private:
    PowerChangeCallback powerCb = nullptr;
    ColorChangeCallback colorCb = nullptr;
    BrightnessChangeCallback brightnessCb = nullptr;
    ModeChangeCallback modeCb = nullptr;

public:

    void onPower(PowerChangeCallback callback) { powerCb = callback; }
    void onColor(ColorChangeCallback callback) { colorCb = callback; }
    void onBrightness(BrightnessChangeCallback callback ) { brightnessCb = callback; }
    void onMode(ModeChangeCallback callback) { modeCb = callback; }

    bool handle(const char* name, const char* payload, bool fromGroup = false);
};

#endif