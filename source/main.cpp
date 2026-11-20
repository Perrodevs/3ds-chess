#include "diagnostics/console.hpp"
#include "diagnostics/log.hpp"
#include "ui/app.hpp"
#include <3ds.h>
#include <chrono>
#include <sys/stat.h>
// The in-app review runs bounded search on the main thread. libctru defaults
// to 32 KiB; reserve the same 256 KiB stack used by our search worker.
extern "C" { u32 __stacksize__ = 256 * 1024; }
static void hook(APT_HookType type, void *arg) {
    if (type == APTHOOK_ONSUSPEND || type == APTHOOK_ONSLEEP || type == APTHOOK_ONEXIT)
        static_cast<ui::App *>(arg)->suspend();
    else if (type == APTHOOK_ONRESTORE || type == APTHOOK_ONWAKEUP)
        static_cast<ui::App *>(arg)->resume();
}
int main() {
    const std::string root = "sdmc:/3ds/3ds-chess";
    mkdir("sdmc:/3ds", 0777);
    mkdir(root.c_str(), 0777);
    auto language = localization::packageDefault;
    localization::loadLanguage(root + "/global-language.dat", language);
    localization::setLanguage(language);
    auto &log = diagnostics::journal();
    log.begin(root, language == localization::Language::English ? "EN" : "ES");
    graphics::Renderer renderer;
    if (!renderer.init()) {
        log.add(diagnostics::Level::Fatal, "GRAPHICS_INIT_FAILED", "renderer_targets_atlases");
        log.end();
        return 1;
    }
    {
        ui::App app(root, true);
        diagnostics::Console diagnostic(root);
        diagnostic.start(true, false, true, app.prepareAudio());
        bool initialized = false;
        aptHookCookie cookie;
        hidSetRepeatParameters(20, 5);
        auto previous = std::chrono::steady_clock::now();
        while (aptMainLoop() && !app.exitRequested) {
            hidScanInput();
            u32 down = hidKeysDown(), repeat = hidKeysDownRepeat();
            ui::Input in;
            u32 navigation = down | repeat;
            if (navigation & KEY_DUP)
                in.keys |= ui::Up;
            if (navigation & KEY_DDOWN)
                in.keys |= ui::Down;
            if (navigation & KEY_DLEFT)
                in.keys |= ui::Left;
            if (navigation & KEY_DRIGHT)
                in.keys |= ui::Right;
            if (down & KEY_A)
                in.keys |= ui::A;
            if (down & KEY_B)
                in.keys |= ui::B;
            if (down & KEY_START)
                in.keys |= ui::Start;
            if (down & KEY_SELECT)
                in.keys |= ui::Select;
            if (down & KEY_X)
                in.keys |= ui::Hint;
            if (down & KEY_TOUCH) {
                touchPosition t;
                hidTouchRead(&t);
                in.touch = true;
                in.x = t.px;
                in.y = t.py;
            }
            if (diagnostic.active()) {
                if (!initialized && (in.keys & ui::Start)) break;
                diagnostic.update(in.keys & ui::Up, in.keys & ui::Down, in.keys & ui::A, in.keys & ui::B);
                if (!initialized && diagnostic.canStartGame()) {
                    app.init();
                    initialized = true;
                    aptHook(&cookie, hook, &app);
                }
                if (diagnostic.active()) diagnostic.draw(renderer);
                else app.draw(renderer);
                continue;
            }
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - previous).count();
            if (elapsed >= 1) {
                if (elapsed < 5 &&
                    (app.view == ui::View::Game || app.view == ui::View::Promotion) &&
                    app.state.session.game.result == chess::Result::Playing)
                    app.state.profile.seconds += elapsed;
                previous = now;
            }
            app.update(in);
            if (app.diagnosticRequested) {
                app.diagnosticRequested = false;
                diagnostic.menu();
                diagnostic.draw(renderer);
                continue;
            }
            if (elapsed >= 1 && app.view != ui::View::Game && app.view != ui::View::Promotion)
                log.flush();
            if (app.workshopTextRequested()) {
                SwkbdState keyboard;
                swkbdInit(&keyboard, SWKBD_TYPE_NORMAL, 2, 24);
                const auto prompt=app.workshopTextPrompt(), initial=app.workshopTextValue();
                const auto localizedPrompt=localization::translate(prompt);
                swkbdSetHintText(&keyboard,localizedPrompt.c_str());
                swkbdSetInitialText(&keyboard,initial.c_str());
                char text[128]{};
                const bool confirmed=swkbdInputText(&keyboard,text,sizeof(text))==SWKBD_BUTTON_CONFIRM;
                app.acceptWorkshopText(text,confirmed);
                previous=std::chrono::steady_clock::now();
            }
            if (app.wantsName) {
                SwkbdState keyboard;
                swkbdInit(&keyboard, SWKBD_TYPE_NORMAL, 2, 24);
                const auto nameHint=localization::translate("Nombre del jugador (sin acentos)");
                swkbdSetHintText(&keyboard,nameHint.c_str());
                const auto initial = app.profileNameInitial();
                swkbdSetInitialText(&keyboard, initial.c_str());
                swkbdSetValidation(&keyboard, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
                char name[128]{};
                const auto button = swkbdInputText(&keyboard, name, sizeof(name));
                if (button == SWKBD_BUTTON_CONFIRM)
                    app.setName(name);
                else
                    app.cancelProfileName(button == SWKBD_BUTTON_NONE, int(keyboard.result));
                previous = std::chrono::steady_clock::now();
            }
            app.draw(renderer);
        }
        diagnostic.cancel();
        if (initialized) {
            app.suspend();
            aptUnhook(&cookie);
        }
    }
    log.end();
    renderer.shutdown();
    return 0;
}
