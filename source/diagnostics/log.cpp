#include "diagnostics/log.hpp"
#include "app_version.hpp"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <sys/stat.h>

namespace diagnostics {
namespace {
constexpr long MaxLogBytes = 64 * 1024;
constexpr long MaxBatchBytes = 4096;
unsigned seconds() {
    static const auto start = std::chrono::steady_clock::now();
    return unsigned(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start).count());
}
bool present(const std::string &path) {
    struct stat info{};
    return stat(path.c_str(), &info) == 0;
}
}
const char *name(Level level) {
    constexpr const char *names[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    return names[static_cast<unsigned>(level)];
}
Journal &journal() {
    static Journal instance;
    return instance;
}
bool Journal::begin(const std::string &root, const char *language) {
    if (active) return false;
    count = errorCount = nextError = dropped = 0;
    failed = false;
    directory = root + "/logs";
    const auto now = std::time(nullptr);
    const bool plausibleDate = now >= 1577836800;
    const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
    session = static_cast<unsigned long long>(ticks) ^ (plausibleDate ? static_cast<unsigned long long>(now) << 24 : 0);
    active = true;
    if (mkdir(directory.c_str(), 0777) != 0 && errno != EEXIST) {
        failed = true;
        return false;
    }
    const auto marker = directory + "/active";
    const bool previous = present(marker);
    FILE *file = std::fopen(marker.c_str(), "wb");
    if (!file) { failed = true; return false; }
    const bool written = std::fprintf(file, "%llu\n", session) > 0;
    const bool closed = std::fclose(file) == 0;
    if (!written || !closed) { failed = true; return false; }
    add(Level::Info, "SESSION_START", language);
    add(Level::Info, "BUILD", appinfo::version);
#ifdef __3DS__
    add(Level::Info, "TARGET", "ARMv6K");
#else
    add(Level::Info, "TARGET", "HOST");
#endif
    if (plausibleDate) add(Level::Info, "CLOCK", "device_epoch_unverified", int(now <= 2147483647 ? now : 0));
    if (previous) add(Level::Warn, "SESSION_INCOMPLETE", "previous_marker_present");
    return flush();
}
void Journal::add(Level level, const char *code, const char *stage, int value) {
    if (!active) return;
    const Event entry{level, value, seconds(), code, stage};
    if (level >= Level::Warn) {
        recent[nextError] = entry;
        nextError = (nextError + 1) % recent.size();
        errorCount = std::min(errorCount + 1, recent.size());
    }
    if (level < threshold || failed) return;
    if (count == pending.size()) {
        // Preserve space for important errors without touching the SD here.
        if (level < Level::Warn) { ++dropped; return; }
        const auto victim = std::find_if(pending.begin(), pending.end(), [level](const Event &e) {
            return e.level < level;
        });
        ++dropped;
        if (victim == pending.end()) return;
        std::move(victim + 1, pending.end(), victim);
        --count;
    }
    pending[count++] = entry;
}
bool Journal::flush() {
    if (!available()) return false;
    if (!count) return true;
    const auto fail = [&](const char *stage, int error) {
        failed = true;
        add(Level::Error, "JOURNAL_IO_FAILED", stage, error);
        count = dropped = 0;
        return false;
    };
    const auto path = directory + "/session.log";
    const auto old = directory + "/session.previous.log";
    struct stat info{};
    if (stat(path.c_str(), &info) == 0 && info.st_size >= MaxLogBytes - MaxBatchBytes) {
        if ((std::remove(old.c_str()) != 0 && errno != ENOENT) ||
            std::rename(path.c_str(), old.c_str()) != 0) {
            return fail("rotate_log", errno);
        }
    }
    FILE *file = std::fopen(path.c_str(), "ab");
    if (!file) return fail("open_log", errno);
    // A newline also separates a final partial record from the next batch.
    bool ok = std::fprintf(file, "\nSESSION %llu dropped=%u\n", session, dropped) > 0;
    for (size_t i = 0; i < count && ok; ++i) {
        const auto &e = pending[i];
        ok = std::fprintf(file, "+%us [%s] %.32s %.32s result=%d\n",
                          e.seconds, name(e.level), e.code, e.stage, e.value) > 0;
    }
    const int writeError = ok ? 0 : errno;
    const int closed = std::fclose(file);
    if (!ok) return fail("write_log", writeError);
    if (closed != 0) return fail("close_log", errno);
    count = dropped = 0;
    return true;
}
void Journal::end() {
    if (!active) return;
    add(Level::Info, "SESSION_END", "clean_shutdown", int(errorCount));
    if (flush()) std::remove((directory + "/active").c_str());
    active = false;
}
}
