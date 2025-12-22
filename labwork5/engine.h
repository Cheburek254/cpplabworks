#pragma once
#include "parser.h"
#include "instruments.h"
#include "effects.h"
#include "wav.h"
#include <memory>

struct RenderOptions {
    int sampleRate = 44100;
};

class Engine {
public:
    Engine(const Song &s);
    ~Engine();

    bool Render(WavData &out, const RenderOptions &opt, std::string &err);
private:
    const Song &song_;

    std::map<std::string, Instrument*> instrument_objs_;
};
