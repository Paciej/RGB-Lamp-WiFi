#ifndef STORAGE_H
#define STORAGE_H

#include <Preferences.h>
#include "LightState.h"
#include "Effects.h"
#include "NetConfig.h"

static const size_t ROOM_MAX = 32;
static const uint32_t SAVE_DELAY_MS = 5000;

enum class RestorePolicy : uint8_t {None = 0, Color = 1, Full = 2 };

class Storage {
private:
    struct Persisted {
        uint8_t version, power, mode, brightness;
        uint8_t r, g, b;
        uint8_t er, eg, eb;
        uint32_t periodMs;
    };
    static const uint8_t STATE_VERSION = 1;

    Preferences prefs;
    char room[ROOM_MAX + 1] = {0};
    RestorePolicy policy = RestorePolicy::Color;

    Persisted last;
    bool haveLast = false;
    bool dirty = false;
    uint32_t dirtyAt = 0;

public:
    void begin();

    const char* roomId() { return room; }
    bool setRoomId(const char* id);
    void clearRoomId();
    RestorePolicy restorePolicy() { return policy; }
    void setRestorePolicy(RestorePolicy p);

    bool loadState(LightState& s, EffectParams& p);
    void markStateDirty();
    bool stateDueSave() const;
    void saveState(const LightState& s, const EffectParams& p);
    
    bool loadNetConfig(NetConfig& c);
    bool saveNetConfig(const NetConfig& c);
    void clearNetConfig();
};

#endif