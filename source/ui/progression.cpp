#include "ui/app.hpp"
#include <algorithm>
#include <sys/stat.h>
namespace ui {
namespace {
constexpr auto background = graphics::rgb(20,25,36), panelColor = graphics::rgb(32,40,54),
    textColor = graphics::rgb(236,227,192), accent = graphics::rgb(235,185,78),
    dim = graphics::rgb(142,158,169), blueColor = graphics::rgb(90,180,211);
const char *trophyName(int i) {
    static const char *names[] = {"Primera victoria", "10 victorias", "50 partidas", "Vencer avanzado",
        "Primer enroque", "Primera promocion", "Primer jaque mate", "Division Plata", "Division Oro",
        "Division Diamante", "Division Maestro", "Supervivencia: 3", "Supervivencia: 5",
        "Supervivencia: 10", "Campeon de torneo", "Academia completa", "Victoria Chess960", "Victoria Three-check", "Victoria King of the Hill", "ERROR 404"};
    return names[std::clamp(i,0,19)];
}
constexpr int trophyTotal = 20;
uint32_t trophyMask(const economy::Profile &profile, const progression::State &progress,
                    const tournament::Session &tournament, const academy::Progress &academy,
                    const variants::State *variants) {
    uint32_t mask = profile.achievements & 127u;
    mask |= ((progress.promotions >> 1) & 15u) << 7;
    mask |= (progress.survivalRewards & 7u) << 11;
    if (tournament.trophies) mask |= 1u << 14;
    if (academy.count() == academy::lessonCount) mask |= 1u << 15;
    if (variants) {
        for (int i = 0; i < 3; ++i)
            if (variants->stats[size_t(i)].wins) mask |= 1u << (16 + i);
    }
    if (profile.achievements & (1u << economy::Error404)) mask |= 1u << 19;
    return mask;
}
bool hasFile(const std::string &path) {
    struct stat info{};
    return ::stat(path.c_str(), &info) == 0 || ::stat((path + ".bak").c_str(), &info) == 0;
}
}
bool App::reconcileProgressionRewards() {
    if (!progressionReady) return false;
    auto &base = progressionActive ? normalState : state;
    auto candidate = base;
    const auto paid = candidate.profile.legacyCoinsClaimed;
    const auto due = progressionState.totalCoins > paid ? progressionState.totalCoins - paid : 0;
    if (due > 1000000000 - candidate.profile.coins) {
        message("Premio pendiente: libera saldo en tienda"); return false;
    }
    candidate.profile.coins += due;
    candidate.profile.legacyCoinsClaimed = std::max(paid, progressionState.totalCoins);
    candidate.profile.unlocked |= progressionState.entitledItems;
    if (candidate.profile.coins == base.profile.coins && candidate.profile.unlocked == base.profile.unlocked &&
        candidate.profile.legacyCoinsClaimed == paid) return true;
    // The sidecar result is committed first. The cumulative paid marker and the
    // real coin balance are saved together; retrying cannot pay twice.
    if (!store.save(candidate)) { message("Premio pendiente: error SD, se reintentara"); return false; }
    base = std::move(candidate);
    if (progressionActive) state.profile = base.profile;
    if (due) message("Premio Legacy: +" + std::to_string(due) + " CC");
    return true;
}
void App::beginProgression(progression::Kind kind) {
    if (!progressionReady) { message("Datos del modo danados; archivos conservados"); return; }
    if (progressionState.run.active && progressionState.run.game.result == chess::Result::Playing && progressionState.kind != kind) {
        message("Retoma primero la partida del otro modo"); return;
    }
    if (!save()) return;
    auto next = progressionState;
    if (!next.run.active || next.run.game.result != chess::Result::Playing) {
        if (!progression::start(next,kind,state.settings.color) || !progressionStore.save(next)) {
            message("Termina el intento pendiente o revisa la SD"); return;
        }
        progressionState = std::move(next);
    }
    bot.stop(); hints.cancel();
    normalState = state;
    progressionActive = true;
    state.session = progressionState.run;
    selected = -1; cursor = state.session.human == -1 ? 52 : 12;
    claimIntent = false; hintMove = {}; animationFrames = 0;
    refresh(); go(View::Game);
}
void App::leaveProgression() {
    bot.stop(); hints.cancel();
    reconcileProgressionRewards();
    state = normalState;
    progressionActive = false;
    selected = -1; hintMove = {}; claimIntent = false; animationFrames = 0;
    refresh();
}
void App::progressionAction() {
    if (view == View::Modes) {
        if (focus == 0) go(View::Career);
        else if (focus == 1) go(View::Survival);
        else if (focus == 2) { trophyReturn = view; go(View::Trophies); }
        else if (focus == 3) go(View::Openings);
        else if (focus == 4) { if (save()) go(View::ComposerWorkshop); }
        else if (focus == 5) { if (save()) go(View::VariantWorkshop); }
        else back();
        return;
    }
    if (view == View::Trophies || view == View::TrophyLeaderboard) { back(); return; }
    if (view == View::ConfirmEndAttempt) {
        if (focus != 0) { back(); return; }
        auto candidate = progressionState;
        if (!progressionReady || candidate.kind != progression::Kind::Survival ||
            !progression::abandon(candidate) || !progressionStore.save(candidate)) {
            message("No se pudo cerrar el intento; se conserva"); return;
        }
        progressionState = std::move(candidate);
        reconcileProgressionRewards(); go(View::Survival); return;
    }
    const bool career = view == View::Career;
    if (focus == 0) beginProgression(career ? progression::Kind::Career : progression::Kind::Survival);
    else if (focus == 3) back();
    else if (career && focus == 2) { trophyReturn = view; go(View::Trophies); }
    else if (!career && focus == 1) {
        if (progressionState.kind == progression::Kind::Survival && progressionState.activeAttempt) go(View::ConfirmEndAttempt);
        else message("No hay un intento activo");
    } else message(career ? "V +3 / T +1 / D -1. Premios solo primer ascenso" :
                            "Gana para seguir; tablas o derrota terminan intento");
}
bool App::trophyUnlocked(int index) const {
    const auto mask = trophyMask(state.profile, progressionState, tournamentState,
                                 academyStore.progress, variantsUI ? &variantsUI->data() : nullptr);
    return index >= 0 && index < trophyTotal && (mask & (1u << index));
}
void App::readLeaderboard() {
    trophyRows = {};
    trophyCount = trophyFocus = unreadableProfiles = 0;
    for (int slot = 0; slot < (profilesEnabled ? profiles::capacity : 1); ++slot) {
        const auto &entry = profileManager.slots[size_t(slot)];
        if (profilesEnabled && !entry.occupied) continue;
        const auto path = profilesEnabled ? profileManager.path(slot) : dataDirectory;
        economy::State profile;
        if (!storage::Store(path).load(profile)) {
            if (profilesEnabled) ++unreadableProfiles;
            continue;
        }
        progression::State progress;
        tournament::Session tournament;
        academy::Store academy(path + "/academy.dat");
        variants::State variants;
        const auto progressPath = path + "/progression.dat";
        const auto tournamentPath = path + "/tournament.dat";
        bool partial = !progression::Store(progressPath).load(progress) && hasFile(progressPath);
        partial |= !tournament::Store(tournamentPath).load(tournament) && hasFile(tournamentPath);
        partial |= !academy.load() && hasFile(path + "/academy.dat");
        variants::Store variantsStore(path + "/variants.dat");
        const bool hasVariants = variantsStore.load(variants);
        partial |= variantsStore.corrupt;
        auto &row = trophyRows[size_t(trophyCount++)];
        row.name = profilesEnabled ? entry.name : profile.profile.name;
        row.slot = slot;
        row.avatar = entry.avatar;
        row.division = progression::division(progress.points);
        row.wins = tournament.trophies;
        row.survival = progress.survivalBest;
        row.unlocked = trophyMask(profile.profile, progress, tournament, academy.progress,
                                  hasVariants ? &variants : nullptr);
        for (int i = 0; i < trophyTotal; ++i) row.total += (row.unlocked >> i) & 1u;
        row.partial = partial;
    }
    std::sort(trophyRows.begin(), trophyRows.begin() + trophyCount,
        [](const TrophyRow &a, const TrophyRow &b) {
            return a.total != b.total ? a.total > b.total : a.slot < b.slot;
        });
}
void App::drawProgressionTop(graphics::Renderer &r) {
    r.screenBegin(0,background);
    if (view == View::TrophyLeaderboard) {
        r.text(12, 12, "TABLA DE TROFEOS", accent, 2, 31);
        for (int rank = 0; rank < trophyCount; ++rank) {
            const auto &row = trophyRows[size_t(rank)];
            const int y = 42 + rank * 56;
            const auto medal = rank == 0 ? accent : rank == 1 ? textColor : graphics::rgb(194,126,83);
            r.rect(12, y, 376, 49, panelColor);
            if (rank == trophyFocus) r.border(12, y, 376, 49, blueColor);
            r.rect(24, y + 9, 14, 11, medal);
            r.rect(29, y + 20, 4, 5, medal);
            r.rect(25, y + 25, 12, 2, medal);
            r.rawText(26, y + 34, std::to_string(rank + 1), medal);
            r.portrait(row.avatar, 0, 52, y + 8, 1);
            r.rawText(96, y + 10, row.name, textColor, 1, 24);
            r.rawText(296, y + 10, std::to_string(row.total) + " / " + std::to_string(trophyTotal), medal);
            r.rect(96, y + 30, 252, 4, dim);
            if (row.total) r.rect(96, y + 30, row.total * 252 / trophyTotal, 4, medal);
        }
        if (!trophyCount) r.text(12, 62, "Sin perfiles disponibles", dim);
        if (unreadableProfiles) r.text(12, 219, "Algunas ranuras no se pudieron leer.", dim);
        return;
    }
    if (view == View::Trophies) {
        r.text(12,9,"SALA DE TROFEOS",accent,2,30);
        for (int i=0;i<20;++i) {
            int x=9+(i%4)*97, y=33+(i/4)*39;
            bool unlocked=trophyUnlocked(i);
            r.rect(x,y,91,36,panelColor);
            const auto color=unlocked?accent:dim;
            r.rect(x+34,y+3,22,10,color); r.rect(x+39,y+13,12,3,color);
            r.rect(x+43,y+16,4,5,color); r.rect(x+34,y+21,22,2,color);
            r.rect(x+29,y+5,5,7,color); r.rect(x+56,y+5,5,7,color);
            r.text(x+5,y+26,std::to_string(i+1)+(unlocked?" OK":" --"),color,1,13);
            if (i==trophyIndex) r.border(x,y,91,36,blueColor);
        }
        return;
    }
    r.text(12,15,view==View::Career?"CAMINO AL GRAN MAESTRO":view==View::Modes?"MODOS LEGACY":"SUPERVIVENCIA",accent,2,31);
    r.text(12,56,"PROGRESO OFFLINE / POR JUGADOR",dim,1,49);
    const int division=progression::division(progressionState.points);
    r.text(12,87,"CARRERA: " + std::string(progression::divisionName(division)),textColor,2,30);
    r.text(12,119,"Puntos internos. No son Elo ni FIDE.",dim,1,49);
    r.text(12,157,"MEJOR RACHA: " + std::to_string(progressionState.survivalBest),accent,2,30);
    r.text(12,196,"Tu partida normal se conserva aparte.",dim,1,49);
}
void App::drawProgressionBottom(graphics::Renderer &r) {
    auto menu=[&](const std::vector<std::string> &labels) {
        for(int i=0;i<int(labels.size());++i) button(r,12,rowY()+i*rowH(),296,labels[size_t(i)],i==focus);
    };
    if (view == View::Modes) {
        menu({"Carrera: camino al gran maestro","Supervivencia","Sala de trofeos","Explorador de aperturas","Editor y puzzles creados","Variantes de ajedrez","Volver"});
        r.text(12,214,"Cada modo guarda por separado.",dim,1,49);

        if (!progressionReady) r.text(12,225,"Datos del modo no disponibles.",accent,1,49);
    } else if (view == View::TrophyLeaderboard) {
        for (int rank = 0; rank < trophyCount; ++rank) {
            const auto &row = trophyRows[size_t(rank)];
            button(r, 12, 36 + 22 * rank, 296, "#" + std::to_string(rank + 1) + "  " +
                row.name + "  " + std::to_string(row.total) + "/" + std::to_string(trophyTotal),
                rank == trophyFocus, true);
        }
        if (trophyCount) {
            const auto &row = trophyRows[size_t(trophyFocus)];
            r.text(12, 111, "DIVISION: " + std::string(progression::divisionName(row.division)), accent);
            r.text(12, 126, "Torneos ganados: " + std::to_string(row.wins), textColor);
            r.text(12, 141, "MEJOR RACHA: " + std::to_string(row.survival), textColor);
            int y = 159;
            if (row.partial) r.text(12, y, "Progreso parcial: hay datos ilegibles.", dim);
            else {
                for (int i = trophyTotal - 1; i >= 0 && y < 192; --i) {
                    if (!(row.unlocked & (1u << i))) continue;
                    r.text(12, y, trophyName(i), dim);
                    y += 11;
                }
            }
        }
        menu({"Volver"});
    } else if (view == View::Trophies) {
        r.text(12,43,trophyName(trophyIndex),accent,1,49);
        r.text(12,67,trophyUnlocked(trophyIndex)?"DESBLOQUEADO":"BLOQUEADO",textColor,1,49);
        r.text(12,94,"Se concede por eventos guardados del juego.",dim,1,49);
        r.text(12,116,"No vuelve a entregar monedas al consultarlo.",dim,1,49);
        button(r,12,142,296,"TABLA DE TROFEOS [SELECT]",false);
        button(r,12,170,140,"< Anterior",false); button(r,166,170,142,"Siguiente >",false);
        menu({"Volver"});
    } else if (view == View::ConfirmEndAttempt) {
        r.text(12,59,"TERMINAR SUPERVIVENCIA?",accent,1,49);
        r.text(12,82,"Partida activa: cuenta como rendicion.",dim,1,49);
        r.text(12,100,"El record se conserva. B cancela.",dim,1,49);
        menu({"Si, terminar intento","Cancelar"});
    } else {
        const bool career=view==View::Career;
        const auto &c=career?progressionState.career:progressionState.survival;
        r.text(12,42,career?"DIVISION: "+std::string(progression::divisionName(progression::division(progressionState.points))):
                         "RACHA ACTUAL: "+std::to_string(progressionState.survivalCurrent),accent,1,49);
        r.text(12,60,career?"Puntos: "+std::to_string(progressionState.points):"Mejor: "+std::to_string(progressionState.survivalBest)+" / Intentos: "+std::to_string(progressionState.survivalAttempts),textColor,1,49);
        r.text(12,78,"V "+std::to_string(c.wins)+" / D "+std::to_string(c.losses)+" / T "+std::to_string(c.draws),dim,1,49);
        if (career) {
            r.text(12,95,"Mejor division: "+std::string(progression::divisionName(progressionState.bestDivision)),dim,1,49);
            static const int targets[]={9,21,36,54}; const int d=progression::division(progressionState.points);
            r.text(12,110,d==4?"Division maxima alcanzada":"Siguiente: "+std::to_string(targets[d])+" puntos",dim,1,49);
        } else {
            const bool pending=progressionState.kind==progression::Kind::Survival && progressionState.run.active && progressionState.run.game.result==chess::Result::Playing;
            r.text(12,95,pending?"Rival: "+std::string(ai::botAt(progressionState.run.bot).name):"Proximo nivel: "+std::to_string(std::min<uint64_t>(5,progressionState.survivalCurrent/2)),dim,1,49);
            r.text(12,110,"Partidas terminadas: "+std::to_string(c.wins+c.losses+c.draws),dim,1,49);
        }
        menu({"Jugar / retomar partida",career?"Reglas y recompensas":"Terminar intento...",career?"Consultar trofeos":"Reglas del modo","Volver"});
    }
}
} // namespace ui
