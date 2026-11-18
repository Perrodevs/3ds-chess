#pragma once
#include "graphics/renderer.hpp"
#include <cstdint>

namespace ui {
inline constexpr int helpPageCount = 4;
// Top-screen functions start their own scene. The caller owns frame()/end().
void drawTitleTop(graphics::Renderer &, uint32_t ticks);
void drawHelpTop(graphics::Renderer &, int page);
// Preserve the existing profile header: draws only within y40..219 on bottom.
void drawHelpBottom(graphics::Renderer &, int page);
} // namespace ui
