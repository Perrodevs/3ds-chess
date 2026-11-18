#pragma once
#include <string>
namespace ui {
// Optional presentation settings live separately from legacy competitive saves.
struct Preferences {
    bool reducedMotion = false;
    bool pieceInfo = true;
    int styleOverride = -1; // -1 selects the rival profile, 0..5 override it.
    bool load(const std::string &path);
    bool save(const std::string &path) const;
};
}
