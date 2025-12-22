#pragma once
#include <vector>
#include <string>
#include <map>
#include "wav.h"
#include "effects.h"


class Instrument {
public:
    virtual ~Instrument() = default;
    virtual std::vector<float> RenderNote(double freq, int noteLengthSamples, float velocity, int sampleRate) = 0;

    virtual void SetParam(const std::string &k, const std::string &v) {}
    void SetEffect(const std::vector<Effect*> &eff) { effects = eff; }
    std::vector<Effect*> effects;
};

Instrument* MakeInstrument(const std::string &type);
