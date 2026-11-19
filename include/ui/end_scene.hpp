#pragma once
#include "economy/profile.hpp"
#include "space/secret.hpp"
#include "graphics/renderer.hpp"

namespace ui {
// Small deterministic timeline, shared by rendering and regression checks.
// Coordinates are pixel units; no resources or persistent state are owned here.
struct SpaceVisual {
    int x = 0, y = 0, spaceMix = 0, captionCharacters = 0;
    bool engines = false, smoke = false, error = false, frustrated = false;
};
SpaceVisual spaceVisual(space::Scene, int frame);
// A cosmetic terminal scene. It never changes the game, clocks or rewards.
class EndScene {
    static constexpr int Duration = 180;
    int frame = Duration, limit = Duration;
    space::Scene special = space::Scene::None;
    int loser = 0;
    int winner = 0;
    bool local = false;
    chess::Result result = chess::Result::Playing;
    const char *headline = "PARTIDA EN CURSO";

  public:
    void begin(const economy::Session &, space::Scene = space::Scene::None);
    space::Scene scene() const { return special; }
    void update();
    void skip();
    bool finished() const;
    int defeatedSide() const;
    const char *title() const;
    // Draws only the 400x240 top screen. The owner draws buttons on the bottom.
    void drawTop(graphics::Renderer &, int skin = 0) const;
};
} // namespace ui
