#include "ui/app.hpp"
#include <algorithm>
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
    if (view == View::Trophies) { back(); return; }
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
bool App::trophyUnlocked(int i) const {
    if(i==19)return (state.profile.achievements&(1u<<economy::Error404))!=0;
    if (i >= 16 && i < 19) return variantsUI && variantsUI->data().stats[size_t(i-16)].wins > 0;
    if (i < 7) return (state.profile.achievements & (1u << i)) != 0;
    if (i < 11) return (progressionState.promotions & (1u << (i-7+1))) != 0;
    if (i < 14) return (progressionState.survivalRewards & (1u << (i-11))) != 0;
    if (i == 14) return tournamentState.trophies > 0;
    return academyStore.progress.count() == academy::lessonCount;
}
void App::drawProgressionTop(graphics::Renderer &r) {
    r.screenBegin(0,background);
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
    } else if (view == View::Trophies) {
        r.text(12,43,trophyName(trophyIndex),accent,1,49);
        r.text(12,67,trophyUnlocked(trophyIndex)?"DESBLOQUEADO":"BLOQUEADO",textColor,1,49);
        r.text(12,94,"Se concede por eventos guardados del juego.",dim,1,49);
        r.text(12,116,"No vuelve a entregar monedas al consultarlo.",dim,1,49);
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
