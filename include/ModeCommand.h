#ifndef MODE_COMMAND_H
#define MODE_COMMAND_H

#include "LightState.h"

struct ModeCommand {
    LightMode mode = LightMode::None;
    bool hasColor = false;
    Rgb color;
    bool hasPeriod = false;
    uint32_t periodMs = 0;
};

bool parseModeCommand(const char* payload, ModeCommand& out);

#endif