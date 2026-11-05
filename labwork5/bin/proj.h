#ifndef PROJECT_H
#define PROJECT_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "types.h"
#include "audio_engine.h"

class Project {
 public:
    int bpm = 120;
    std::map<std::string, std::shared_ptr<Instrument>> instruments;
    std::map<std::string, Pattern> patterns;
    std::vector<RenderEvent> event_list;

    void LoadFromFile(const std::string& filename);
    void Render(const std::string& out_file);

 private:
    void ProcessPattern(const std::string& pat_name, double start_time_sec);
    void WriteWav(const std::string& out_file, const AudioBuffer& buffer);
};

#endif
