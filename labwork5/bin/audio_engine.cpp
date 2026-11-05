#include "audio_engine.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include "types.h"


double NoteToFreq(const std::string& note) {
    if (note == "rest") return 0.0;

    static std::map<std::string, double> base_freqs = {
        {"C", 16.35}, {"C#", 17.32}, {"D", 18.35}, {"D#", 19.45},
        {"E", 20.60}, {"F", 21.83}, {"F#", 23.12}, {"G", 24.50},
        {"G#", 25.96}, {"A", 27.50}, {"A#", 29.14}, {"B", 30.87}
    };

    std::string name;
    int octave = 4;
    size_t digit_pos = note.find_first_of("0123456789-");
    if (digit_pos != std::string::npos) {
        name = note.substr(0, digit_pos);
        octave = std::stoi(note.substr(digit_pos));
    } else {
        name = note;
    }

    if (base_freqs.find(name) == base_freqs.end()) return 440.0;
    return base_freqs[name] * std::pow(2.0, octave);
}


GainEffect::GainEffect(double gain) : gain_(gain) {}

void GainEffect::Apply(AudioBuffer& buffer) {
    for (auto& s : buffer.samples) {
        s *= gain_;
    }
}

EchoEffect::EchoEffect(double delay, double decay) 
    : delay_(delay), decay_(decay) {}

void EchoEffect::Apply(AudioBuffer& buffer) {
    size_t delay_samples = static_cast<size_t>(delay_ * kSampleRate);
    std::vector<double> original = buffer.samples;

    if (buffer.Size() + delay_samples > buffer.Size()) {
        buffer.Resize(buffer.Size() + delay_samples);
    }

    for (size_t i = 0; i < original.size(); ++i) {
        size_t echo_pos = i + delay_samples;
        if (echo_pos < buffer.Size()) {
            buffer.samples[echo_pos] += original[i] * decay_;
        }
    }
}

TremoloEffect::TremoloEffect(double freq, double depth) 
    : freq_(freq), depth_(depth) {}

void TremoloEffect::Apply(AudioBuffer& buffer) {
    for (size_t i = 0; i < buffer.Size(); ++i) {
        double t = static_cast<double>(i) / kSampleRate;
        double mod = 1.0 - depth_ + depth_ * std::sin(2.0 * kPi * freq_ * t);
        buffer.samples[i] *= mod;
    }
}


void Instrument::SetParams(const std::map<std::string, std::string>& params) {
    if (params.count("attack")) attack_ = std::stod(params.at("attack"));
    if (params.count("release")) release_ = std::stod(params.at("release"));
    ParseSpecificParams(params);
}

void Instrument::AddEffect(std::shared_ptr<Effect> effect) {
    effects_.push_back(effect);
}

void Instrument::RenderNotes(
    AudioBuffer* track_buffer,
    const std::vector<std::tuple<size_t, double, double, double>>& notes) {
    for (const auto& note : notes) {
        auto [start_sample, freq, duration_sec, velocity] = note;

        std::vector<double> voice = GenerateVoice(freq, duration_sec, velocity);

        if (track_buffer->Size() < start_sample + voice.size()) {
            track_buffer->Resize(start_sample + voice.size());
        }

        for (size_t i = 0; i < voice.size(); ++i) {
            track_buffer->Add(start_sample + i, voice[i]);
        }
    }

    for (auto& effect : effects_) {
        effect->Apply(*track_buffer);
    }
}

void Instrument::ApplyEnvelope(std::vector<double>* voice,
                               double duration_samples) {
    size_t total_samples = voice->size();
    size_t attack_samples = static_cast<size_t>(attack_ * kSampleRate);
    size_t release_samples = static_cast<size_t>(release_ * kSampleRate);

    for (size_t i = 0; i < total_samples; ++i) {
        double env = 1.0;

        if (i < attack_samples) {
            env = static_cast<double>(i) / attack_samples;
        }

        size_t sustain_end = total_samples - release_samples;
        if (i >= sustain_end) {
            double rel_progress = static_cast<double>(i - sustain_end) / release_samples;
            env = std::max(0.0, 1.0 - rel_progress);

            if (i < attack_samples) {
                env *= (static_cast<double>(i) / attack_samples);
            }
        }

        (*voice)[i] *= env;
    }
}


Oscillator::Oscillator(Type type) : type_(type) {}

void Oscillator::ParseSpecificParams(
    const std::map<std::string, std::string>& params) {
    if (type_ == kSquare && params.count("duty")) {
        duty_ = std::stod(params.at("duty")) / 100.0;
    }
}

std::vector<double> Oscillator::GenerateVoice(double freq, double duration_sec,
                                              double velocity) {
    size_t len = static_cast<size_t>((duration_sec + release_) * kSampleRate);
    std::vector<double> voice(len);
    double phase = 0.0;
    double phase_step = freq / kSampleRate;

    for (size_t i = 0; i < len; ++i) {
        double sample = 0.0;

        switch (type_) {
            case kSine:
                sample = std::sin(2.0 * kPi * phase);
                break;
            case kTriangle:
                sample = 2.0 * std::abs(2.0 * (phase - std::floor(phase + 0.5))) - 1.0;
                break;
            case kSquare:
                double t = phase - std::floor(phase);
                sample = (t < duty_) ? 1.0 : -1.0;
                break;
        }

        voice[i] = sample * (velocity / 100.0);
        phase += phase_step;
    }

    ApplyEnvelope(&voice, duration_sec * kSampleRate);
    return voice;
}


void Sampler::ParseSpecificParams(
    const std::map<std::string, std::string>& params) {
    if (params.count("sample")) LoadWav(params.at("sample"));
    if (params.count("root")) root_freq_ = NoteToFreq(params.at("root"));
    if (params.count("loop")) {
        std::string l = params.at("loop");
        size_t comma = l.find(',');
        if (comma != std::string::npos) {
            loop_start_ = std::stoul(l.substr(0, comma));
            loop_end_ = std::stoul(l.substr(comma + 1));
            has_loop_ = true;
        }
    }
}

void Sampler::LoadWav(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open sample " << path 
                  << ". Playing silence." << std::endl;
        return;
    }

    WavHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));

    int bytes_per_sample = header.bits_per_sample / 8;
    size_t num_samples = header.data_size / bytes_per_sample;
    source_data_.resize(num_samples);

    for (size_t i = 0; i < num_samples; ++i) {
        int16_t sample16;
        file.read(reinterpret_cast<char*>(&sample16), sizeof(int16_t));
        source_data_[i] = sample16 / 32768.0;
    }
}

std::vector<double> Sampler::GenerateVoice(double freq, double duration_sec,
                                           double velocity) {
    size_t out_len = static_cast<size_t>((duration_sec + release_) * kSampleRate);
    std::vector<double> voice(out_len);

    if (source_data_.empty()) return voice;

    double rate_ratio = freq / root_freq_;
    double pos = 0.0;

    for (size_t i = 0; i < out_len; ++i) {
        if (!has_loop_ && pos >= source_data_.size() - 1) break;

        if (has_loop_ && pos >= loop_end_) {
            pos = loop_start_ + (pos - loop_end_);
        }

        size_t idx = static_cast<size_t>(pos);
        double frac = pos - idx;
        double s1 = source_data_[idx % source_data_.size()];
        double s2 = source_data_[(idx + 1) % source_data_.size()];

        double val = s1 + frac * (s2 - s1);
        voice[i] = val * (velocity / 100.0);

        pos += rate_ratio;
    }

    ApplyEnvelope(&voice, duration_sec * kSampleRate);
    return voice;
}
