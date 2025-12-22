#include "effects.h"
#include "utils.h"
#include <cmath>
#include <vector>
#include <cstddef>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <string>
class GainEffect : public Effect {
    float gain_;
public:
    GainEffect(float g): gain_(g){}
    void Process(std::vector<float> &samples, int) override {
        for (auto &s : samples) s *= gain_;
    }
};


class EchoEffect : public Effect {
    float delay_s_;
    float decay_;
public:
    EchoEffect(float d, float dec): delay_s_(d), decay_(dec) {}
    void Process(std::vector<float> &samples, int sampleRate) override {
        int delay_samples = int(delay_s_ * sampleRate + 0.5f);
        if (delay_samples <= 0) return;
        std::vector<float> buf(samples.size(), 0.0f);
        for (size_t i=0;i<samples.size();++i) {
            float dry = samples[i];
            float wet = 0.0f;
            if ((int)i - delay_samples >= 0) wet = buf[i - delay_samples] * decay_;
            buf[i] = dry + wet;
        }
        samples.swap(buf);
    }
};

// y(t) = x(t)*(1 - depth + depth * sin(2*pi*freq*t))
class TremoloEffect : public Effect {
    float freq_;
    float depth_;
public:
    TremoloEffect(float f, float d): freq_(f), depth_(d) {}
    void Process(std::vector<float> &samples, int sampleRate) override {
        for (size_t i=0;i<samples.size();++i) {
            double t = double(i) / sampleRate;
            float m = 1.0f - depth_ + depth_ * std::sin(2.0*M_PI*freq_*t);
            samples[i] *= m;
        }
    }
};

Effect* MakeEffectFromString(const std::string &raw) {
    
    std::string s = Trim(raw);
    auto parts = SplitWS(s);
    if (parts.empty()) return nullptr;
    std::string type = parts[0];
    if (type == "gain") {
        float g = 1.0f;
        
        if (parts.size() >= 2) {
            auto p = parts[1];
            auto eq = p.find('=');
            if (eq!=std::string::npos) p = p.substr(eq+1);
            g = std::stof(p);
        }
        return new GainEffect(g);
    } else if (type == "echo") {
        float delay = 0.3f, decay = 0.3f;
        for (size_t i=1;i<parts.size();++i) {
            auto kv = parts[i]; auto eq = kv.find('='); if (eq==std::string::npos) continue;
            std::string k = kv.substr(0,eq), v = kv.substr(eq+1);
            if (k=="delay") delay = std::stof(v);
            if (k=="decay") decay = std::stof(v);
        }
        return new EchoEffect(delay, decay);
    } else if (type == "tremolo") {
        float freq = 10.0f, depth = 0.5f;
        for (size_t i=1;i<parts.size();++i) {
            auto kv = parts[i]; auto eq = kv.find('='); if (eq==std::string::npos) continue;
            std::string k = kv.substr(0,eq), v = kv.substr(eq+1);
            if (k=="freq") freq = std::stof(v);
            if (k=="depth") depth = std::stof(v);
        }
        return new TremoloEffect(freq, depth);
    }
    return nullptr;
}
