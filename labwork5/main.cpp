#include "parser.h"
#include "engine.h"
#include "wav.h"
#include <string>
#include <iostream>

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " score.txt out.wav\n";
        return 2;
    }
    std::string score = argv[1];
    std::string outpath = argv[2];

    Song song;
    std::string err;
    if (!ParseScoreFile(score, song, err)) {
        std::cerr << "Failed parse: " << err << "\n";
        return 3;
    }

    Engine eng(song);
    WavData out;
    RenderOptions opt;
    opt.sampleRate = 44100;
    if (!eng.Render(out, opt, err)) {
        std::cerr << "Render failed: " << err << "\n";
        return 4;
    }

    if (!WriteWAV(outpath, out)) {
        std::cerr << "Failed write WAV\n";
        return 5;
    }
    std::cout << "WAV written: " << outpath << " (" << out.samples.size() << " samples)\n";
    return 0;
}
