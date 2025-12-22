#include "instruments.h"
#include "wav.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <vector>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <algorithm>
#include <string>

static std::vector<float> MakeSine(double freq, int lengthSamples, int sampleRate, float attack_s, float release_s, float velocity) {
    std::vector<float> out(lengthSamples, 0.0f);
    double phase = 0.0;
    double dt = 1.0 / sampleRate;
    for (int i=0;i<lengthSamples;++i) {
        double t = i * dt;
        out[i] = (float)(std::sin(2.0*M_PI*freq*t));
    }
    
    int attack_samples = int(attack_s * sampleRate + 0.5);
    int release_samples = int(release_s * sampleRate + 0.5);
    for (int i=0;i<lengthSamples;++i) {
        float env = 1.0f;
        if (attack_samples > 0 && i < attack_samples) env = (float)i / std::max(1,attack_samples);
        if (release_samples > 0 && i >= lengthSamples - release_samples) {
            int relpos = i - (lengthSamples - release_samples);
            env = env * (1.0f - (float)relpos / std::max(1,release_samples));
        }
        out[i] *= env * velocity;
    }
    return out;
}

static std::vector<float> MakeSquare(double freq, int lengthSamples, int sampleRate, int duty, float attack_s, float release_s, float velocity) {
    std::vector<float> out(lengthSamples, 0.0f);
    double dt = 1.0/sampleRate;
    for (int i=0;i<lengthSamples;++i) {
        double t = i*dt;
        double period = 1.0/freq;
        double pos = fmod(t, period) / period;
        float val = (pos < duty/100.0) ? 1.0f : -1.0f;
        out[i] = val;
    }
    int attack_samples = int(attack_s * sampleRate + 0.5);
    int release_samples = int(release_s * sampleRate + 0.5);
    for (int i=0;i<lengthSamples;++i) {
        float env = 1.0f;
        if (attack_samples > 0 && i < attack_samples) env = (float)i / std::max(1,attack_samples);
        if (release_samples > 0 && i >= lengthSamples - release_samples) {
            int relpos = i - (lengthSamples - release_samples);
            env = env * (1.0f - (float)relpos / std::max(1,release_samples));
        }
        out[i] *= env * velocity;
    }
    return out;
}

static std::vector<float> MakeTriangle(double freq, int lengthSamples, int sampleRate, float attack_s, float release_s, float velocity) {
    std::vector<float> out(lengthSamples,0.0f);
    double dt = 1.0/sampleRate;
    double period = 1.0/freq;
    for (int i=0;i<lengthSamples;++i) {
        double t = i*dt;
        double pos = fmod(t, period) / period; 
        double val = 0.0;
        if (pos < 0.25) val = pos * 4.0;
        else if (pos < 0.75) val = 2.0 - pos*4.0;
        else val = pos*4.0 - 4.0;
        out[i] = (float)val;
    }
    int attack_samples = int(attack_s * sampleRate + 0.5);
    int release_samples = int(release_s * sampleRate + 0.5);
    for (int i=0;i<lengthSamples;++i) {
        float env = 1.0f;
        if (attack_samples > 0 && i < attack_samples) env = (float)i / std::max(1,attack_samples);
        if (release_samples > 0 && i >= lengthSamples - release_samples) {
            int relpos = i - (lengthSamples - release_samples);
            env = env * (1.0f - (float)relpos / std::max(1,release_samples));
        }
        out[i] *= env * velocity;
    }
    return out;
}

class Sampler : public Instrument {
    WavData sample_data_;
    double root_freq_ = 440.0;
    int loop_start_ = -1, loop_end_ = -1; 
    float attack_ = 0.0f, release_ = 0.0f;
    bool loaded_ = false;
public:
    Sampler() = default;
    void SetParam(const std::string &k, const std::string &v) override {
        if (k == "sample") {
            
            if (!ReadWav(v, sample_data_)) {
                std::cerr << "Sampler: failed load " << v << std::endl;
            } else {
                loaded_ = true;
            }
        } else if (k == "root") {
            root_freq_ = NoteToFreq(v);
        } else if (k == "loop") {
            auto comma = v.find(',');
            if (comma != std::string::npos) {
                loop_start_ = std::stoi(v.substr(0,comma));
                loop_end_ = std::stoi(v.substr(comma+1));
            }
        } else if (k == "attack") {
            attack_ = std::stof(v);
        } else if (k == "release") {
            release_ = std::stof(v);
        }
    }

    std::vector<float> RenderNote(double freq, int noteLengthSamples, float velocity, int sampleRate) override {
        std::vector<float> out(noteLengthSamples, 0.0f);
        if (!loaded_ || sample_data_.samples.empty()) return out;
        double rate = freq / root_freq_;

        double inc = (double)sample_data_.sampleRate * rate / sampleRate;
        double src_pos = 0.0;

        int src_len = (int)sample_data_.samples.size();
        int idx = 0;
        if (loop_start_ >= 0 && loop_start_ < src_len) {
            while (idx < noteLengthSamples && src_pos < loop_start_) {
                int p = (int)floor(src_pos);
                int p2 = std::min(p+1, src_len-1);
                double frac = src_pos - p;
                float s = (1.0f-frac)*sample_data_.samples[p] + frac*sample_data_.samples[p2];
                out[idx++] = s * velocity;
                src_pos += inc;
            }

            if (loop_end_ <= loop_start_ || loop_end_ > src_len) {

                while (idx < noteLengthSamples && (int)src_pos < src_len) {
                    int p = (int)floor(src_pos);
                    int p2 = std::min(p+1, src_len-1);
                    double frac = src_pos - p;
                    float s = (1.0f-frac)*sample_data_.samples[p] + frac*sample_data_.samples[p2];
                    out[idx++] = s * velocity;
                    src_pos += inc;
                }
            } else {
                double loop_len = loop_end_ - loop_start_;
                double loop_pos = fmod(src_pos - loop_start_, loop_len);
                if (loop_pos < 0) loop_pos += loop_len;
                while (idx < noteLengthSamples) {
                    double s_pos = loop_start_ + loop_pos;
                    int p = (int)floor(s_pos);
                    int p2 = std::min(p+1, src_len-1);
                    double frac = s_pos - p;
                    float s = (1.0f-frac)*sample_data_.samples[p] + frac*sample_data_.samples[p2];
                    out[idx++] = s * velocity;
                    loop_pos += inc;
                    while (loop_pos >= loop_len) loop_pos -= loop_len;
                }
            }
        } else {

            while (idx < noteLengthSamples && (int)src_pos < src_len) {
                int p = (int)floor(src_pos);
                int p2 = std::min(p+1, src_len-1);
                double frac = src_pos - p;
                float s = (1.0f-frac)*sample_data_.samples[p] + frac*sample_data_.samples[p2];
                out[idx++] = s * velocity;
                src_pos += inc;
            }

        }

        int attack_samples = int(attack_ * sampleRate + 0.5);
        int release_samples = int(release_ * sampleRate + 0.5);
        for (int i=0;i<noteLengthSamples;++i) {
            float env = 1.0f;
            if (attack_samples>0 && i<attack_samples) env = (float)i / std::max(1,attack_samples);
            if (release_samples>0 && i >= noteLengthSamples - release_samples) {
                int relpos = i - (noteLengthSamples - release_samples);
                env = env * (1.0f - (float)relpos / std::max(1,release_samples));
            }
            out[i] *= env;
        }

        return out;
    }
};


class OscInstrument : public Instrument {
    std::string kind_ = "sine";
    float attack_ = 0.0f, release_ = 0.0f;
    int duty_ = 50;
public:
    OscInstrument(const std::string &k): kind_(k) {}
    void SetParam(const std::string &k, const std::string &v) override {
        if (k=="attack") attack_ = std::stof(v);
        else if (k=="release") release_ = std::stof(v);
        else if (k=="duty") duty_ = std::stoi(v);
    }
    std::vector<float> RenderNote(double freq, int noteLengthSamples, float velocity, int sampleRate) override {
        if (kind_=="sine") return MakeSine(freq, noteLengthSamples, sampleRate, attack_, release_, velocity);
        if (kind_=="square") return MakeSquare(freq, noteLengthSamples, sampleRate, duty_, attack_, release_, velocity);
        if (kind_=="triangle") return MakeTriangle(freq, noteLengthSamples, sampleRate, attack_, release_, velocity);
        return std::vector<float>(noteLengthSamples,0.0f);
    }
};

Instrument* MakeInstrument(const std::string &type) {
    if (type == "sampler") return new Sampler();
    if (type == "sine" || type == "square" || type == "triangle") return new OscInstrument(type);

    return new OscInstrument("sine");
}
