#include "graphics/audio.hpp"
#include "ai/bots.hpp"
#include <cstring>
namespace graphics {
int16_t musicSample(int theme, int sampleIndex) {
    if (sampleIndex < 0 || sampleIndex >= musicLoopSamples)
        return 0;
    theme = theme < -1 || theme >= ai::botCount ? -1 : theme;
    const unsigned seed = static_cast<unsigned>(theme + 2) * 2654435761u;
    // Sixteen short original notes, with a quiet bass pulse. Neither recordings
    // nor melodies from the represented games/players are used.
    static constexpr int frequencies[] = {196, 220, 247, 262, 294, 330, 349, 392,
                                          440, 494, 523, 587, 659, 698, 784, 880};
    constexpr int beat = musicLoopSamples / 16;
    const int step = sampleIndex / beat;
    if (step >= 16)
        return 0;
    const int local = sampleIndex % beat;
    const unsigned motif = seed ^ (seed >> ((step % 7) + 1));
    const int note = (static_cast<int>((motif >> (step % 13)) & 7u) + step / 4) % 12;
    const int hz = frequencies[note];
    const int phase =
        static_cast<int>((static_cast<int64_t>(local) * hz * 1024 / musicSampleRate) % 1024);
    const int triangle = phase < 512 ? phase * 4 - 1024 : 3072 - phase * 4;
    const int square = phase < 512 ? 1024 : -1024;
    const int style = (theme + 1) % 4;
    const int tone = style == 0   ? triangle
                     : style == 1 ? (triangle * 3 + square) / 4
                     : style == 2 ? (triangle + square) / 2
                                  : square / 2;
    const int attack = local < 160 ? local : 160;
    const int decay = beat - local;
    const int envelope = attack * decay / beat;
    const int bassHz = frequencies[(theme + 2 + step / 4) % 5] / 2;
    const int bassPhase =
        static_cast<int>((static_cast<int64_t>(local) * bassHz * 1024 / musicSampleRate) % 1024);
    const int bass = bassPhase < 512 ? bassPhase * 4 - 1024 : 3072 - bassPhase * 4;
    return static_cast<int16_t>((tone * 520 + bass * 170) * envelope / (1024 * 160));
}

bool Audio::init() {
#ifdef __3DS__
    if (ready)
        return true;
    if (R_FAILED(ndspInit()))
        return false;
    samples = (int16_t *)linearAlloc(effectSampleCapacity * sizeof(int16_t));
    if (!samples) {
        ndspExit();
        return false;
    }
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetInterp(0, NDSP_INTERP_NONE);
    ndspChnSetRate(0, 22050);
    ndspChnSetFormat(0, NDSP_FORMAT_MONO_PCM16);
    float mix[12] = {0.35f, 0.35f};
    ndspChnSetMix(0, mix);
    musicSamples[0] = (int16_t *)linearAlloc(musicLoopSamples * sizeof(int16_t));
    musicSamples[1] = (int16_t *)linearAlloc(musicLoopSamples * sizeof(int16_t));
    if (!musicSamples[0] || !musicSamples[1]) {
        // A failed music allocation must not report a fully working audio
        // device. No buffers have been queued yet, so rollback is immediate.
        ndspExit();
        linearFree(samples);
        samples = nullptr;
        for (auto &buffer : musicSamples) {
            if (buffer)
                linearFree(buffer);
            buffer = nullptr;
        }
        return false;
    }
    ndspChnSetInterp(1, NDSP_INTERP_NONE);
    ndspChnSetRate(1, musicSampleRate);
    ndspChnSetFormat(1, NDSP_FORMAT_MONO_PCM16);
    ndspChnSetMix(1, mix);
    ready = true;
#endif
    return ready;
}
void Audio::play(int event, bool enabled) {
#ifdef __3DS__
    if (!enabled || !ready)
        return;
    ndspChnWaveBufClear(0);
    int n = event >= 4 ? effectSampleCapacity : 1800;
    int hz = event == 0   ? 660
             : event == 1 ? 440
             : event == 2 ? 220
             : event == 3 ? 880
             : event == 4 ? 990
                          : 165;
    for (int i = 0; i < n; ++i) {
        int freq = hz + (event >= 4 ? (i / 2200) * 110 : 0);
        int period = 22050 / freq;
        int amplitude = 2600 * (n - i) / n;
        samples[i] = (i % period < period / 2 ? amplitude : -amplitude);
    }
    DSP_FlushDataCache(samples, n * sizeof(int16_t));
    memset(&wave, 0, sizeof(wave));
    wave.data_vaddr = samples;
    wave.nsamples = n;
    ndspChnWaveBufAdd(0, &wave);
#else
    (void)event;
    (void)enabled;
#endif
}
void Audio::music(bool enabled, int theme) {
#ifdef __3DS__
    if (!ready)
        return;
    theme = theme < -1 || theme >= ai::botCount ? -1 : theme;
    if (!enabled) {
        if (musicPlaying)
            ndspChnWaveBufClear(1);
        musicPlaying = false;
        return;
    }
    if (musicPlaying && theme == musicTheme)
        return;
    const int nextBuffer = 1 - musicBuffer;
    if (!musicSamples[nextBuffer])
        return;
    // Prepare a separate buffer while the current loop is still playing.
    // Only swap its descriptor after removing the previous DSP queue entry.
    for (int i = 0; i < musicLoopSamples; ++i)
        musicSamples[nextBuffer][i] = musicSample(theme, i);
    DSP_FlushDataCache(musicSamples[nextBuffer], musicLoopSamples * sizeof(int16_t));
    ndspChnWaveBufClear(1);
    memset(&musicWave, 0, sizeof(musicWave));
    musicWave.data_vaddr = musicSamples[nextBuffer];
    musicWave.nsamples = musicLoopSamples;
    musicWave.looping = true;
    ndspChnWaveBufAdd(1, &musicWave);
    musicBuffer = nextBuffer;
    musicTheme = theme;
    musicPlaying = true;
#else
    (void)enabled;
    (void)theme;
#endif
}
void Audio::shutdown() {
#ifdef __3DS__
    if (ready) {
        ndspChnWaveBufClear(0);
        ndspChnWaveBufClear(1);
        // NDSP consumes channel updates asynchronously; stop the DSP service
        // before returning its sample memory to the allocator.
        ndspExit();
        linearFree(samples);
        for (auto &buffer : musicSamples) {
            if (buffer)
                linearFree(buffer);
            buffer = nullptr;
        }
        samples = nullptr;
        musicPlaying = false;
        musicTheme = -2;
        ready = false;
    }
#endif
}
} // namespace graphics
