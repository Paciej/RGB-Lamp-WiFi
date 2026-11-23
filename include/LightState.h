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

inline const char* modeToString(LightMode m) {
    switch (m)
    {
    case LightMode::Wave:   return "Wave";
    case LightMode::Candle: return "Candle";
    case LightMode::Pulse:  return "Pulse";
    default:                return "None";
    }
}

inline bool modeFromString(const char* s, LightMode& out) {
    if (strcmp(s, "None") == 0)     out = LightMode::None;
    else if (strcmp(s, "Wave") == 0) out = LightMode::Wave;
    else if (strcmp(s, "Candle") == 0) out = LightMode::Candle;
    else if (strcmp(s, "Pulse") == 0) out = LightMode::Pulse;
    else return false;
    return true;
}

#endif