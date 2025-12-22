#pragma once
#include <vector>
#include <string>

struct Effect {
    virtual ~Effect() = default;
    
    virtual void Process(std::vector<float> &samples, int sampleRate) = 0;
};

Effect* MakeEffectFromString(const std::string &raw);
