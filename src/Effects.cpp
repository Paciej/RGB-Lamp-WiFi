#include "Effects.h"

/****** Effect functions ******/
static float phase01(const EffectParams& p, uint32_t t) {
    uint32_t period = p.periodMs ? p.periodMs : 1;
    return (float)((t - p.startAt) % period) / (float)period;
}

static Rgb waveFx(const EffectParams& p, uint32_t t, EffectScratch&) {
    float a = phase01(p, t) * TWO_PI;
    return Rgb(
        (uint8_t)((sin(a) + 1.f) * 127.5f),
        (uint8_t)((sin(a + 2.0944f) + 1.f) * 127.5f),
        (uint8_t)((sin(a + 4.1888f) + 1.f) * 127.5f)
    );
}

static Rgb pulseFx(const EffectParams& p, uint32_t t, EffectScratch&) {
    float a = phase01(p, t) * TWO_PI;
    float f = (sin(a) + 1.f) / 2.f; 

    return Rgb(p.color.r * f, p.color.g * f, p.color.b * f);
}

static Rgb candleFx(const EffectParams& p, uint32_t t, EffectScratch& s) {
    if (t - s.lastFlicker > s.interval) {
        s.lastFlicker = t;
        s.target = random(120, 256);
        s.interval = random(50, 150);
    }

    if (s.current < s.target) s.current += 5;
    if (s.current > s.target) s.current -= 5;
    s.current = constrain(s.current, 0, 255);
    int greenFlicker = random(-20, 20);

    return Rgb(
        (uint8_t)(s.current),
        (uint8_t)(constrain(((s.current * 140) / 255) + greenFlicker, 0, 255)),
        (uint8_t)(s.current * 10 / 255)
    );
}

static const EffectDef EFFECTS[] = {
    {"None", nullptr,   {Rgb(), 1000, 0}, false},
    {"Wave", waveFx,    {Rgb(), 6000, 0}, false},
    {"Pulse", pulseFx,  {Rgb(), 2000, 0}, true},
    {"Candle", candleFx,{Rgb(), 1000, 0}, false}
};
static_assert(sizeof(EFFECTS) / sizeof(EFFECTS[0]) == 4, "EFFECTS must match LightMode enum");

static const EffectDef& defOf(LightMode m) { return EFFECTS[(uint8_t)m]; }

const char* modeToString(LightMode m) { return defOf(m).name; }

bool modeFromString(const char* s, LightMode& out) {
    for (uint8_t i = 0; i < sizeof(EFFECTS) / sizeof(EFFECTS[0]); i++) {
        if (strcmp(s, EFFECTS[i].name) == 0) {
        out = (LightMode)i;
        return true;
        }
    }
    return false;
}

uint32_t effectNowMs() { return millis(); }

/******* EFFECT ENGINE *******/
void EffectEngine::arm(LightMode m, const EffectParams& p) {
    this->active = m;
    this->params = p;
    this->scratch = EffectScratch{0, 100, 255, 255};
}

bool EffectEngine::start(LightMode mode) {
    EffectParams p = defOf(mode).defaults;
    p.startAt = effectNowMs();
    if (defOf(mode).usesColor) p.color = light.state().color;
    return start(mode, p);
}

bool EffectEngine::start(LightMode mode, const EffectParams& params) {
    light.setMode(mode);
    if (light.state().mode != mode) return false;
    arm(mode, params);
    return true;
}

void EffectEngine::update() {
    LightMode m = light.state().mode;

    if (m != active) {
        if (m == LightMode::None) { active = m; return; }
        EffectParams p = defOf(m).defaults;
        p.startAt = effectNowMs();
    }

    if (active == LightMode::None) return;
    light.setEffectColor(defOf(active).fn(this->params, effectNowMs(), this->scratch));
}
