#pragma once
#include "3ds.h"
constexpr int C3D_DEFAULT_CMDBUF_SIZE = 0x40000, C3D_FRAME_SYNCDRAW = 1;
constexpr int GPU_RGBA8 = 0, GPU_NEAREST = 0, GPU_CLAMP_TO_EDGE = 0, C2D_TintSolid = 0;
struct C3D_Tex {
    void *data = nullptr;
};
struct C3D_RenderTarget {
    int screen = 0;
};
struct Tex3DS_SubTexture {
    u16 width, height;
    float left, top, right, bottom;
};
struct C2D_Image {
    C3D_Tex *tex;
    const Tex3DS_SubTexture *subtex;
};
struct C2D_ImageTint {
    u32 color = 0;
    float blend = 0;
};
bool C3D_Init(size_t);
void C3D_Fini();
bool C2D_Init(size_t);
void C2D_Fini();
void C2D_Prepare();
C3D_RenderTarget *C2D_CreateScreenTarget(int, int);
void C3D_RenderTargetDelete(C3D_RenderTarget *);
bool C3D_TexInit(C3D_Tex *, unsigned, unsigned, int);
void C3D_TexDelete(C3D_Tex *);
void C3D_TexFlush(C3D_Tex *);
void C3D_TexSetFilter(C3D_Tex *, int, int);
void C3D_TexSetWrap(C3D_Tex *, int, int);
bool C3D_FrameBegin(int);
void C3D_FrameEnd(int);
void C2D_TargetClear(C3D_RenderTarget *, u32);
void C2D_SceneBegin(C3D_RenderTarget *);
void C2D_SetTintMode(int);
void C2D_DrawRectSolid(int, int, int, int, int, u32);
void C2D_PlainImageTint(C2D_ImageTint *, u32, float);
void C2D_DrawImageAt(C2D_Image, int, int, int, const C2D_ImageTint *, int, int);
