#include "Storage.h"

static const char* NS_CFG = "cfg";
static const char* NS_STATE = "state";

void Storage::begin() {
    room[0] = 0;
    prefs.begin(NS_CFG, false);
    prefs.getString("room", room, sizeof(room));
    room[ROOM_MAX] = 0;
    uint8_t pol = prefs.getUChar("restore", (uint8_t)RestorePolicy::Color);
    prefs.end();
    policy = (pol <= (uint8_t)RestorePolicy::Full) ? (RestorePolicy)pol : RestorePolicy::Color;
}

bool Storage::setRoomId(const char* id) {
    if (strcmp(room, id) == 0) return true;
    prefs.begin(NS_CFG, false);
    prefs.putString("room", id);
    prefs.end();
    strlcpy(room, id, sizeof(room));
    return true;
}

void Storage::clearRoomId() {
    if (room[0] == 0) return;
    prefs.begin(NS_CFG, false);
    prefs.remove("room");
    prefs.end();
    room[0] = 0;
}

void Storage::setRestorePolicy(RestorePolicy p) {
    if (p == policy) return;
    prefs.begin(NS_CFG, false);
    prefs.putUChar("restore", (uint8_t)p);
    prefs.end();
    policy = p;
}

bool Storage::loadState(LightState& s, EffectParams& p) {
    Persisted ps;
    prefs.begin(NS_STATE, true);
    size_t n = prefs.getBytes("st", &ps, sizeof(ps));
    prefs.end();

    // Candle as a last mode in enum
    if (n != sizeof(ps) || ps.version != STATE_VERSION || ps.mode > (uint8_t)LightMode::Candle) return false;

    s.color = Rgb(ps.r, ps.g, ps.b);
    s.brightness = ps.brightness;
    s.power = ps.power != 0;
    s.mode = (LightMode)ps.mode;
    p.color = Rgb(ps.er, ps.eg, ps.eb);
    p.periodMs = ps.periodMs;
    p.startAt = 0;

    last = ps;
    haveLast = true;
    return true;
}

void Storage::markStateDirty() {
    dirty = true;
    dirtyAt = millis();
}

bool Storage::stateDueSave() const {
    return dirty && (millis() - dirtyAt >= SAVE_DELAY_MS);
}

void Storage::saveState(const LightState& s, const EffectParams& p) {
    Persisted ps;
    memset(&ps, 0, sizeof(ps));
    ps.version = STATE_VERSION;
    ps.power = s.power;
    ps.mode = (uint8_t)s.mode;
    ps.brightness = s.brightness;
    ps.r = s.color.r; ps.g = s.color.g; ps.b = s.color.b;
    ps.er = p.color.r; ps.eg = p.color.g; ps.eb = p.color.b;
    ps.periodMs = p.periodMs;

    dirty = false;
    if (haveLast && memcmp(&ps, &last, sizeof(ps)) == 0) return;

    prefs.begin(NS_STATE, false);
    prefs.putBytes("st", &ps, sizeof(ps));
    prefs.end();
    last = ps;
    haveLast = true;
}