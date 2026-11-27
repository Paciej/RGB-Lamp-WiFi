#include "CommandRouter.h"


bool CommandRouter::handle(const char* name, const char* payload, bool fromGroup) {

    Serial.printf("In Command Router. Topic: %s, payload: %s\n", name, payload);

    if (strcmp(name, "color") == 0) {
        const char* p = payload;
        char* end;
        long v[3];
        for (int i = 0; i < 3; i++) {
        v[i] = strtol(p, &end, 10);
        if (end == p) return true;
            if (i < 2) {
                if (*end != ',') return true;
                p = end + 1;
            }
        }
        if (colorCb) colorCb((uint8_t)constrain(v[0], 0, 255),
                            (uint8_t)constrain(v[1], 0, 255),
                            (uint8_t)constrain(v[2], 0, 255));
        return true;
    }
    else if (strcmp(name, "brightness") == 0) {
        if (brightnessCb) brightnessCb((uint8_t)constrain(atoi(payload), 0, 255));
        return true;
    }
    else if (strcmp(name, "mode") == 0) {
        ModeCommand cmd;
        if (parseModeCommand(payload, cmd) && modeCb) modeCb(cmd);
        return true;
    }
    else if (strcmp(name, "power") == 0) {
        if (!powerCb) return true;
        if (strcmp(payload, "ON") == 0) powerCb(true);
        else if (strcmp(payload, "OFF") == 0) powerCb(false);
        return true;
    }
    return false;
}