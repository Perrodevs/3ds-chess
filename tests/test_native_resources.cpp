// Exercise the actual __3DS__ branches against an instrumented SDK boundary.
// This checks ownership/failure handling, not GPU/DSP execution or physical audio.
#include "atlas.hpp"
#include "graphics/audio.hpp"
#include "graphics/renderer.hpp"
#include "portraits.hpp"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <type_traits>
#include <vector>
static_assert(!std::is_copy_constructible<graphics::Renderer>::value, "GPU resources have one owner");
namespace {
int failStage = 0, stage = 0, gfxRefs = 0, c3dRefs = 0, c2dRefs = 0, dspRefs = 0;
int gfxCalls = 0, dspCalls = 0;
bool gpuBusy = false;
std::set<void *> linear, textures;
std::map<void *, size_t> linearSizes;
size_t linearBytes = 0;
std::vector<C3D_RenderTarget *> targets;
u32 tintColor = 0;
float tintBlend = -1;
ndspWaveBuf *queued[2]{};
bool fail() {
    return ++stage == failStage;
}
void clean() {
    assert(!gfxRefs && !c3dRefs && !c2dRefs && !dspRefs);
    assert(linear.empty() && linearSizes.empty() && linearBytes == 0 && textures.empty() &&
           targets.empty());
    assert(!queued[0] && !queued[1]);
}
void reset(int stageToFail = 0) {
    clean();
    failStage = stageToFail;
    stage = 0;
    gpuBusy = false;
}
} // namespace
void gfxInitDefault() {
    ++gfxRefs;
    ++gfxCalls;
}
void gfxExit() {
    assert(gfxRefs == 1);
    --gfxRefs;
}
bool C3D_Init(size_t) {
    if (fail())
        return false;
    ++c3dRefs;
    return true;
}
void C3D_Fini() {
    assert(c3dRefs == 1);
    gpuBusy = false;
    for (auto *t : targets)
        delete t;
    targets.clear();
    --c3dRefs;
}
bool C2D_Init(size_t objects) {
    assert(objects == 4096);
    if (fail())
        return false;
    ++c2dRefs;
    return true;
}
void C2D_Fini() {
    assert(c2dRefs == 1 && !gpuBusy);
    --c2dRefs;
}
void C2D_Prepare() {}
C3D_RenderTarget *C2D_CreateScreenTarget(int screen, int) {
    if (fail())
        return nullptr;
    auto *t = new C3D_RenderTarget;
    t->screen = screen;
    targets.push_back(t);
    return t;
}
void C3D_RenderTargetDelete(C3D_RenderTarget *t) {
    assert(t);
    gpuBusy = false;
    auto i = std::find(targets.begin(), targets.end(), t);
    assert(i != targets.end());
    targets.erase(i);
    delete t;
}
bool C3D_TexInit(C3D_Tex *t, unsigned w, unsigned h, int) {
    if (fail())
        return false;
    t->data = std::malloc(w * h * 4);
    assert(t->data);
    textures.insert(t->data);
    return true;
}
void C3D_TexDelete(C3D_Tex *t) {
    assert(!gpuBusy && textures.erase(t->data) == 1);
    std::free(t->data);
    t->data = nullptr;
}
void C3D_TexFlush(C3D_Tex *) {}
void C3D_TexSetFilter(C3D_Tex *, int, int) {}
void C3D_TexSetWrap(C3D_Tex *, int, int) {}
bool C3D_FrameBegin(int) {
    gpuBusy = false;
    return true;
}
void C3D_FrameEnd(int) {
    gpuBusy = true;
}
void C2D_TargetClear(C3D_RenderTarget *t, u32) {
    assert(t);
}
void C2D_SceneBegin(C3D_RenderTarget *t) {
    assert(t);
}
void C2D_SetTintMode(int mode) {
    assert(mode == C2D_TintSolid);
}
void C2D_DrawRectSolid(int, int, int, int, int, u32) {}
void C2D_PlainImageTint(C2D_ImageTint *t, u32 color, float blend) {
    t->color = color;
    t->blend = blend;
}
void C2D_DrawImageAt(C2D_Image image, int, int, int, const C2D_ImageTint *t, int, int) {
    assert(image.tex && textures.count(image.tex->data));
    assert(image.subtex);
    if (t) {
        tintColor = t->color;
        tintBlend = t->blend;
    }
}
int ndspInit() {
    ++dspCalls;
    if (fail())
        return -1;
    ++dspRefs;
    return 0;
}
void ndspExit() {
    assert(dspRefs == 1);
    queued[0] = queued[1] = nullptr;
    --dspRefs;
}
void *linearAlloc(size_t size) {
    if (fail())
        return nullptr;
    void *p = std::malloc(size);
    assert(p);
    linear.insert(p);
    linearSizes[p] = size;
    linearBytes += size;
    return p;
}
void linearFree(void *p) {
    assert(dspRefs == 0 && !queued[0] && !queued[1]);
    assert(linear.erase(p) == 1);
    linearBytes -= linearSizes.at(p);
    linearSizes.erase(p);
    std::free(p);
}
void ndspSetOutputMode(int) {}
void ndspChnSetInterp(int, int) {}
void ndspChnSetRate(int, float) {}
void ndspChnSetFormat(int, int) {}
void ndspChnSetMix(int, const float *) {}
void ndspChnWaveBufClear(int channel) {
    queued[channel] = nullptr;
}
void ndspChnWaveBufAdd(int channel, ndspWaveBuf *buffer) {
    assert(dspRefs == 1 && linear.count(buffer->data_vaddr));
    assert(buffer->nsamples > 0 &&
           buffer->nsamples * sizeof(int16_t) <= linearSizes.at(buffer->data_vaddr));
    queued[channel] = buffer;
}
void DSP_FlushDataCache(const void *p, size_t bytes) {
    assert(linear.count(const_cast<void *>(p)));
    assert(bytes > 0);
}
int main() {
    static_assert(!std::is_copy_constructible<graphics::Audio>::value,
                  "NDSP resources cannot be copied");
    // C3D, C2D, top target, bottom target, font atlas, portrait atlas fail in turn.
    for (int failure = 1; failure <= 6; ++failure) {
        reset(failure);
        graphics::Renderer renderer;
        assert(!renderer.init());
        renderer.shutdown();
        clean();
        failStage = 0;
        assert(renderer.init());
        int calls = gfxCalls;
        assert(renderer.init() && gfxCalls == calls);
        renderer.frame();
        renderer.screenBegin(0, graphics::rgb(20, 25, 36));
        renderer.text(0, 0, "A", graphics::rgb(235, 185, 78));
        assert(tintColor == graphics::rgb(235, 185, 78) && tintBlend == 1.0f);
        renderer.screenBegin(1, graphics::rgb(20, 25, 36));
        renderer.portrait(39, 3, 0, 0);
        renderer.end();
        assert(gpuBusy);
        renderer.shutdown();
        renderer.shutdown();
        clean();
    }
    // NDSP and each of its three linear sample allocations fail independently.
    for (int failure = 1; failure <= 4; ++failure) {
        reset(failure);
        graphics::Audio audio;
        assert(!audio.init());
        audio.play(4, true);
        audio.music(true, 39);
        audio.shutdown();
        clean();
        failStage = 0;
        assert(audio.init());
        int calls = dspCalls;
        assert(audio.init() && dspCalls == calls);
        assert(linearBytes ==
               size_t(graphics::effectSampleCapacity + 2 * graphics::musicLoopSamples) *
                   sizeof(int16_t));
        assert(linearBytes ==
               366800); // Previously376800:10000bytes saved without changing a sample.
        for (int event = 0; event < 7; ++event) {
            audio.play(event, true);
            assert(queued[0]);
            assert(queued[0]->nsamples == u32(event >= 4 ? 7000 : 1800));
        }
        audio.music(true, 39);
        assert(queued[0] && queued[1]);
        void *first = queued[1]->data_vaddr;
        audio.music(true, 39);
        assert(queued[1]->data_vaddr == first);
        audio.music(true, 0);
        assert(queued[1]->data_vaddr != first);
        audio.music(false);
        assert(!queued[1]);
        audio.shutdown();
        audio.shutdown();
        clean();
    }
    std::cout << "Native SDK boundary: 10 allocation/service failures, rollback/retry, idempotent "
                 "init, GPU wait, NDSP release, tint contract passed\n";
}
