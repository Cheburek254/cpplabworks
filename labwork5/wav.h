#pragma once
#include <vector>
#include <string>
#include <cstdint>

struct WavData {
    int sampleRate = 44100;
    int channels = 1;
    int bitsPerSample = 16;
    std::vector<float> samples; 
};

bool ReadWav(const std::string &path, WavData &out);
bool WriteWAV(const std::string &path, const WavData &in);
