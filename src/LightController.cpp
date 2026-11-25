#include "LightController.h"

/****** Private methods ******/

Rgb LightController::computeOutput() const {
    if (!_state.power) return Rgb{0, 0, 0};
    const Rgb src = effectActive ? effectColor : _state.color;
    auto scale = [&](uint8_t v) -> uint8_t {
        uint8_t out = (uint16_t)v * _state.brightness / 255;
        return out > outputLimit ? outputLimit : out;
    };

    return Rgb{scale(src.r), scale(src.g), scale(src.b)};
}

void LightController::writePwm(const Rgb& c) {
    ledcWrite(_pins.chRed, c.r);
    ledcWrite(_pins.chGreen, c.g);
    ledcWrite(_pins.chBlue, c.b);
}

/****** Public methods ******/

void LightController::begin() {
    ledcSetup(_pins.chRed, _pins.freq, _pins.resolution);
    ledcSetup(_pins.chGreen, _pins.freq, _pins.resolution);
    ledcSetup(_pins.chBlue, _pins.freq, _pins.resolution);
    ledcAttachPin(_pins.red, _pins.chRed);
    ledcAttachPin(_pins.green, _pins.chGreen);
    ledcAttachPin(_pins.blue, _pins.chBlue);
    this->outputDirty = true;
    update();
}

void LightController::update() {
    if (!outputDirty) return;
    writePwm(computeOutput());
    outputDirty = false;
}

void LightController::setColor(Rgb color) {
    if (!_state.power) return;
    _state.color = color;
    if (_state.mode != LightMode::None) {
        _state.mode == LightMode::None;
        changes |= CH_MODE;
    }
    effectActive = false;
    changes |= CH_COLOR;
    outputDirty = true;
}

void LightController::setBrightness(uint8_t brightness) {
    if (!_state.power) return;
    _state.brightness = brightness;
    changes |= CH_BRIGHTNESS;
    outputDirty = true;
}

void LightController::setPower(bool on) {
    _state.power = on;
    changes |= CH_POWER;
    outputDirty = true;
}

void LightController::setMode(LightMode mode) {
    if (!_state.power) return;
    _state.mode = mode;
    if (_state.mode == LightMode::None) effectActive = false;
    changes |= CH_MODE;
    outputDirty = true;
}

void LightController::setEffectColor(Rgb color) {
    if (effectActive && color.r == effectColor.r && color.g == effectColor.g && color.b == effectColor.b) return;
    effectColor = color;
    effectActive = true;
    outputDirty = true;
}

void LightController::setOutputLimit(uint8_t limit) {
    if (outputLimit == limit || limit > 255 || limit < 0) return;
    outputLimit = limit;
    outputDirty = true;
}

uint8_t LightController::takeChanges() {
    uint8_t c = changes;
    changes = 0;
    return c;
}

void LightController::restore(const LightState& st) {
    this->_state = st;
    effectActive = false;
    outputDirty = true;
}