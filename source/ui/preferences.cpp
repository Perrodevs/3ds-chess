#include "ui/preferences.hpp"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <unistd.h>
namespace ui {
static bool read(const std::string &path, Preferences &out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    char bytes[10];
    f.read(bytes, sizeof(bytes));
    if (f.bad() || f.gcount() == 10) return false;
    std::string data(bytes, size_t(f.gcount()));
    if (data.size() == 8 && data.substr(0, 4) == "UI01") {
        if ((data[4] != '0' && data[4] != '1') || (data[5] != '0' && data[5] != '1') ||
            data[6] != char(data[4] ^ data[5]) || data[7] != '\n') return false;
    } else if (data.size() == 9 && data.substr(0, 4) == "UI02") {
        if ((data[4] != '0' && data[4] != '1') || (data[5] != '0' && data[5] != '1') ||
            data[6] < '0' || data[6] > '6' || data[7] != char(data[4] ^ data[5] ^ data[6]) || data[8] != '\n') return false;
        out.styleOverride = data[6] - '0' - 1;
    } else return false;
    out.reducedMotion = data[4] == '1'; out.pieceInfo = data[5] == '1';
    return true;
}
bool Preferences::load(const std::string &path) {
    Preferences candidate;
    if (!read(path, candidate) && !read(path + ".bak", candidate)) return false;
    *this = candidate; return true;
}
bool Preferences::save(const std::string &path) const {
    if (styleOverride < -1 || styleOverride > 5) return false;
    std::string data = "UI02";
    data += reducedMotion ? '1' : '0'; data += pieceInfo ? '1' : '0';
    data += char('0' + styleOverride + 1);
    data += char(data[4] ^ data[5] ^ data[6]); data += '\n';
    auto tmp = path + ".tmp";
    FILE *f = std::fopen(tmp.c_str(), "wb");
    if (!f) return false;
    bool ok = std::fwrite(data.data(), 1, data.size(), f) == data.size();
    if (std::fflush(f) != 0 || fsync(fileno(f)) != 0) ok = false;
    if (std::fclose(f) != 0) ok = false;
    Preferences prior;
    if (ok && read(path, prior) && std::rename(path.c_str(), (path + ".bak").c_str()) != 0) ok = false;
    if (ok && std::rename(tmp.c_str(), path.c_str()) == 0) return true;
    std::remove(tmp.c_str()); return false;
}
}
