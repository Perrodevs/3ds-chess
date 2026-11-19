#include "localization/localization.hpp"
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
    const auto translated=localization::translate(text);
    const int width = int(localization::glyphCount(translated)) * 6 * scale;
    renderer.rawText((400 - width) / 2, y, translated, color, scale);
}
void largeQueen(graphics::Renderer &renderer, int side, int x, int y, int scale, int skin) {
    renderer.image((chess::Queen - 1) * 16, 64 + skin * 32 + (side < 0 ? 16 : 0), 12, 12, x, y,
                   scale);
}
} // namespace
SpaceVisual spaceVisual(space::Scene scene, int frame) {
    const int duration = scene == space::Scene::Rocket ? 240 : scene == space::Scene::FailedRocket ? 120 : 180;
    frame = std::clamp(frame, 0, duration);
    SpaceVisual v;
    if (scene == space::Scene::Roadster) {
        v.x = -100 + frame * 4;
        v.y = 140 - std::max(0, frame - 40) / 2;
        v.spaceMix = 255;
        v.captionCharacters = std::max(0, frame - 122) * 2;
    } else if (scene == space::Scene::Rocket || scene == space::Scene::FailedRocket) {
        const bool failed = scene == space::Scene::FailedRocket;
        int rise;
        if (failed) {
            // Continuous lift, stall, then gentle return to the same platform.
            rise = frame < 55 ? std::clamp(frame - 35, 0, 20) * 12 / 20 :
                   frame < 70 ? 12 : std::max(0, 95 - frame) * 12 / 25;
            v.spaceMix = 0;
            v.frustrated = frame >= 60;
            v.error = frame >= 60 && frame < 88 && (frame / 4) % 2 == 1;
            v.captionCharacters = std::max(0, frame - 91) * 2;
        } else {
            const int launch = std::max(0, frame - 80);
            rise = launch * launch / 55;
            v.spaceMix = std::clamp(frame - 95, 0, 65) * 255 / 65;
            v.captionCharacters = std::max(0, frame - 195) * 2;
        }
        v.x = 180; v.y = 84 - rise;
        v.engines = frame >= 20 && (!failed || frame < 60);
        v.smoke = frame >= 20 && (failed ? frame < 90 : frame < 150);
    }
    return v;
}
void EndScene::begin(const economy::Session &session, space::Scene scene) {
    const bool humanWon = (session.game.result == chess::Result::WhiteWin && session.human == 1) ||
                          (session.game.result == chess::Result::BlackWin && session.human == -1);
    const bool decisive = session.game.result == chess::Result::WhiteWin || session.game.result == chess::Result::BlackWin;
    const bool matchesResult = humanWon ? scene == space::Scene::FailedRocket :
        scene == space::Scene::Roadster || scene == space::Scene::Rocket;
    special = session.bot == 40 && session.mode == 0 && (session.human == 1 || session.human == -1) &&
              decisive && matchesResult ? scene : space::Scene::None;
    limit = special==space::Scene::Rocket?240:special==space::Scene::FailedRocket?120:Duration;
    result = session.game.result;
    local = session.mode == 1;
    winner = result == chess::Result::WhiteWin ? 1 : result == chess::Result::BlackWin ? -1 : 0;
    loser = -winner;
    if (winner) {
        headline = local || winner == session.human ? "VICTORIA" : "DERROTA";
        frame = 0;
    } else {
        headline = result == chess::Result::Draw ? "TABLAS" : "PARTIDA EN CURSO";
        frame = limit;
    }
}
void EndScene::update() {
    if (frame < limit)
        ++frame;
}
void EndScene::skip() {
    frame = limit;
}
bool EndScene::finished() const {
    return frame >= limit;
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
    if (special != space::Scene::None) {
        const bool failed = special == space::Scene::FailedRocket;
        const bool car = special == space::Scene::Roadster;
        const auto visual = spaceVisual(special, frame);
        const auto blend = [&](int a, int b) { return a + (b - a) * visual.spaceMix / 255; };
        renderer.rect(0, 0, 400, 240, rgb(blend(35, 8), blend(56, 13), blend(82, 29)));
        if (visual.spaceMix < 255)
            renderer.rect(0, 180, 400, 60, rgb(blend(40, 8), blend(48, 13), blend(65, 29)));
        for (int i = 0; i < 48; ++i) {
            // Positive wrapping: stars never jump through a negative remainder.
            int x = ((i * 73 - frame * (1 + i % 3)) % 400 + 400) % 400;
            int y = (i * 37) % 175;
            if (car || i < 4 + visual.spaceMix * 44 / 255)
                renderer.rect(x, y, 1 + i % 2, 1 + i % 2, Ink);
        }
        const int x = visual.x, y = visual.y;
        if (car) {
            // Original layered silhouette: low red body, open cabin, lamps and wheels.
            renderer.rect(x+4,y+6,78,11,rgb(158,30,43));
            renderer.rect(x+9,y,64,13,rgb(220,46,57));
            renderer.rect(x+16,y-6,42,9,rgb(244,73,75));
            renderer.rect(x+24,y-18,31,13,Blue);renderer.rect(x+21,y-18,4,15,rgb(170,35,46));
            renderer.rect(x+29,y-16,20,8,rgb(172,214,226));
            renderer.rect(x+38,y-5,16,5,Background);
            renderer.rect(x+9,y+12,15,12,Background);renderer.rect(x+61,y+12,15,12,Background);
            renderer.rect(x+13,y+16,7,5,Muted);renderer.rect(x+65,y+16,7,5,Muted);
            renderer.rect(x+74,y+3,8,4,Gold);renderer.rect(x+4,y+5,4,4,rgb(255,109,86));
        } else {
            if (visual.spaceMix < 255) renderer.rect(110,199,180,9,Muted);
            renderer.rect(x,y,40,104,Ink);
            // Widen from the tip down to the fuselage, never an inverted cone.
            for (int n=0;n<20;++n) renderer.rect(x+19-n,y-20+n,2+2*n,1,Ink);
            renderer.rect(x-8,y+77,8,26,rgb(190,37,52));renderer.rect(x+40,y+77,8,26,rgb(190,37,52));
            renderer.portrait(40,visual.frustrated?3:2,x+4,y+20,1);
            renderer.border(x+3,y+19,34,34,Blue);
            if (visual.engines) {
                renderer.rect(x+8,y+104,24,12+(frame%9),Gold);
                renderer.rect(x+14,y+104,12,20+(frame%11),rgb(235,93,45));
            }
            for (int i=0;i<12;++i) if (visual.smoke)
                renderer.rect(155+(i*29+frame)%90,192+i%3*5,7+i%4,4,Muted);
            if (visual.error) {
                renderer.rect(242,y+30,25,21,Background);
                renderer.text(249,y+32,"!",rgb(246,91,85),2,1);
                renderer.rect(x-12,y+12,3,3,Gold);renderer.rect(x+49,y+6,3,3,Gold);
            }
        }
        const auto caption = localization::translate(failed ? "EL AJEDREZ NO ERA TAN SIMPLE." :
            car ? "DERROTA EN ÓRBITA" : "SOLO 64 CASILLAS. QUÉ SIMPLE.");
        // Reveal whole UTF-8 glyphs, keeping the final caption centered and fixed.
        size_t bytes=0; int count=0;
        while(bytes<caption.size() && count++<visual.captionCharacters) localization::nextCodepoint(caption,bytes);
        renderer.rawText((400-int(localization::glyphCount(caption))*6)/2,12,caption.substr(0,bytes),Gold,1);
        centered(renderer,224,"A o B para saltar la animacion.",Muted,1);
        return;
    }
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
