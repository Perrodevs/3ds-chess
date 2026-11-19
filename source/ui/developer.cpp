#include "ui/app.hpp"
namespace ui {
void App::developerPassword(Input in) {
    if (in.touch && in.x >= 12 && in.x < 308) {
        if (in.y >= 181 && in.y < 202) in.keys |= A;
        else if (in.y >= 205 && in.y < 226) in.keys |= B;
    }
    if (in.keys & B) { back(); return; }
    if (in.keys & Left) developerDigit = (developerDigit + 3) % 4;
    if (in.keys & Right) developerDigit = (developerDigit + 1) % 4;
    auto &digit = developerCode[size_t(developerDigit)];
    if (in.keys & Up) digit = (digit + 1) % 10;
    if (in.keys & Down) digit = (digit + 9) % 10;
    if (in.touch && in.y >= 104 && in.y < 125 && in.x >= 48 && in.x < 260) {
        const int index = (in.x - 48) / 56;
        if ((in.x - 48) % 56 < 44 && index < 4) {
            developerDigit = index;
            auto &value = developerCode[size_t(index)];
            value = (value + 1) % 10;
        }
    }
    if (in.keys & A) {
        if (developerCode == std::array<int, 4>{{1, 2, 3, 4}}) {
            developerEnabled = true;
            back(); message("Developer activo: SELECT en partida");
        } else {
            developerCode = {}; developerDigit = 0;
            message("Codigo incorrecto");
        }
    }
}
bool App::beginDeveloperSandbox() {
    if (trainingActive || tournamentActive || progressionActive || openingActive || !state.session.active) {
        message("Trucos: abre una partida normal o practica"); return false;
    }
    if (!developerSandbox) {
        // Stop before snapshotting. Results from this position must never reach
        // another position or persist as an earned competitive outcome.
        bot.stop(); hints.cancel();
        if (!save()) return false;
        developerOriginal = state;
        developerSandbox = true;
        state.session.mode = 2;
        state.session.clock.reset(0);
    }
    return true;
}
void App::developerAction() {
    if (!developerEnabled) { go(View::Game); return; }
    if (focus == 6) { diagnosticRequested = true; return; }
    if (focus == 5) { go(View::Game); return; }
    if (focus == 4) { openReview(View::Developer); return; }
    if (!beginDeveloperSandbox()) return;
    selected = -1; hintMove = {}; animationFrames = 0;
    if (focus == 3) {
        state.profile.unlocked = (1u << economy::productCount) - 1;
        state.profile.coins = 10000;
        state.profile.equip(4); state.profile.equip(6);
        go(View::Game);
        message("Vista de prueba: al salir se restaura todo"); return;
    }
    state.session.game.result = focus == 1 ? chess::Result::Draw :
        ((focus == 0) == (state.session.human == 1) ? chess::Result::WhiteWin : chess::Result::BlackWin);
    state.session.game.reason = "Developer: resultado de prueba";
    refresh(); showResult();
}
} // namespace ui
