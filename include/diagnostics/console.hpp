#pragma once
#include "ai/worker.hpp"
#include "graphics/renderer.hpp"
#include <array>
#include <chrono>
#include <string>

namespace diagnostics {
enum class Status { Pass, Fail, Warn, Skip };
struct Check {
    const char *code = "";
    Status status = Status::Skip;
    int result = 0;
    const char *stage = "";
};
class Console {
    std::string root;
    std::array<Check, 10> checks{};
    size_t count = 0, step = 0;
    bool running = false, extended = false, boot = true, summary = false;
    bool graphics = false, audio = false, safe = false, ready = false;
    int mode = 0, selection = 0, offset = 0;
    ai::Worker worker;
    chess::Game position;
    std::chrono::steady_clock::time_point deadline;
    void add(const char *, Status, const char *, int = 0);
    void tick();
public:
    explicit Console(std::string directory) : root(std::move(directory)) {}
    bool active() const { return mode != 0; }
    bool canStartGame() const { return ready && safe && !active(); }
    bool completed() const { return !running; }
    size_t size() const { return count; }
    const Check &operator[](size_t n) const { return checks[n]; }
    void start(bool startup, bool full, bool graphicsReady, bool audioReady);
    void menu();
    void update(bool up, bool down, bool a, bool b);
    void cancel();
    void draw(graphics::Renderer &) const;
};
}
