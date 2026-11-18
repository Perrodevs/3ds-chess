#include "localization/localization.hpp"
#include "ai/bots.hpp"
#include "graphics/audio.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <set>
#include <string>
int main() {
    std::set<uint64_t> loops;
    for (int theme = -1; theme < ai::botCount; ++theme) {
        uint64_t hash = 1469598103934665603ull;
        int64_t energy = 0, total = 0;
        for (int i = 0; i < graphics::musicLoopSamples; ++i) {
            const int sample = graphics::musicSample(theme, i);
            assert(std::abs(sample) <= 700);
            assert(sample == graphics::musicSample(theme, i));
            energy += static_cast<int64_t>(sample) * sample;
            total += sample;
            hash = (hash ^ static_cast<uint16_t>(sample)) * 1099511628211ull;
        }
        assert(energy > 100000000);
        assert(std::abs(total) < 100000); // No sustained DC offset.
        assert(graphics::musicSample(theme, 0) == 0);
        assert(graphics::musicSample(theme, graphics::musicLoopSamples - 1) == 0);
        assert(loops.insert(hash).second); // Every rival and menu has a distinct loop.
    }
    assert(graphics::musicSample(-1, -1) == 0);
    assert(graphics::musicSample(0, graphics::musicLoopSamples) == 0);
    for (int i = 0; i < 10000; ++i)
        assert(graphics::musicSample(999, i) == graphics::musicSample(-1, i));
    std::set<std::string> greetings;
    for (int bot = 0; bot < ai::botCount; ++bot) {
        assert(greetings.insert(ai::botDialogue(bot, ai::Dialogue::Greeting)).second);
        for (int event = 0; event < 9; ++event) {
            const char *line = ai::botDialogue(bot, static_cast<ai::Dialogue>(event));
            assert(line && *line && localization::glyphCount(line) <= 36);
            std::string utf8=line;size_t byte=0;
            while(byte<utf8.size()){const auto cp=localization::nextCodepoint(utf8,byte);assert(cp=='?'||localization::fontIndex(cp)!='?'-32);}
        }
        for (int expression = 0; expression < 4; ++expression)
            assert(*ai::botLine(bot, expression) && std::strlen(ai::botLine(bot, expression)) <= 23);
    }
    assert(std::strcmp(ai::botDialogue(-1, ai::Dialogue::Greeting),
                       ai::botDialogue(0, ai::Dialogue::Greeting)) == 0);
    assert(std::strcmp(ai::botDialogue(38, static_cast<ai::Dialogue>(-1)),
                       ai::botDialogue(38, ai::Dialogue::Greeting)) == 0);
    graphics::Audio audio;
    audio.music(true, 38); // Host has no audio device; lifecycle must remain safe.
    audio.music(false);
    audio.shutdown();
    audio.shutdown();
    std::cout << "42 unique original music loops; 369 short character lines verified\n";
}
