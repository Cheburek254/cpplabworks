#include "parser.h"
#include "utils.h"
#include <fstream>
#include <vector>
#include <string>
#include <cstddef>
#include <cctype>
static std::vector<std::string> ReadComments(const std::string &path) {
    std::ifstream ifs(path);
    std::vector<std::string> out;
    std::string line;
    while (std::getline(ifs, line)) {
        auto pos = line.find('#');
        if (pos != std::string::npos) line = line.substr(0,pos);
        out.push_back(Trim(line));
    }
    return out;
}

bool ParseScoreFile(const std::string &path, Song &out, std::string &err) {
    auto lines = ReadComments(path);
    size_t i=0;
    while (i<lines.size()) {
        std::string line = lines[i++];
        if (line.empty()) continue;
        auto words = SplitWS(line);
        if (words.empty()) continue;
        if (words[0] == "bpm") {
            if (words.size()<2) { err="bpm value expected"; return false; }
            out.bpm = std::stoi(words[1]);
        } else if (words[0] == "instrument") {
            if (words.size()<3) { err="instrument name/type expected"; return false; }
            InstrumentDef def;
            def.name = words[1];
            def.type = words[2];

            while (i < lines.size()) {
                std::string l = lines[i++];
                if (l.empty()) continue;
                if (l == "end") break;
                auto ws = SplitWS(l);
                if (ws.empty()) continue;
                if (ws[0] == "effect") {
                    def.effects_raw.push_back(l.substr(6));
                } else {

                    auto eq = l.find('=');
                    if (eq != std::string::npos) {
                        std::string k = Trim(l.substr(0,eq));
                        std::string v = Trim(l.substr(eq+1));
                        def.params[k] = v;
                    }
                }
            }
            out.instruments[def.name] = def;
        } else if (words[0] == "pattern") {
            if (words.size() < 2) { err="pattern name expected"; return false; }
            Pattern p;
            p.name = words[1];

            if (words.size() >= 3) {
                if (words[2] == "resolution" && words.size() >=4) {
                    p.resolution = std::stoi(words[3]);
                } else {

                    p.resolution = std::stoi(words[2]);
                }
            }

            while (i < lines.size()) {
                std::string l = lines[i++];
                if (l.empty()) continue;
                if (l == "end") break;
                p.raw_lines.push_back(l);
            }

            for (auto &rl : p.raw_lines) {
                auto ws = SplitWS(rl);
                if (ws.empty()) continue;
                if (ws[0].size()>0 && ws[0][0]=='@') {

                    int start = 0;
                    size_t idx = 0;
                    if (isdigit((unsigned char)ws[0][0])) {

                        start = std::stoi(ws[0]);
                        if (ws.size() >= 2 && ws[1].size()>0 && ws[1][0]=='@') {
                            std::string pname = ws[1].substr(1);
                            p.calls.push_back({start, pname});
                        }
                    } else { 
                        std::string t = ws[0];
                        if (t[0]=='@') p.calls.push_back({0, t.substr(1)});
                    }
                } else if (ws[0].size()>0 && isdigit((unsigned char)ws[0][0])) {

                    if (ws.size() < 5) {

                        continue;
                    }
                    NoteEvent ne;
                    ne.start_unit = std::stoi(ws[0]);
                    ne.instrument = ws[1];
                    ne.pitch = ws[2];
                    ne.duration_units = std::stoi(ws[3]);
                    ne.velocity = std::stof(ws[4]) / 100.0f;
                    p.notes.push_back(ne);
                }
            }
            out.patterns[p.name] = p;
        } else {

        }
    }

    if (out.patterns.find("main") == out.patterns.end()) {
        err = "pattern 'main' not found";
        return false;
    }
    return true;
}
