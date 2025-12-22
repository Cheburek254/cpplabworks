#include "wav.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cmath>
#include <string>
#include <cstdint>
#include <vector>
static uint32_t ReadU32(std::ifstream &ifs) {
    uint32_t v; ifs.read(reinterpret_cast<char*>(&v), 4); return v;
}
static uint16_t ReadU16(std::ifstream &ifs) {
    uint16_t v; ifs.read(reinterpret_cast<char*>(&v), 2); return v;
}

bool ReadWav(const std::string &path, WavData &out) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) { std::cerr << "Failed open WAV: " << path << "\n"; return false; }
    char riff[4]; ifs.read(riff,4);
    if (std::strncmp(riff,"RIFF",4)!=0) { std::cerr<<"Not RIFF\n"; return false; }
    ReadU32(ifs); 
    char wave[4]; ifs.read(wave,4);
    if (std::strncmp(wave,"WAVE",4)!=0) { std::cerr<<"Not WAVE\n"; return false; }

    uint16_t audio_format=0, num_channels=0, bps=0;
    uint32_t sample_rate=44100;
    std::vector<char> data_chunk;

    while (ifs) {
        char id[4]; ifs.read(id,4);
        if (!ifs) break;
        uint32_t chunk_size = ReadU32(ifs);
        std::string sid(id,4);
        if (sid == "fmt ") {
            audio_format = ReadU16(ifs);
            num_channels = ReadU16(ifs);
            sample_rate = ReadU32(ifs);
            ReadU32(ifs); 
            ReadU16(ifs); 
            bps = ReadU16(ifs);

            if (chunk_size > 16) {
                ifs.seekg(chunk_size - 16, std::ios::cur);
            }
        } else if (sid == "data") {
            data_chunk.resize(chunk_size);
            ifs.read(data_chunk.data(), chunk_size);
        } else {

            ifs.seekg(chunk_size, std::ios::cur);
        }
    }

    if (data_chunk.empty()) { std::cerr<<"No data chunk\n"; return false; }
    if (audio_format != 1) { std::cerr<<"Only PCM supported\n"; return false; }
    if (bps != 16) { std::cerr<<"Only 16-bit supported\n"; return false; }

    out.sampleRate = (int)sample_rate;
    out.channels = (int)num_channels;
    out.bitsPerSample = (int)bps;
    out.samples.clear();

    const int16_t *ptr = reinterpret_cast<const int16_t*>(data_chunk.data());
    size_t frames = data_chunk.size() / (num_channels * 2);
    out.samples.reserve(frames);
    for (size_t i=0;i<frames;i++) {
        float v = 0.0f;
        for (int ch=0; ch<num_channels; ++ch) {
            int16_t s = ptr[i * num_channels + ch];
            v += float(s) / 32768.0f;
        }
        v /= (float)num_channels;
        out.samples.push_back(v);
    }
    return true;
}

bool WriteWAV(const std::string &path, const WavData &in) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return false;
    int sample_rate = in.sampleRate;
    int bps = in.bitsPerSample;
    int channels = 1;
    int byte_rate = sample_rate * channels * bps / 8;
    int block_align = channels * bps / 8;
    int data_bytes = (int)in.samples.size() * channels * bps / 8;
    int fmt_cs = 16;
    int riff_size = 4 + (8 + fmt_cs) + (8 + data_bytes);

    ofs.write("RIFF",4);
    uint32_t tmp32 = riff_size;
    ofs.write(reinterpret_cast<const char*>(&tmp32),4);
    ofs.write("WAVE",4);


    ofs.write("fmt ",4);
    tmp32 = fmt_cs; ofs.write(reinterpret_cast<const char*>(&tmp32),4);
    uint16_t tmp16 = 1; ofs.write(reinterpret_cast<const char*>(&tmp16),2); // PCM
    tmp16 = channels; ofs.write(reinterpret_cast<const char*>(&tmp16),2);
    tmp32 = sample_rate; ofs.write(reinterpret_cast<const char*>(&tmp32),4);
    tmp32 = byte_rate; ofs.write(reinterpret_cast<const char*>(&tmp32),4);
    tmp16 = block_align; ofs.write(reinterpret_cast<const char*>(&tmp16),2);
    tmp16 = bps; ofs.write(reinterpret_cast<const char*>(&tmp16),2);

    ofs.write("data",4);
    tmp32 = data_bytes; ofs.write(reinterpret_cast<const char*>(&tmp32),4);
    for (size_t i=0;i<in.samples.size();++i) {
        float v = in.samples[i];
        if (v > 1.0f) v = 1.0f;
        if (v < -1.0f) v = -1.0f;
        int16_t s = (int16_t)std::lrint(v * 32767.0f);
        ofs.write(reinterpret_cast<const char*>(&s), sizeof(s));
    }
    return true;
}
