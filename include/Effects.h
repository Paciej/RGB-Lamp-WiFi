#ifndef EFFECTS_H
#define EFFECTS_H

#include "LightController.h"
#include "ModeCommand.h"

struct EffectParams {
    Rgb color;
    uint32_t periodMs;
    uint32_t startAt;
};

struct EffectScratch {
    uint32_t lastFlicker;
    uint32_t interval;
    int16_t target;
    int16_t current;
};

using EffectFn = Rgb (*)(const EffectParams&, uint32_t t, EffectScratch&);

struct EffectDef {
    const char* name;
    EffectFn fn;
    EffectParams defaults;
    bool usesColor;
    uint32_t minPeriod;
    uint32_t maxPeriod;
};

const char* modeToString(LightMode mode);
bool modeFromString(const char* s, LightMode& out);

uint32_t effectNowMs();

class EffectEngine {
private:
LightController& light;
LightMode active = LightMode::None;
EffectParams params{Rgb(), 100, 0};
EffectScratch scratch{0, 100, 255, 255};

void arm(LightMode m, const EffectParams& p);
public:
explicit EffectEngine(LightController& light) : light(light) {}

bool start(const ModeCommand& cmd);
bool start(LightMode mode, const EffectParams& params);
void update();

const EffectParams& currentParams() const { return params; }
};

#endif