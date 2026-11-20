#include <cstdlib>
#include <algorithm>
#include "diagnostics/console.hpp"
#include "diagnostics/log.hpp"
#include "ui/app.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace diagnostics;
static void complete(Console &console) {
    for (int frame = 0; frame < 700 && !console.completed(); ++frame) {
        console.update(false, false, false, false);
        if (!console.completed()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(console.completed() && console.size() == 10);
}
int main(int argc, char **argv) {
    char root[] = "/tmp/chess-diagnostics-XXXXXX";
    assert(mkdtemp(root));
    fs::path directory(root);
    assert(journal().begin(root, "ES"));
    const auto begin = std::chrono::steady_clock::now();
    Console console(root);
    console.start(true, false, true, false);
    complete(console);
    for (size_t i = 0; i < 10; ++i) {
        const auto expected = i == 3 ? Status::Warn : i == 9 ? Status::Skip : Status::Pass;
        if (console[i].status != expected) std::cerr << console[i].code << " " << console[i].stage << "\n";
        assert(console[i].status == expected);
    }
    assert(!fs::exists(directory / "profiles"));
    for (const auto &entry : fs::directory_iterator(directory)) assert(entry.path().filename() == "logs");
    console.update(false, false, true, false);
    assert(console.canStartGame());
    auto bootMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count();
    console.start(false, true, true, false);
    for (int i = 0; i < 10; ++i) console.update(false, false, false, false);
    console.update(false, false, false, true);
    assert(console.completed() && console.active());
    console.start(false, true, true, false);
    complete(console);
    assert(console[9].status == Status::Pass);
    graphics::Renderer renderer;
    assert(renderer.init());
    localization::setLanguage(localization::Language::English);
    console.draw(renderer);
    const auto texts = renderer.visibleTexts();
    assert(std::find(texts.begin(), texts.end(), "[PASS] AI thread and legal move") != texts.end());
    renderer.shutdown();
    fs::create_directory(directory / "blocked");
    Console broken((directory / "blocked" / "missing").string());
    broken.start(true, false, true, false);
    complete(broken);
    assert(broken[6].status == Status::Fail && broken[7].status == Status::Skip);
    broken.update(false, false, true, false);
    assert(broken.active() && !broken.canStartGame());
    economy::State legacyBackup;
    std::ofstream(directory / "profile.dat.bak") << storage::encode(legacyBackup);
    Console backupOnly(root);
    backupOnly.start(true, false, true, false);
    complete(backupOnly);
    assert(backupOnly[8].status == Status::Warn);
    fs::remove(directory / "profile.dat.bak");
    std::ofstream(directory / "profile.dat") << "legacy-corrupt";
    {
        ui::App app(root, true);
        app.init();
        const auto press = [&](unsigned keys) { app.update({keys, false, 0, 0}, 0); };
        if (app.view == ui::View::Language) press(ui::A);
        assert(app.view == ui::View::Profiles);
        press(ui::A);
        assert(app.view == ui::View::ProfileSlot);
        press(ui::A);
        assert(app.wantsName);
        app.setName("Console Test");
        assert(app.view == ui::View::Title); // Failed migration must not disable a new slot.
    }
    std::ifstream old(directory / "profile.dat");
    std::string original;
    old >> original;
    assert(original == "legacy-corrupt" && !storage::Store(root).save(economy::State{}));
    journal().end();
    if (argc == 2) fs::copy_file(directory / "logs/session.log", argv[1], fs::copy_options::overwrite_existing);
    fs::remove_all(directory);
    std::cout << "Diagnostics quick/extended/cancel/English, SD gate, isolated profile, and physical-report regression passed. Host elapsed " << bootMs << " ms (includes fixture I/O and test pacing)\n";
}
