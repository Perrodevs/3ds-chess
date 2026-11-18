#pragma once
#include <cstdint>
#ifdef __3DS__
#include <3ds.h>
#endif
namespace graphics {
inline constexpr int effectSampleCapacity = 7000;
inline constexpr int musicSampleRate = 22050;
inline constexpr int musicLoopSamples = 88200;
// Original deterministic PCM synthesis; theme -1 is the menu, other IDs are rivals.
int16_t musicSample(int theme, int sampleIndex);
class Audio {
#ifdef __3DS__
    int16_t *samples = nullptr;
    ndspWaveBuf wave{};
    int16_t *musicSamples[2] = {nullptr, nullptr};
    int musicBuffer = 0, musicTheme = -2;
    ndspWaveBuf musicWave{};
    bool musicPlaying = false;
#endif
    bool ready = false;

  public:
    Audio() = default;
    Audio(const Audio &) = delete;
    Audio &operator=(const Audio &) = delete;
    bool init();
    void play(int event, bool enabled);
    void music(bool enabled, int theme = -1);
    void shutdown();
};
} // namespace graphics
