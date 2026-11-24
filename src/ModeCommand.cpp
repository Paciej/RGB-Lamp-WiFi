#include "ModeCommand.h"
#include "Effects.h"
#include <ArduinoJson.h>

bool parseModeCommand(const char* payload, ModeCommand& out) {
    out = ModeCommand();

    while (*payload == ' ' || *payload == '\n' || *payload == '\r') payload++;

    if (*payload != '{') {
        return modeFromString(payload, out.mode);
    }

    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, payload)) return false;

    const char* m = doc["mode"];
    if (!m || !modeFromString(m, out.mode)) return false;

    if (doc["period"].as<uint32_t>()) {
        out.hasPeriod = true;
        out.periodMs = doc["period"].as<uint32_t>();
    }

    JsonArray c = doc["color"].as<JsonArray>();
    if (!c.isNull()) {
        if (c.size() != 3) return false;
            out.hasColor = true;
            out.color = Rgb(constrain(c[0].as<int>(), 0, 255),
                            constrain(c[1].as<int>(), 0, 255),
                            constrain(c[2].as<int>(), 0, 255));
        }

    return true;
}