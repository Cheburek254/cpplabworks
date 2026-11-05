#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <tuple>

constexpr int kSampleRate = 44100;
constexpr double kPi = 3.14159265358979323846;

#pragma pack(push, 1)
struct WavHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t overall_size;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt_chunk_marker[4] = {'f', 'm', 't', ' '};
    uint32_t length_of_fmt = 16;
    uint16_t format_type = 1;
    uint16_t channels = 1;
    uint32_t sample_rate = kSampleRate;
    uint32_t byterate = kSampleRate * 2;
    uint16_t block_align = 2;
    uint16_t bits_per_sample = 16;
    char data_chunk_header[4] = {'d', 'a', 't', 'a'};
    uint32_t data_size;
};
#pragma pack(pop)

struct NoteCommand {
    std::string instrument;
    std::string pitch;
    int duration;
    int velocity;
    int start_unit;
};

struct Pattern {
    int resolution = 4;
    std::vector<NoteCommand> notes;
    std::vector<std::pair<int, std::string>> pattern_refs;
};

struct RenderEvent {
    size_t start_sample;
    std::string inst_name;
    double freq;
    double duration_sec;
    double velocity;
};

class AudioBuffer {
 public:
    std::vector<double> samples;

    void Resize(size_t size) {
        if (size > samples.size()) {
            samples.resize(size, 0.0);
        }
    }

    void Add(size_t index, double value) {
        if (index < samples.size()) {
            samples[index] += value;
        }
    }

    size_t Size() const { return samples.size(); }
    double& operator[](size_t i) { return samples[i]; }
    const double& operator[](size_t i) const { return samples[i]; }
};

#endif
