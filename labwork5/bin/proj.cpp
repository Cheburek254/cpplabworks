#include "proj.h"


#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <tuple>
#include "audio_engine.h"
#include "types.h"



void Project::LoadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) throw std::runtime_error("Cannot open file");

    std::string line;
    std::string context = "GLOBAL";
    std::string current_obj_name;

    while (std::getline(file, line)) {
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }

        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;
        if (cmd.empty()) continue;

        if (cmd == "bpm") {
            ss >> bpm;
        } else if (cmd == "instrument") {
            std::string type;
            ss >> current_obj_name >> type;

            if (type == "sampler") {
                instruments[current_obj_name] = std::make_shared<Sampler>();
            } else if (type == "square") {
                instruments[current_obj_name] = 
                    std::make_shared<Oscillator>(Oscillator::kSquare);
            } else if (type == "sine") {
                instruments[current_obj_name] = 
                    std::make_shared<Oscillator>(Oscillator::kSine);
            } else if (type == "triangle") {
                instruments[current_obj_name] = 
                    std::make_shared<Oscillator>(Oscillator::kTriangle);
            }

            context = "INSTRUMENT";
        } else if (cmd == "pattern") {
            ss >> current_obj_name;
            std::string res_kw;
            ss >> res_kw;
            if (res_kw == "resolution") {
                ss >> patterns[current_obj_name].resolution;
            } else {
                patterns[current_obj_name].resolution = std::stoi(res_kw);
            }
            context = "PATTERN";
        } else if (cmd == "end") {
            context = "GLOBAL";
        } else {
            if (context == "INSTRUMENT") {
                if (cmd == "effect") {
                    std::string eff_type;
                    ss >> eff_type;
                    if (eff_type == "gain") {
                        std::string p;
                        double g = 1.0;
                        while (ss >> p) {
                            if (p.find("gain=") == 0) g = std::stod(p.substr(5));
                        }
                        instruments[current_obj_name]->AddEffect(
                            std::make_shared<GainEffect>(g));
                    } else if (eff_type == "echo") {
                        std::string p;
                        double dl = 0.0, dc = 0.0;
                        while (ss >> p) {
                            if (p.find("delay=") == 0) dl = std::stod(p.substr(6));
                            if (p.find("decay=") == 0) dc = std::stod(p.substr(6));
                        }
                        instruments[current_obj_name]->AddEffect(
                            std::make_shared<EchoEffect>(dl, dc));
                    } else if (eff_type == "tremolo") {
                        std::string p;
                        double f = 0.0, d = 0.0;
                        while (ss >> p) {
                            if (p.find("freq=") == 0) f = std::stod(p.substr(5));
                            if (p.find("depth=") == 0) d = std::stod(p.substr(6));
                        }
                        instruments[current_obj_name]->AddEffect(
                            std::make_shared<TremoloEffect>(f, d));
                    }
                } else {
                    std::map<std::string, std::string> params;
                    auto parse_kv = [&](std::string s) {
                        size_t eq = s.find('=');
                        if (eq != std::string::npos) {
                            params[s.substr(0, eq)] = s.substr(eq + 1);
                        }
                    };
                    parse_kv(cmd);
                    std::string temp;
                    while (ss >> temp) parse_kv(temp);
                    instruments[current_obj_name]->SetParams(params);
                }
            } else if (context == "PATTERN") {
                int time = std::stoi(cmd);
                std::string arg1;
                ss >> arg1;
                if (!arg1.empty() && arg1[0] == '@') {
                    patterns[current_obj_name].pattern_refs.push_back(
                        {time, arg1.substr(1)});
                } else {
                    NoteCommand nc;
                    nc.start_unit = time;
                    nc.instrument = arg1;
                    ss >> nc.pitch >> nc.duration >> nc.velocity;
                    patterns[current_obj_name].notes.push_back(nc);
                }
            }
        }
    }
}

void Project::ProcessPattern(const std::string& pat_name, double start_time_sec) {
    if (patterns.find(pat_name) == patterns.end()) return;
    const auto& pat = patterns.at(pat_name);

    double sec_per_beat = 60.0 / bpm;
    double sec_per_unit = sec_per_beat / pat.resolution;

    for (const auto& note : pat.notes) {
        RenderEvent ev;
        ev.start_sample = static_cast<size_t>(
            (start_time_sec + note.start_unit * sec_per_unit) * kSampleRate);
        ev.inst_name = note.instrument;
        ev.freq = NoteToFreq(note.pitch);
        ev.duration_sec = note.duration * sec_per_unit;
        ev.velocity = note.velocity;
        event_list.push_back(ev);
    }

    for (const auto& ref : pat.pattern_refs) {
        ProcessPattern(ref.second, start_time_sec + ref.first * sec_per_unit);
    }
}

void Project::WriteWav(const std::string& out_file, const AudioBuffer& buffer) {
    std::ofstream file(out_file, std::ios::binary);
    WavHeader header;
    header.data_size = buffer.Size() * 2;
    header.overall_size = header.data_size + 36;

    file.write(reinterpret_cast<char*>(&header), sizeof(WavHeader));

    for (double s : buffer.samples) {
        if (s > 1.0) s = 1.0;
        if (s < -1.0) s = -1.0;
        int16_t val = static_cast<int16_t>(s * 32767.0);
        file.write(reinterpret_cast<char*>(&val), sizeof(int16_t));
    }
}

void Project::Render(const std::string& out_file) {
    event_list.clear();
    ProcessPattern("main", 0.0);

    std::map<std::string, std::vector<std::tuple<size_t, double, double, double>>> inst_notes;
    size_t max_sample = 0;

    for (const auto& ev : event_list) {
        inst_notes[ev.inst_name].emplace_back(
            ev.start_sample, ev.freq, ev.duration_sec, ev.velocity);
        size_t end = ev.start_sample + 
                     static_cast<size_t>(ev.duration_sec * kSampleRate) + kSampleRate;
        if (end > max_sample) max_sample = end;
    }

    AudioBuffer master_buffer;

    for (auto& [name, notes] : inst_notes) {
        if (instruments.find(name) == instruments.end()) continue;

        AudioBuffer inst_buffer;
        instruments[name]->RenderNotes(&inst_buffer, notes);

        if (master_buffer.Size() < inst_buffer.Size()) {
            master_buffer.Resize(inst_buffer.Size());
        }
        for (size_t i = 0; i < inst_buffer.Size(); ++i) {
            master_buffer[i] += inst_buffer[i];
        }
    }

    WriteWav(out_file, master_buffer);

    std::cout << "Soundcloud rap god done this greatest music "
              << master_buffer.Size() / kSampleRate 
              << " SECONDS BROOOOO" << std::endl;
}
