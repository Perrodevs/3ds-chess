#include "ui/end_scene.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
    ui::EndScene scene;
    assert(scene.finished());
    economy::Session session;
    for (int human : {1, -1}) {
        session.human = human;
        for (auto result : {chess::Result::WhiteWin, chess::Result::BlackWin}) {
            session.game.result = result;
            const int winner = result == chess::Result::WhiteWin ? 1 : -1;
            scene.begin(session);
            assert(!scene.finished());
            assert(scene.defeatedSide() == -winner);
            assert(std::strcmp(scene.title(), winner == human ? "VICTORIA" : "DERROTA") == 0);
            for (int frame = 0; frame < 179; ++frame) {
                assert(!scene.finished());
                scene.update();
            }
            assert(!scene.finished());
            scene.update();
            assert(scene.finished());
            for (int i = 0; i < 1000; ++i)
                scene.update();
            assert(scene.finished() && scene.defeatedSide() == -winner);
            scene.begin(session);
            scene.skip();
            assert(scene.finished());
        }
    }
    session.mode = 1;
    session.human = 1;
    session.game.result = chess::Result::BlackWin;
    scene.begin(session);
    assert(std::strcmp(scene.title(), "VICTORIA") == 0 && scene.defeatedSide() == 1);
    session.game.result = chess::Result::Draw;
    scene.begin(session);
    assert(scene.finished() && scene.defeatedSide() == 0);
    assert(std::strcmp(scene.title(), "TABLAS") == 0);
    session.game.result = chess::Result::Playing;
    scene.begin(session);
    assert(scene.finished() && scene.defeatedSide() == 0);
    assert(std::strcmp(scene.title(), "PARTIDA EN CURSO") == 0);

    graphics::Renderer renderer;
    assert(renderer.init());
    session.mode = 0;
    session.human = 1;
    for (auto result : {chess::Result::WhiteWin, chess::Result::BlackWin, chess::Result::Draw}) {
        session.game.result = result;
        scene.begin(session);
        const char *prefix = result == chess::Result::Draw       ? "draw"
                             : result == chess::Result::WhiteWin ? "victory"
                                                                 : "defeat";
        for (int frame : {0, 80, 160, 180}) {
            scene.begin(session);
            for (int i = 0; i < frame; ++i)
                scene.update();
            renderer.frame();
            scene.drawTop(renderer);
            renderer.screenBegin(1, graphics::rgb(20, 25, 36));
            renderer.text(12, 20, "RESULTADO DE PRUEBA / MAC");
            renderer.end();
#ifndef __3DS__
            renderer.save(std::string("build/preview-end-") + prefix + "-" + std::to_string(frame) +
                          ".svg");
#endif
        }
    }
    renderer.shutdown();
    std::cout << "End scene: both human colors, winning/losing sides, local/draw, finite frames, "
                 "skip and render snapshots passed\n";
}
