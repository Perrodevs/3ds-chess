#pragma once
#include <cstddef>
#include <cstdint>
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using gfxScreen_t = int;
constexpr int GFX_TOP = 0, GFX_BOTTOM = 1, GFX_LEFT = 0;
constexpr int NDSP_OUTPUT_STEREO = 1, NDSP_INTERP_NONE = 0, NDSP_FORMAT_MONO_PCM16 = 1;
#define R_FAILED(value) ((value) < 0)
struct ndspWaveBuf {
    void *data_vaddr = nullptr;
    u32 nsamples = 0;
    bool looping = false;
    int status = 0;
};
void gfxInitDefault();
void gfxExit();
int ndspInit();
void ndspExit();
void *linearAlloc(size_t);
void linearFree(void *);
void ndspSetOutputMode(int);
void ndspChnSetInterp(int, int);
void ndspChnSetRate(int, float);
void ndspChnSetFormat(int, int);
void ndspChnSetMix(int, const float *);
void ndspChnWaveBufClear(int);
void ndspChnWaveBufAdd(int, ndspWaveBuf *);
void DSP_FlushDataCache(const void *, size_t);
