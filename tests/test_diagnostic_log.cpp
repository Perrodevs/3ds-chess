#include "diagnostics/log.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <csignal>
#include <sys/resource.h>
#include <sys/wait.h>
namespace fs = std::filesystem;
using namespace diagnostics;
static std::string read(const fs::path &path) {
    std::ifstream in(path);
    return {std::istreambuf_iterator<char>(in), {}};
}
int main() {
    char root[] = "/tmp/chess-journal-XXXXXX";
    assert(mkdtemp(root));
    fs::path logs = fs::path(root) / "logs";
    Journal first;
    assert(first.begin(root, "ES"));
    first.add(Level::Debug, "FILTERED", "detail");
    for (auto level : {Level::Info, Level::Warn, Level::Error, Level::Fatal})
        first.add(level, "EVENT", "test", 28);
    first.threshold = Level::Debug;
    first.add(Level::Debug, "DEBUG_VISIBLE", "test");
    assert(first.flush() && first.errors() == 3);
    auto content = read(logs / "session.log");
    for (auto marker : {"[DEBUG]", "[INFO]", "[WARN]", "[ERROR]", "[FATAL]", "result=28"})
        assert(content.find(marker) != std::string::npos);
    assert(content.find("FILTERED") == std::string::npos);
    Journal second;
    assert(second.begin(root, "EN"));
    assert(read(logs / "session.log").find("SESSION_INCOMPLETE") != std::string::npos);
    std::ofstream(logs / "session.log", std::ios::app) << "partial";
    second.add(Level::Error, "NEXT", "after_truncation");
    assert(second.flush());
    assert(read(logs / "session.log").find("partial\nSESSION") != std::string::npos);
    fs::resize_file(logs / "session.log", 64000);
    second.add(Level::Error, "ROTATED", "test");
    assert(second.flush() && fs::exists(logs / "session.previous.log"));
    second.end();
    assert(!fs::exists(logs / "active"));
    fs::remove(logs / "session.log");
    fs::create_directory(logs / "session.log");
    Journal blocked;
    assert(!blocked.begin(root, "ES"));
    blocked.add(Level::Error, "VISIBLE_IN_RAM", "open_failure");
    assert(blocked.errors() == 1 && !blocked.flush());
    fs::remove_all(logs / "session.log");
    Journal limited;
    assert(limited.begin(root, "ES"));
    const pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        std::signal(SIGXFSZ, SIG_IGN);
        const rlimit zero{0, 0};
        if (setrlimit(RLIMIT_FSIZE, &zero) != 0) _exit(2);
        limited.add(Level::Error, "CAPACITY", "simulated_write_exhaustion");
        if (limited.flush() || limited.available()) _exit(3);
        limited.add(Level::Fatal, "AFTER_IO_FAILED", "retained_in_memory");
        _exit(!limited.errors() || limited.flush() ? 4 : 0);
    }
    int status = 0;
    assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    limited.end();
    fs::remove_all(root);
    std::cout << "Diagnostic log: filtering, severity, truncation, rotation, marker and failed open passed\n";
}
