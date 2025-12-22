#pragma once
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>

inline std::string Trim(const std::string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a==std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b-a+1);
}

inline std::vector<std::string> SplitWS(const std::string &s) {
    std::istringstream iss(s);
    std::vector<std::string> out;
    std::string w;
    while (iss >> w) out.push_back(w);
    return out;
}

inline std::string ToUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::toupper(c); });
    return s;
}


inline double NoteToFreq(const std::string &note) {

    static std::map<std::string,int> offsets = {
        {"C",0}, {"C#",1}, {"DB",1}, {"D",2}, {"D#",3}, {"EB",3}, {"E",4},
        {"F",5}, {"F#",6}, {"GB",6}, {"G",7}, {"G#",8}, {"AB",8}, {"A",9},
        {"A#",10}, {"BB",10}, {"B",11}
    };
    if (note.empty()) return 440.0;
    std::string s = ToUpper(Trim(note));

    int i = (int)s.size()-1;
    while (i>=0 && isdigit((unsigned char)s[i])) --i;
    std::string pitch = s.substr(0, i+1);
    std::string octave_s = s.substr(i+1);
    int octave = 4;
    if (!octave_s.empty()) octave = std::stoi(octave_s);
    auto it = offsets.find(pitch);
    if (it==offsets.end()) {
        return 440.0;
    }
    int note_index = it->second;

    int midi = (octave + 1) * 12 + note_index;

    double freq = 440.0 * std::pow(2.0, (midi - 69) / 12.0);
    return freq;
}
