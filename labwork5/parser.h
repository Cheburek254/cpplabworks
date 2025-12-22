#pragma once
#include <string>
#include <vector>
#include <map>

struct NoteEvent {
    int start_unit = 0;
    std::string instrument;
    std::string pitch;
    int duration_units = 0;
    float velocity = 1.0f; 
};

struct Pattern {
    std::string name;
    int resolution = 1;
    std::vector<std::string> raw_lines; 

    std::vector<NoteEvent> notes;
    struct PatternCall { int start_unit; std::string pattern_name; };
    std::vector<PatternCall> calls;
};

struct InstrumentDef {
    std::string name;
    std::string type;
    std::map<std::string,std::string> params;

    std::vector<std::string> effects_raw;
};

struct Song {
    int bpm = 120;
    std::map<std::string, InstrumentDef> instruments;
    std::map<std::string, Pattern> patterns;
};

bool ParseScoreFile(const std::string &path, Song &out, std::string &err);
