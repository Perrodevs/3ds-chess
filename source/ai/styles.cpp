#include "ai/styles.hpp"
namespace ai {
namespace {
constexpr StyleWeights weights[] = {
    {0,0,0,0,0,0,0}, {1,5,1,-1,0,-2,0}, {0,-2,0,4,2,1,0},
    {1,0,2,1,3,0,0}, {2,3,1,0,0,-1,0}, {0,0,0,0,1,3,3}
};
constexpr const char *names[] = {"Equilibrado","Agresivo","Defensivo","Posicional","Tactico","Finales"};
int index(Style s) { const int n = int(s); return n >= 0 && n < styleCount ? n : 0; }
}
const char *styleName(Style s) { return names[index(s)]; }
const StyleWeights &styleWeights(Style s) { return weights[index(s)]; }
Style styleForBot(int id) {
    // Stable IDs, preferences for this game only; no claim of authentic imitation.
    static constexpr Style profiles[41] = {
        Style::Balanced,Style::Aggressive,Style::Tactical,Style::Positional,Style::Tactical,
        Style::Balanced,Style::Balanced,Style::Defensive,Style::Positional,Style::Aggressive,
        Style::Defensive,Style::Balanced,Style::Endgame,Style::Tactical,Style::Positional,
        Style::Defensive,Style::Endgame,Style::Tactical,Style::Defensive,Style::Aggressive,
        Style::Positional,Style::Balanced,Style::Defensive,Style::Aggressive,Style::Positional,
        Style::Tactical,Style::Aggressive,Style::Endgame,Style::Tactical,Style::Defensive,
        Style::Endgame,Style::Tactical,Style::Defensive,Style::Positional,Style::Aggressive,
        Style::Defensive,Style::Tactical,Style::Endgame,Style::Positional,Style::Balanced,Style::Aggressive
    };
    return id >= 0 && id < 41 ? profiles[id] : Style::Balanced;
}
} // namespace ai
