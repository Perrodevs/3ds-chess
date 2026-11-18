#include "graphics/renderer.hpp"
#include "localization/localization.hpp"
#include "graphics/elon_sprite.hpp"
#include "atlas.hpp"
#include "portraits.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#ifndef __3DS__
#include <fstream>
#include <iomanip>
#endif
namespace graphics {
bool Renderer::init() {
#ifdef __3DS__
    if (ready)
        return true;
    gfxInitDefault();
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        gfxExit();
        return false;
    }
    if (!C2D_Init(4096)) {
        C3D_Fini();
        gfxExit();
        return false;
    }
    C2D_Prepare();
    targets[0] = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    targets[1] = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (!targets[0] || !targets[1]) {
        // Fini owns and releases every successfully created target, including a
        // single target from a partial initialization. Never draw through NULL.
        C2D_Fini();
        C3D_Fini();
        targets[0] = targets[1] = nullptr;
        gfxExit();
        return false;
    }
    if (!C3D_TexInit(&atlas, 256, 256, GPU_RGBA8)) {
        C2D_Fini();
        C3D_Fini();
        gfxExit();
        return false;
    }
    if (!C3D_TexInit(&portraitAtlas, portraits::width, portraits::height, GPU_RGBA8)) {
        C3D_TexDelete(&atlas);
        C2D_Fini();
        C3D_Fini();
        gfxExit();
        return false;
    }
    auto upload = [](C3D_Tex &texture, const uint32_t *pixels, unsigned width, unsigned height) {
        auto *dst = static_cast<uint8_t *>(texture.data);
        for (unsigned y = 0; y < height; ++y)
            for (unsigned x = 0; x < width; ++x) {
                // PICA row orientation verified against the native 3DSX in Azahar.
                unsigned morton = 0;
                for (int b = 0; b < 3; ++b)
                    morton |= ((x >> b) & 1) << (b * 2) | ((y >> b) & 1) << (b * 2 + 1);
                unsigned index = ((y / 8) * (width / 8) + x / 8) * 64 + morton;
                uint32_t c = pixels[y * width + x];
                dst[index * 4] = c >> 24;
                dst[index * 4 + 1] = c >> 16;
                dst[index * 4 + 2] = c >> 8;
                dst[index * 4 + 3] = c;
            }
        C3D_TexFlush(&texture);
        C3D_TexSetFilter(&texture, GPU_NEAREST, GPU_NEAREST);
        C3D_TexSetWrap(&texture, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    };
    upload(atlas, assets::atlas, 256, 256);
    upload(portraitAtlas, portraits::atlas, portraits::width, portraits::height);
    ready = true;
#endif
    return true;
}
void Renderer::shutdown() {
#ifdef __3DS__
    if (ready) {
        // FrameEnd submits asynchronously. Deleting a target waits for its GPU
        // queue before destroying it, so textures/vertices are no longer in use.
        for (auto &target : targets) {
            C3D_RenderTargetDelete(target);
            target = nullptr;
        }
        C3D_TexDelete(&portraitAtlas);
        C3D_TexDelete(&atlas);
        C2D_Fini();
        C3D_Fini();
        gfxExit();
        ready = false;
    }
#endif
}
void Renderer::frame() {
#ifndef __3DS__
    presented.clear();
#endif
#ifdef __3DS__
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
#else
    svg.str("");
    svg.clear();
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"400\" "
           "height=\"496\" viewBox=\"0 0 400 496\" "
           "shape-rendering=\"crispEdges\">\n";
#endif
}
void Renderer::screenBegin(int s, Color c) {
#ifdef __3DS__
    C2D_TargetClear(targets[s], c);
    C2D_SceneBegin(targets[s]);
    C2D_SetTintMode(C2D_TintSolid);
#else
    if (s)
        svg << "</g>\n";
    screen = s;
    svg << "<g transform=\"translate(" << (s ? 40 : 0) << "," << (s ? 256 : 0) << ")\">\n";
    rect(0, 0, s ? 320 : 400, 240, c);
#endif
}
void Renderer::end() {
#ifdef __3DS__
    C3D_FrameEnd(0);
#else
    svg << "</g></svg>\n";
#endif
}
void Renderer::rect(int x, int y, int w, int h, Color c) {
    if (w <= 0 || h <= 0 || !(c >> 24))
        return;
#ifdef __3DS__
    C2D_DrawRectSolid(x, y, 0, w, h, c);
#else
    svg << "<rect x=\"" << x << "\" y=\"" << y << "\" width=\"" << w << "\" height=\"" << h
        << "\" fill=\"#" << std::hex << std::setfill('0') << std::setw(2) << (c & 255)
        << std::setw(2) << ((c >> 8) & 255) << std::setw(2) << ((c >> 16) & 255) << std::dec
        << "\" fill-opacity=\"" << double(c >> 24) / 255 << "\"/>\n";
#endif
}
void Renderer::arrow(int x1, int y1, int x2, int y2, Color color) {
    double dx = x2 - x1, dy = y2 - y1, length = std::sqrt(dx * dx + dy * dy);
    if (length < 1)
        return;
    double ux = dx / length, uy = dy / length;
    x1 += int(std::round(ux * 9));
    y1 += int(std::round(uy * 9));
    auto stroke = [&](int a, int b, int c, int d, int width, Color ink) {
        int x = std::abs(c - a), sx = a < c ? 1 : -1, y = -std::abs(d - b), sy = b < d ? 1 : -1,
            error = x + y;
        for (;;) {
            rect(a - width / 2, b - width / 2, width, width, ink);
            if (a == c && b == d)
                break;
            int e = 2 * error;
            if (e >= y) {
                error += y;
                a += sx;
            }
            if (e <= x) {
                error += x;
                b += sy;
            }
        }
    };
    int hx1 = int(std::round(x2 - ux * 10 - uy * 6)), hy1 = int(std::round(y2 - uy * 10 + ux * 6));
    int hx2 = int(std::round(x2 - ux * 10 + uy * 6)), hy2 = int(std::round(y2 - uy * 10 - ux * 6));
    for (int width : {5, 3}) {
        Color ink = width == 5 ? rgb(11, 34, 48) : color;
        stroke(x1, y1, x2, y2, width, ink);
        stroke(x2, y2, hx1, hy1, width, ink);
        stroke(x2, y2, hx2, hy2, width, ink);
    }
}
void Renderer::border(int x, int y, int w, int h, Color c, int n) {
    rect(x, y, w, n, c);
    rect(x, y + h - n, w, n, c);
    rect(x, y, n, h, c);
    rect(x + w - n, y, n, h, c);
}
void Renderer::image(int sx, int sy, int w, int h, int x, int y, int scale, Color tint) {
#ifdef __3DS__
    Tex3DS_SubTexture sub{(u16)w,
                          (u16)h,
                          float(sx) / 256,
                          1.f - float(sy) / 256,
                          float(sx + w) / 256,
                          1.f - float(sy + h) / 256};
    C2D_Image img{&atlas, &sub};
    C2D_ImageTint t;
    C2D_PlainImageTint(&t, tint, tint == 0xffffffff ? 0.f : 1.f);
    C2D_DrawImageAt(img, x, y, 0, &t, scale, scale);
#else
    for (int yy = 0; yy < h; ++yy)
        for (int xx = 0; xx < w;) {
            Color c = assets::atlas[(sy + yy) * 256 + sx + xx];
            int run = 1;
            while (xx + run < w && assets::atlas[(sy + yy) * 256 + sx + xx + run] == c)
                ++run;
            Color v = 0;
            for (int b = 0; b < 4; ++b)
                v |= (((c >> (b * 8)) & 255) * ((tint >> (b * 8)) & 255) / 255) << (b * 8);
            rect(x + xx * scale, y + yy * scale, run * scale, scale, v);
            xx += run;
        }
#endif
}
void Renderer::text(int x, int y, const std::string &text, Color c, int scale, int maxChars) { rawText(x,y,localization::translate(text),c,scale,maxChars); }
void Renderer::rawText(int x, int y, const std::string &text, Color c, int scale, int maxChars) {
#ifndef __3DS__
    presented.push_back(text);
#endif
    int start = x, count = 0;
    size_t byte=0;
    while(byte<text.size()) {
        uint32_t ch=localization::nextCodepoint(text,byte);
        if (ch == '\n') {
            x = start;
            y += 9 * scale;
            continue;
        }
        if (++count > maxChars)
            break;
        int index = localization::fontIndex(ch);
        image((index % 16) * 8, (index / 16) * 8, 6, 8, x, y, scale, c);
        x += 6 * scale;
    }
}
void Renderer::piece(int p, int x, int y, int skin) {
    if (!p)
        return;
    image((std::abs(p) - 1) * 16, 64 + std::clamp(skin, 0, 2) * 32 + (p < 0 ? 16 : 0), 12, 12, x, y,
          2);
}
void Renderer::portrait(int character, int expression, int x, int y, int scale) {
    if(character==40){drawElonPortrait(*this,expression,x,y,scale);return;}
    const int id = std::clamp(character, 0, portraits::characterCount - 1);
    const int sx = (id / portraits::rowsPerBank) * 128 + std::clamp(expression, 0, 3) * 32;
    const int sy = (id % portraits::rowsPerBank) * 32;
#ifdef __3DS__
    Tex3DS_SubTexture sub{32,
                          32,
                          float(sx) / portraits::width,
                          1.f - float(sy) / portraits::height,
                          float(sx + 32) / portraits::width,
                          1.f - float(sy + 32) / portraits::height};
    C2D_Image img{&portraitAtlas, &sub};
    C2D_DrawImageAt(img, x, y, 0, nullptr, scale, scale);
#else
    for (int yy = 0; yy < 32; ++yy)
        for (int xx = 0; xx < 32;) {
            Color c = portraits::atlas[(sy + yy) * portraits::width + sx + xx];
            int run = 1;
            while (xx + run < 32 &&
                   portraits::atlas[(sy + yy) * portraits::width + sx + xx + run] == c)
                ++run;
            rect(x + xx * scale, y + yy * scale, run * scale, scale, c);
            xx += run;
        }
#endif
}
#ifndef __3DS__
void Renderer::save(const std::string &p) {
    std::ofstream(p) << svg.str();
}
#endif
} // namespace graphics
