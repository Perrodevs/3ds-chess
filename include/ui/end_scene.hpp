#pragma once
#include "economy/profile.hpp"
#include "graphics/renderer.hpp"

namespace ui {
// A cosmetic terminal scene. It never changes the game, clocks or rewards.
class EndScene {
    static constexpr int Duration = 180;
    int frame = Duration;
    int loser = 0;
    int winner = 0;
    bool local = false;
    chess::Result result = chess::Result::Playing;
    const char *headline = "PARTIDA EN CURSO";

  public:
    void begin(const economy::Session &);
    void update();
    void skip();
    bool finished() const;
    int defeatedSide() const;
    const char *title() const;
    // Draws only the 400x240 top screen. The owner draws buttons on the bottom.
    void drawTop(graphics::Renderer &, int skin = 0) const;
};
} // namespace ui
