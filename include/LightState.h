#ifndef LIGHT_STATE_H
#define LIGHT_STATE_H

#include <Arduino.h>

struct Rgb {
    uint8_t r, g, b;

    Rgb(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0)
        : r(red), g(green), b(blue) {}
};

enum class LightMode : uint8_t {None, Wave, Pulse, Candle};

struct LightState {
    Rgb color{100, 100, 100};
    uint8_t brightness = 255;
    bool power = true;
    LightMode mode = LightMode::None;
};

#endif