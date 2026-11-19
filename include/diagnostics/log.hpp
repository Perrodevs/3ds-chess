#pragma once
#include <array>
#include <cstddef>
#include <string>

namespace diagnostics {
enum class Level { Debug, Info, Warn, Error, Fatal };
const char *name(Level);
struct Event {
    Level level{};
    int value = 0;
    unsigned seconds = 0;
    const char *code = "";
    const char *stage = "";
};

// Only the UI thread calls the journal. Event identifiers and stages must be
// static, developer-defined strings. Never pass user text, passwords or paths.
class Journal {
    std::string directory;
    std::array<Event, 32> pending{};
    std::array<Event, 16> recent{};
    size_t count = 0, errorCount = 0, nextError = 0;
    unsigned dropped = 0;
    unsigned long long session = 0;
    bool active = false, failed = false;
public:
    Level threshold = Level::Info;
    bool begin(const std::string &root, const char *language);
    void add(Level, const char *code, const char *stage, int value = 0);
    bool flush();
    void end();
    bool available() const { return active && !failed; }
    unsigned long long id() const { return session; }
    size_t errors() const { return errorCount; }
    const Event &error(size_t index) const { return recent[(nextError + recent.size() - 1 - index % recent.size()) % recent.size()]; }
};

Journal &journal();
}
