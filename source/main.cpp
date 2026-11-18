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
    graphics::Renderer renderer;
    if (!renderer.init())
        return 1;
    mkdir("sdmc:/3ds", 0777);
    mkdir("sdmc:/3ds/3ds-chess", 0777);
    {
        ui::App app("sdmc:/3ds/3ds-chess", true);
        app.init();
        aptHookCookie cookie;
        aptHook(&cookie, hook, &app);
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
                swkbdSetInitialText(&keyboard, app.state.profile.name.c_str());
                char name[128]{};
                if (swkbdInputText(&keyboard, name, sizeof(name)) == SWKBD_BUTTON_CONFIRM)
                    app.setName(name);
                else
                    app.wantsName = false;
                previous = std::chrono::steady_clock::now();
            }
            app.draw(renderer);
        }
        app.suspend();
        aptUnhook(&cookie);
    }
    renderer.shutdown();
    return 0;
}
