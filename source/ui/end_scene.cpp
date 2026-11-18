#include "ui/end_scene.hpp"
#include <algorithm>
#include <array>
#include <cstring>

namespace ui {
namespace {
using graphics::Color;
using graphics::rgb;
constexpr Color Background = rgb(20, 25, 36), Panel = rgb(32, 40, 54), Ink = rgb(236, 227, 192),
                Muted = rgb(142, 158, 169), Gold = rgb(235, 185, 78), Blue = rgb(90, 180, 211);
constexpr std::array<int, 15> FallingPieces = {
    chess::Pawn,   chess::Knight, chess::Pawn,   chess::Bishop, chess::Pawn,
    chess::Rook,   chess::Pawn,   chess::Pawn,   chess::Rook,   chess::Pawn,
    chess::Bishop, chess::Pawn,   chess::Knight, chess::Pawn,   chess::Queen,
};
void centered(graphics::Renderer &renderer, int y, const char *text, Color color, int scale) {
    const int width = int(std::strlen(text)) * 6 * scale;
    renderer.text((400 - width) / 2, y, text, color, scale);
}
void largeQueen(graphics::Renderer &renderer, int side, int x, int y, int scale, int skin) {
    renderer.image((chess::Queen - 1) * 16, 64 + skin * 32 + (side < 0 ? 16 : 0), 12, 12, x, y,
                   scale);
}
} // namespace
void EndScene::begin(const economy::Session &session) {
    result = session.game.result;
    local = session.mode == 1;
    winner = result == chess::Result::WhiteWin ? 1 : result == chess::Result::BlackWin ? -1 : 0;
    loser = -winner;
    if (winner) {
        headline = local || winner == session.human ? "VICTORIA" : "DERROTA";
        frame = 0;
    } else {
        headline = result == chess::Result::Draw ? "TABLAS" : "PARTIDA EN CURSO";
        frame = Duration;
    }
}
void EndScene::update() {
    if (frame < Duration)
        ++frame;
}
void EndScene::skip() {
    frame = Duration;
}
bool EndScene::finished() const {
    return frame >= Duration;
}
int EndScene::defeatedSide() const {
    return loser;
}
const char *EndScene::title() const {
    return headline;
}
void EndScene::drawTop(graphics::Renderer &renderer, int skin) const {
    skin = std::clamp(skin, 0, 2);
    renderer.screenBegin(0, Background);
    renderer.border(8, 6, 384, 228, Muted);
    renderer.rect(12, 218, 376, 3, loser < 0 ? Blue : loser > 0 ? Gold : Muted);
    renderer.rect(12, 221, 376, 9, Panel);
    centered(renderer, 16, "FIN DE PARTIDA", Muted, 2);

    if (!winner) {
        if (result == chess::Result::Draw) {
            largeQueen(renderer, 1, 140, 84, 4, skin);
            largeQueen(renderer, -1, 212, 84, 4, skin);
        }
        centered(renderer, 164, headline, Ink, result == chess::Result::Draw ? 3 : 2);
        return;
    }

    // A staggered 15-piece procession. Each sprite falls for 36 frames, bounces
    // gently for eight, then rests on the ground. The final queen lands at160.
    for (int i = 0; i < int(FallingPieces.size()); ++i) {
        const int age = frame - (4 + i * 8);
        if (age < 0)
            continue;
        int y = 194;
        if (age < 36)
            y = -26 + age * age * 220 / (36 * 36);
        else if (age < 44) {
            const int bounce = age - 36;
            y -= bounce * (8 - bounce) / 2;
        }
        // The falling sprites can begin above the viewport. Citro2D clips them;
        // integer coordinates and the existing nearest-neighbor atlas are kept.
        renderer.piece(loser * FallingPieces[i], 12 + i * 25, y, skin);
        if (age >= 36 && age < 40) {
            renderer.rect(12 + i * 25 - (age - 35), 216, 2, 2, Muted);
            renderer.rect(35 + i * 25 + (age - 35), 216, 2, 2, Muted);
        }
    }
    if (frame < 160) {
        centered(renderer, 45, loser == 1 ? "CAEN LAS PIEZAS DORADAS" : "CAEN LAS PIEZAS AZULES",
                 loser == 1 ? Gold : Blue, 1);
        return;
    }

    const int queenScale = std::min(6, 1 + (frame - 160) / 4);
    largeQueen(renderer, loser, (400 - 12 * queenScale) / 2, 112 - 6 * queenScale, queenScale,
               skin);
    centered(renderer, 160, headline, winner == 1 ? Gold : Blue, 3);
    if (local)
        centered(renderer, 185, winner == 1 ? "GANADOR DORADO" : "GANADOR AZUL",
                 winner == 1 ? Gold : Blue, 1);
    else
        centered(renderer, 185, loser == 1 ? "BANDO DERROTADO: DORADO" : "BANDO DERROTADO: AZUL",
                 Muted, 1);
}
} // namespace ui
