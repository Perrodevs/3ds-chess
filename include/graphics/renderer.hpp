#pragma once
#include <cstdint>
#include <string>
#ifndef __3DS__
#include <vector>
#endif
#ifdef __3DS__
#include <citro2d.h>
#else
#include <sstream>
#endif
namespace graphics {
using Color = uint32_t;
constexpr Color rgb(int r, int g, int b, int a = 255) {
    return uint32_t(r) | (uint32_t(g) << 8) | (uint32_t(b) << 16) | (uint32_t(a) << 24);
}
class Renderer {
#ifdef __3DS__
    C3D_Tex atlas{}, portraitAtlas{};
    C3D_RenderTarget *targets[2]{};
    bool ready = false;
#else
    std::ostringstream svg;
    int screen = 0;
    std::vector<std::string> presented;
#endif
  public:
    Renderer() = default;
    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;
    bool init();
    void shutdown();
    void frame();
    void screenBegin(int, Color);
    void end();
    void rect(int x, int y, int w, int h, Color);
    void arrow(int x1, int y1, int x2, int y2, Color color);
    void border(int x, int y, int w, int h, Color, int thickness = 1);
    void image(int sx, int sy, int w, int h, int x, int y, int scale = 1, Color tint = 0xffffffff);
    void text(int x, int y, const std::string &, Color color = 0xffffffff, int scale = 1,
              int maxChars = 100);
    void rawText(int x, int y, const std::string &, Color color = 0xffffffff, int scale = 1, int maxChars = 100);
    void piece(int p, int x, int y, int skin = 0);
    static bool hasPortrait(int character);
    void portrait(int character, int expression, int x, int y, int scale = 2);
#ifndef __3DS__
    void save(const std::string &);
    const std::vector<std::string> &visibleTexts() const { return presented; }
#endif
};
} // namespace graphics
