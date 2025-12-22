#include "engine.h"
#include "parser.h"
#include "utils.h"
#include <iostream>
#include <cmath>
#include <vector>
#include "instruments.h"
#include "effects.h"
#include "wav.h"
#include <string>
#include <cstddef>
#include <algorithm>
#include <utility>

static void ExpandPattern(const Song &song_, const Pattern &p, int base_unit, std::vector<NoteEvent> &out) {
    
    for (auto n : p.notes) {
        NoteEvent m = n;
        m.start_unit += base_unit;
        out.push_back(m);
    }

    for (auto c : p.calls) {
        auto it = song_.patterns.find(c.pattern_name);
        if (it == song_.patterns.end()) {
            std::cerr << "Pattern " << c.pattern_name << " not found\n";
            continue;
        }

        ExpandPattern(song_, it->second, base_unit + c.start_unit, out);
    }
}

Engine::Engine(const Song &s) : song_(s) {
    for (auto &kv : song_.instruments) {
        Instrument* inst = MakeInstrument(kv.second.type);

        for (auto &p : kv.second.params) inst->SetParam(p.first, p.second);

        std::vector<Effect*> effs;
        for (auto &er : kv.second.effects_raw) {
            Effect* e = MakeEffectFromString(er);
            if (e) effs.push_back(e);
        }
        inst->SetEffect(effs);
        instrument_objs_[kv.first] = inst;
    }
}

Engine::~Engine() {
    for (auto &kv : instrument_objs_) {
        for (auto e : kv.second->effects) delete e;
        delete kv.second;
    }
}

bool Engine::Render(WavData &out, const RenderOptions &opt, std::string &err) {
    int sample_rate = opt.sampleRate;
    auto it = song_.patterns.find("main");
    if (it == song_.patterns.end()) { err="main pattern missing"; return false; }
    std::vector<NoteEvent> events;
    ExpandPattern(song_, it->second, 0, events);

    int main_resolution = it->second.resolution;
    double beat_sec = 60.0 / song_.bpm;
    double max_end = 0.0;
    struct Ev2 { NoteEvent ev; double start_sec; double dur_sec; int resolution; };
    std::vector<Ev2> ev2s;
    for (auto &e : events) {
        double unit_sec = beat_sec / main_resolution;
        double start_sec = e.start_unit * unit_sec;
        double dur_sec = e.duration_units * unit_sec;
        if (dur_sec < 0.0) dur_sec = unit_sec;
        if (start_sec + dur_sec > max_end) max_end = start_sec + dur_sec;
        ev2s.push_back({e, start_sec, dur_sec, main_resolution});
    }
    int total_samples = int(std::ceil(max_end * sample_rate)) + sample_rate; 
    std::vector<float> mix(total_samples, 0.0f);

    
    for (auto &e2 : ev2s) {
        auto &ne = e2.ev;
        auto inst_it = instrument_objs_.find(ne.instrument);
        if (inst_it == instrument_objs_.end()) {
            std::cerr << "Unknown instrument " << ne.instrument << "\n";
            continue;
        }
        Instrument* inst = inst_it->second;
        double freq = NoteToFreq(ne.pitch);
        int note_len_samples = int(std::round(e2.dur_sec * sample_rate));
        if (note_len_samples <= 0) continue;
        auto voice = inst->RenderNote(freq, note_len_samples, ne.velocity, sample_rate);
        for (auto eff : inst->effects) {
            eff->Process(voice, sample_rate);
        }
        int start_sample = int(std::round(e2.start_sec * sample_rate));
        if (start_sample < 0) start_sample = 0;
        for (size_t k=0;k<voice.size();++k) {
            int idx = start_sample + (int)k;
            if (idx >= 0 && idx < (int)mix.size()) mix[idx] += voice[k];
        }
    }

    float peak = 0.0f;
    for (auto v : mix) peak = std::max(peak, std::fabs(v));
    if (peak > 1.0f) {
        for (auto &v : mix) v /= peak;
    }

    out.sampleRate = sample_rate;
    out.channels = 1;
    out.bitsPerSample = 16;
    out.samples = std::move(mix);
    return true;
}
