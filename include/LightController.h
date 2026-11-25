#ifndef LIGHT_CONTROLLER_H
#define LIGHT_CONTROLLER_H

#include "LightState.h"

struct LightPins {
    uint8_t red, green, blue;
    uint8_t chRed, chGreen, chBlue; 
    uint32_t freq;
    uint8_t resolution;

    LightPins(uint8_t r, uint8_t g, uint8_t b, 
            uint8_t chR = 0, uint8_t chG = 1, uint8_t chB = 2,
            uint32_t freq = 5000, uint8_t res = 8) : red(r), green(g), blue(b),
                    chRed(chR), chGreen(chG), chBlue(chB),
                    freq(freq), resolution(res) {}
};

class LightController {
private:
    Rgb computeOutput() const;
    void writePwm(const Rgb& c);

    LightState _state;
    LightPins _pins;
    Rgb effectColor;
    bool effectActive = false;
    uint8_t outputLimit = 255;
    uint8_t changes = 0;
    bool outputDirty = true;

public:

    explicit LightController(const LightPins& pins) : _pins(pins) {}

    enum Change : uint8_t {
        CH_COLOR = 1, CH_BRIGHTNESS = 2, CH_POWER = 4, CH_MODE = 8
    };

    void begin();
    void update();

    void setColor(Rgb color);
    void setBrightness(uint8_t brightness);
    void setPower(bool on);
    void setMode(LightMode mode);

    void setEffectColor(Rgb color);
    void setOutputLimit(uint8_t limit);

    uint8_t takeChanges();
    const LightState& state() const { return _state; }
    void markAllChanged() { changes = 0xFF; }
    void restore(const LightState& st);
};

#endif