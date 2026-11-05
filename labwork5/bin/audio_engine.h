#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "types.h"

double NoteToFreq(const std::string& note);

class Effect {
 public:
    virtual ~Effect() = default;
    virtual void Apply(AudioBuffer& buffer) = 0;
};

class GainEffect : public Effect {
 public:
    explicit GainEffect(double gain);
    void Apply(AudioBuffer& buffer) override;
 private:
    double gain_;
};

class EchoEffect : public Effect {
 public:
    EchoEffect(double delay, double decay);
    void Apply(AudioBuffer& buffer) override;
 private:
    double delay_;
    double decay_;
};

class TremoloEffect : public Effect {
 public:
    TremoloEffect(double freq, double depth);
    void Apply(AudioBuffer& buffer) override;
 private:
    double freq_;
    double depth_;
};

class Instrument {
 public:
    virtual ~Instrument() = default;
    
    void SetParams(const std::map<std::string, std::string>& params);
    void AddEffect(std::shared_ptr<Effect> effect);
    void RenderNotes(AudioBuffer* track_buffer,
                     const std::vector<std::tuple<size_t, double, double, double>>& notes);

    virtual void ParseSpecificParams(const std::map<std::string, std::string>& params) {}
    virtual std::vector<double> GenerateVoice(double freq, double duration_sec,
                                              double velocity) = 0;

 protected:
    void ApplyEnvelope(std::vector<double>* voice, double duration_samples);

    double attack_ = 0.0;
    double release_ = 0.0;
    std::vector<std::shared_ptr<Effect>> effects_;
};

class Oscillator : public Instrument {
 public:
    enum Type { kSine, kTriangle, kSquare };

    explicit Oscillator(Type type);
    void ParseSpecificParams(const std::map<std::string, std::string>& params) override;
    std::vector<double> GenerateVoice(double freq, double duration_sec,
                                      double velocity) override;
 private:
    Type type_;
    double duty_ = 0.5;
};

class Sampler : public Instrument {
 public:
    void ParseSpecificParams(const std::map<std::string, std::string>& params) override;
    std::vector<double> GenerateVoice(double freq, double duration_sec,
                                      double velocity) override;
 private:
    void LoadWav(const std::string& path);

    std::vector<double> source_data_;
    double root_freq_ = 440.0;
    size_t loop_start_ = 0;
    size_t loop_end_ = 0;
    bool has_loop_ = false;
};

#endif
