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
void EndScene::begin(const economy::Session &session, space::Scene scene) {
    special = session.bot==40 && session.mode==0 ? scene : space::Scene::None;
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
    if(special!=space::Scene::None){
        const bool failed=special==space::Scene::FailedRocket;
        const bool car=special==space::Scene::Roadster;
        const bool sky=car||frame>125;
        if(!sky)renderer.rect(0,180,400,60,rgb(40,48,65));
        for(int i=0;i<48;++i){int x=(i*73+400-frame*(1+i%3))%400;int y=(i*37)%175;if(sky||i<12)renderer.rect(x,y,1+i%2,1+i%2,Ink);}
        if(car){int x=-100+frame*4;int y=140-std::max(0,frame-40)/2;
            renderer.rect(x,y,82,17,rgb(190,37,52));renderer.rect(x+8,y-8,54,11,rgb(230,55,65));
            renderer.rect(x+24,y-22,33,16,Blue);renderer.rect(x+21,y-22,4,16,rgb(190,37,52));
            renderer.rect(x+8,y+12,15,12,Background);renderer.rect(x+61,y+12,15,12,Background);
            renderer.rect(x+12,y+16,7,5,Muted);renderer.rect(x+65,y+16,7,5,Muted);
            renderer.rect(x+73,y+2,8,4,Gold);
        }else{
            int rise=failed?(frame<60?std::max(0,frame-35)/2:std::max(0,90-frame)/2):std::max(0,frame-80)*std::max(0,frame-80)/55;
            int y=84-rise;renderer.rect(110,199,180,9,Muted);renderer.rect(180,y,40,104,Ink);
            for(int n=0;n<16;++n)renderer.rect(180+n,y-16+n,40-2*n,1,Ink);
            renderer.rect(172,y+77,8,26,rgb(190,37,52));renderer.rect(220,y+77,8,26,rgb(190,37,52));
            renderer.portrait(40,failed&&frame>60?3:2,184,y+20,1);
            if(frame>20&&(!failed||frame<65)){renderer.rect(188,y+104,24,12+(frame%9),Gold);renderer.rect(194,y+104,12,20+(frame%11),rgb(235,93,45));}
            for(int i=0;i<12;++i)if(frame>20)renderer.rect(155+(i*29+frame)%90,192+i%3*5,7+i%4,4,Muted);
        }
        centered(renderer,12,failed?"EL AJEDREZ NO ERA TAN SIMPLE.":car?"DERROTA EN ÓRBITA":"SOLO 64 CASILLAS. QUÉ SIMPLE.",Gold,1);
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
