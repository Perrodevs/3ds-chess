#include "diagnostics/log.hpp"
#include "localization/localization.hpp"
#include "ui/app.hpp"
#include "app_version.hpp"
#include "ai/bots.hpp"
#include <algorithm>
#include <fstream>
#include <cstdlib>
#include <utility>
namespace ui {
using namespace graphics;
static constexpr Color bg = rgb(20, 25, 36), panel = rgb(32, 40, 54), ink = rgb(236, 227, 192),
                       muted = rgb(142, 158, 169), gold = rgb(235, 185, 78),
                       blue = rgb(90, 180, 211), red = rgb(236, 110, 111);
static const char *modes[] = {"Jugador vs IA", "Dos jugadores", "Practica: deshacer"};
// The tiny bitmap font does not wrap automatically.
static void paragraph(Renderer &r, int x, int y, const std::string &text, Color color,
                      int columns, int lines, bool literal=false) {
    std::string remaining = literal?text:localization::translate(text);
    for (int line = 0; line < lines && !remaining.empty(); ++line) {
        size_t length = std::min(remaining.size(), size_t(columns));
        if (length < remaining.size()) {
            size_t space = remaining.rfind(' ', length);
            if (space != std::string::npos && space > 0) length = space;
        }
        r.rawText(x, y + line * 11, remaining.substr(0, length), color, 1, columns);
        remaining.erase(0, length);
        while (!remaining.empty() && remaining.front() == ' ') remaining.erase(0, 1);
    }
}
void App::recordCompleted() {
    syncSecret();
    syncNull();
    if (!archiveReady || developerSandbox || progressionActive || openingActive || trainingActive || tournamentActive || !state.session.active ||
        state.session.game.result == chess::Result::Playing) return;
    auto candidate = archiveState;
    const auto result = archive::record(candidate, state.session, state.profile.name);
    if (result == archive::RecordResult::Invalid || result == archive::RecordResult::AlreadyRecorded) return;
    if (!archiveStore.save(candidate)) { message("Error SD: biblioteca pendiente de guardar"); return; }
    archiveState = std::move(candidate);
    if (result == archive::RecordResult::Full) message("Biblioteca llena: elimina una partida para guardar");
}
void App::openLibraryReview() {
    if (archiveState.games.empty()) return;
    const auto &entry = archiveState.games[size_t(libraryIndex)];
    if (!entry.replay(entry.moves.size(), reviewSource)) { message("Partida archivada invalida"); return; }
    reviewHuman = entry.human; reviewMode = entry.mode;
    reviewReturn = View::Library;
    go(View::Review);
    reviewExplanation = review::explain(reviewSource, entry.human);
    seekReview(int(reviewSource.moves.size()));
}
void App::leaveTournament() {
    tournamentController.stop(); bot.stop(); hints.cancel();
    state = normalState;
    tournamentActive = false;
    selected = -1; hintMove = {}; animationFrames = 0; claimIntent = false;
    refresh();
}
void App::beginTournamentMatch() {
    if (!tournamentState.playerMatch()) return;
    if (!save()) return;
    normalState = state;
    const int entrant = tournamentState.humanColor() == 1 ? tournamentState.blackEntrant() : tournamentState.whiteEntrant();
    const int rival = tournamentState.entrants[entrant];
    tournamentActive = true;
    economy::start(state, 2, ai::botAt(rival).engineLevel, tournamentState.humanColor(), rival);
    state.session.game = tournamentState.game;
    state.session.clock.reset(0);
    selected = -1; cursor = state.session.human == 1 ? 12 : 52;
    hintMove = {}; animationFrames = 0; claimIntent = false;
    refresh(); go(View::Game);
}
void App::beginAcademy() {
    if (trainingActive) leaveTraining();
    if (!academyStore.progress.unlocked(lessonIndex)) { message("Completa la leccion anterior primero"); return; }
    if (!save()) return;
    if (!academyRun.start(lessonIndex)) { message("Leccion no disponible"); return; }
    normalState = state;
    trainingActive = true; trainingProgressSaved = true;
    economy::start(state, 2, 0, 1, 37);
    state.session.game = academyRun.game();
    state.session.human = state.session.game.position.side;
    state.session.clock.reset(0);
    selected = -1; cursor = state.session.human == 1 ? 12 : 52;
    hintMove = {}; animationFrames = 0; claimIntent = false;
    refresh(); go(View::Game);
    message(academy::lesson(lessonIndex).objective);
}
int App::lessonCount() const {
    if (academyMode) return academy::lessonCount;
    return trainingKind == training::Kind::Campaign ? training::chapterCount :
           trainingKind == training::Kind::Challenge ? training::challengeCount : training::puzzleCount;
}
void App::leaveTraining() {
    bot.stop();
    hints.cancel();
    state = normalState;
    trainingActive = false;
    claimIntent = false;
    selected = -1;
    hintMove = {};
    animationFrames = 0;
    refresh();
}
void App::beginLesson() {
    if (academyMode) { beginAcademy(); return; }
    if (trainingActive) leaveTraining();
    if (!trainingStore.progress.unlocked(trainingKind, lessonIndex)) {
        message("Completa la etapa anterior primero");
        go(View::Lessons);
        return;
    }
    // Persist the real game before starting a temporary training session.
    if (!save()) return;
    normalState = state;
    trainingActive = true;
    claimIntent = false;
    trainingProgressSaved = true;
    const bool campaign = trainingKind == training::Kind::Campaign;
    const int rival = campaign ? training::chapter(lessonIndex).botId :
                                training::exercise(trainingKind, lessonIndex).botId;
    economy::start(state, 2, ai::botAt(rival).engineLevel, state.settings.color, rival);
    state.session.clock.reset(0);
    if (!campaign) {
        if (!trainingRun.start(trainingKind, lessonIndex)) {
            leaveTraining();
            message("No se pudo abrir este reto");
            return;
        }
        state.session.game = trainingRun.game();
        state.session.human = state.session.game.position.side;
    }
    selected = -1;
    cursor = state.session.human == -1 ? 52 : 12;
    hintMove = {};
    animationFrames = 0;
    reaction = reactionFrames = 0;
    refresh();
    go(View::Game);
    message(ai::botDialogue(rival, campaign ? ai::Dialogue::Greeting : ai::Dialogue::PuzzleStart));
}
void App::trainingComplete() {
    if (!trainingActive) return;
    if (academyMode) {
        if (!academyRun.completed()) return;
        const auto previous = academyStore.progress;
        academyStore.progress.markComplete(lessonIndex);
        trainingProgressSaved = academyStore.save();
        if (!trainingProgressSaved) academyStore.progress = previous;
        return;
    }
    if (trainingKind != training::Kind::Campaign && !trainingRun.completed()) return;
    if (trainingKind == training::Kind::Campaign) {
        const auto result = state.session.game.result;
        const bool win = (result == chess::Result::WhiteWin && state.session.human == 1) ||
                         (result == chess::Result::BlackWin && state.session.human == -1);
        if (!win) return;
    }
    const auto previous = trainingStore.progress;
    trainingStore.progress.markComplete(trainingKind, lessonIndex);
    trainingProgressSaved = trainingStore.save();
    if (!trainingProgressSaved) {
        trainingStore.progress = previous;
        message("Error SD: progreso del reto no guardado");
    }
}
void App::rematch() {
    if (trainingActive) { beginLesson(); return; }
    if (!save()) return;
    auto previous = state;
    bot.stop();
    hints.cancel();
    const auto session = state.session;
    economy::start(state, session.mode, session.level, session.human, session.bot);
    state.session.clock.reset(session.clock.preset);
    claimIntent = false;
    if (!commit(previous)) return;
    selected = -1;
    cursor = state.session.human == -1 ? 52 : 12;
    hintMove = {};
    animationFrames = 0;
    reaction = reactionFrames = 0;
    refresh();
    go(View::Game);
    message(ai::botDialogue(state.session.bot, ai::Dialogue::Greeting));
}
void App::openReview(View origin) {
    reviewSource = state.session.game;
    reviewHuman = state.session.human; reviewMode = state.session.mode;
    reviewReturn = origin;
    noticeFrames = 0;
    go(View::Review);
    if (view != View::Review) return; // A clock flag may have opened Result.
    reviewExplanation = review::explain(reviewSource, state.session.human);
    seekReview(int(reviewSource.moves.size()));
}
void App::seekReview(int ply) {
    const size_t bounded = size_t(std::clamp(ply, 0, int(reviewSource.moves.size())));
    if (!review::positionAt(reviewSource, bounded, reviewGame)) {
        message("No se pudo reconstruir la partida");
        return;
    }
    reviewPly = bounded;
    reviewCaptured = review::captured(reviewSource, reviewPly);
    reviewLines = review::analyze(reviewSource, reviewPly);
}
void App::undoPractice() {
    if (state.session.mode != 2 || trainingActive || tournamentActive || progressionActive || openingActive) {
        message("Deshacer: elige el modo Practica");
        return;
    }
    bot.stop();
    hints.cancel();
    auto previous = state;
    if (!review::undoPractice(state.session.game, state.session.human, true)) {
        message("No hay una decision que deshacer");
        return;
    }
    state.session.clock.reset(0);
    if (!commit(previous)) return;
    selected = -1;
    cursor = state.session.human == -1 ? 52 : 12;
    hintMove = {};
    animationFrames = 0;
    claimIntent = false;
    refresh();
    go(View::Game);
    message("Decision deshecha. Prueba otra jugada.");
}
static std::string capturesLine(const review::Captured &captured, int bySide) {
    const auto &pieces = bySide == 1 ? captured.black : captured.white;
    std::string text = bySide == 1 ? "CAPTURO DORADO: " : "CAPTURO AZUL: ";
    const char names[] = " PCATDK";
    bool any = false;
    for (int piece = chess::Pawn; piece <= chess::Queen; ++piece) {
        if (!pieces[piece]) continue;
        any = true;
        text += names[piece];
        text += std::to_string(pieces[piece]);
        text += ' ';
    }
    return text + (any ? "" : "--");
}
namespace {
const char *diagnosticArea(View view) {
    switch (view) {
    case View::Profiles: case View::ProfileSlot: case View::ConfirmProfileDelete: return "profiles";
    case View::Game: case View::Promotion: case View::Pause: return "game";
    case View::Result: return "result_scene";
    case View::Training: case View::Lessons: case View::TrainingEnd: return "training_campaign_academy";
    case View::Tournament: return "tournament";
    case View::Career: return "career";
    case View::Survival: return "survival";
    case View::Shop: return "shop_inventory";
    case View::Library: case View::ConfirmDelete: return "library";
    case View::Trophies: return "trophies";
    case View::Language: return "language";
    case View::ComposerWorkshop: return "composer";
    case View::VariantWorkshop: return "variants";
    case View::Developer: return "developer";
    default: return "menu";
    }
}
}
void App::message(const std::string &s) {
    // Never log UI text. Some notices contain profile names or other player data.
    if (s.rfind("Error", 0) == 0 || s.rfind("No se pudo", 0) == 0 ||
        s.find("sin guardar") != std::string::npos || s.find("danad") != std::string::npos)
        diagnostics::journal().add(diagnostics::Level::Error, "APP_OPERATION_FAILED", diagnosticArea(view));
    notice = s;
    noticeFrames = 240;
}
void App::init() {
    currentLanguage=globalLanguage=localization::packageDefault;
    firstLanguage=profilesEnabled&&!localization::loadLanguage(rootDirectory+"/global-language.dat",globalLanguage);
    currentLanguage=globalLanguage;localization::setLanguage(currentLanguage);
    if (!audioReady) prepareAudio();
    if (profilesEnabled) {
        profilesReady = profileManager.init();
        if (profilesReady && !profileManager.migrate()) {
            diagnostics::journal().add(diagnostics::Level::Warn, "PROFILE_MIGRATION_DEFERRED", "originals_preserved");
            message("Migracion pendiente: originales conservados");
        }
        view = firstLanguage?View::Language:View::Profiles;
        if(firstLanguage)focus=currentLanguage==localization::Language::English?1:0;
        if (!profilesReady) message("Perfiles: " + profileManager.error);
        refresh(); lastClockSample = clockNow();
        return;
    }
    loadData();
}
void App::loadData() {
    state = economy::State{};
    preferences = Preferences{};
    trainingStore.progress = {}; academyStore.progress = {};
    tournamentState = {}; archiveState = {}; archiveReady = false;
    progressionState = {}; progressionReady = false;
    bool loaded = store.load(state);
    if (store.recovered)
        message("Respaldo recuperado");
    else if (!loaded)
        message("Perfil nuevo: bienvenido");
    trainingStore.load();
    academyStore.load();
    if (trainingStore.recovered) diagnostics::journal().add(diagnostics::Level::Warn, "TRAINING_BACKUP_USED", "puzzles_campaign");
    if (academyStore.recovered) diagnostics::journal().add(diagnostics::Level::Warn, "ACADEMY_BACKUP_USED", "academy");
    preferences.load(dataDirectory + "/ui.dat");
    currentLanguage=globalLanguage;
    if(!localization::loadLanguage(dataDirectory+"/language.dat",currentLanguage)){
        if(!std::ifstream(dataDirectory+"/language.dat")&&!std::ifstream(dataDirectory+"/language.dat.bak"))localization::saveLanguage(dataDirectory+"/language.dat",currentLanguage);
        else message("Archivo de idioma danado; elige de nuevo");
    }
    localization::setLanguage(currentLanguage);
    diagnostics::journal().add(diagnostics::Level::Info, "PROFILE_LANGUAGE",
        currentLanguage == localization::Language::English ? "EN" : "ES");
    tournamentStore.load(tournamentState);
    archiveReady = archiveStore.load(archiveState);
    if (!archiveReady && !archiveStore.corrupt) {
        archive::initialize(archiveState, state.profile.paidId);
        archiveReady = archiveStore.save(archiveState);
    }
    if (archiveStore.corrupt) message("Biblioteca danada: perfil normal conservado");
    if (archiveReady) {
        // A profile restored from its older backup must not reuse an ID already
        // committed by the independent archive/statistics store.
        state.profile.nextId = std::max(state.profile.nextId,
            std::max(archiveState.lastStatisticsId, archiveState.lastArchivedId) + 1);
        recordCompleted();
    }
    progressionReady = progressionStore.load(progressionState);
    if (!progressionReady && !progressionStore.exists && !progressionStore.corrupt) progressionReady = progressionStore.save(progressionState);
    if (progressionReady) reconcileProgressionRewards();
    openingReady = openingStore.load();
    if (!openingReady && !openingStore.exists && !openingStore.corrupt) openingReady = openingStore.save();
    openingIndex = openingStore.progress.lastOpening; openingBranch = 0;
    secretPending=false;spaceState={};spaceReady=spaceStore.load(spaceState);pendingScene=space::Scene::None;unlockFrames=0;
    if(!spaceReady&&!spaceStore.exists&&!spaceStore.corrupt){space::initialize(spaceState,state.profile.paidId,archiveReady?&archiveState:nullptr);spaceReady=spaceStore.save(spaceState);}
    nullPending=false;nullState={};nullReady=nullStore.load(nullState);nullUnlockFrames=pendingNullScene=0;pendingNullReward=false;
    if(!nullReady&&!nullStore.exists&&!nullStore.corrupt){
        if(nullboss::initialize(nullState,state.profile.paidId,archiveReady?&archiveState:nullptr))nullReady=nullStore.save(nullState);
    }
    if(nullReady)state.profile.nextId=std::max(state.profile.nextId,nullState.lastId+1);
    if(nullStore.corrupt)message("NULL: datos danados, perfil conservado");
    syncSecret();syncNull();
    composerUI = std::make_unique<ComposerUI>(dataDirectory);
    variantsUI = std::make_unique<VariantsUI>(dataDirectory);
    refresh();
    lastClockSample = clockNow();
}
void App::refresh() {
    history = state.session.game.notation(state.settings.spanish);
    gameCaptured = review::captured(state.session.game, state.session.game.moves.size());
}
bool App::clockRunning() const {
    return !clockSuspended && state.session.active && state.session.clock.preset != 0 &&
           state.session.game.result == chess::Result::Playing &&
           (view == View::Game || view == View::Promotion);
}
void App::advanceClock(int64_t elapsedMs) {
    if (!clockRunning() || elapsedMs <= 0)
        return;
    const int side = state.session.game.position.side;
    if (!state.session.clock.advance(side, elapsedMs))
        return;
    bot.stop();
    hints.cancel();
    hintMove = {};
    auto previousProfile = state.profile;
    chessclock::adjudicateTimeout(state.session.game, side);
    diagnostics::journal().add(diagnostics::Level::Info, "CLOCK_FLAG", "adjudicated", side);
    selected = -1;
    claimIntent = false;
    if (view == View::Promotion)
        view = View::Game;
    finish();
    if (!store.save(state)) {
        // A flag remains terminal even if the SD cannot persist it. Retry its reward on save.
        state.profile = previousProfile;
        message("Tiempo agotado; error al guardar");
    } else recordCompleted();
    lastClockSample = clockNow();
    auto result = state.session.game.result;
    bool humanWon = (result == chess::Result::WhiteWin && state.session.human == 1) ||
                    (result == chess::Result::BlackWin && state.session.human == -1);
    audio.play(result == chess::Result::Draw ? 0 : humanWon ? 4 : 5, state.settings.sound);
    showResult();
}
void App::syncClock() {
    if (explicitClockStep) {
        lastClockSample = clockNow();
        return;
    }
    auto now = clockNow();
    if (!clockRunning()) {
        lastClockSample = now;
        return;
    }
    int64_t elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - lastClockSample).count();
    // Retain the fractional millisecond across frames instead of slowing the clock at 60fps.
    lastClockSample += std::chrono::milliseconds(elapsed);
    advanceClock(elapsed);
}
bool App::commit(const economy::State &previous) {
    if (openingActive) return true;
    if (progressionActive) {
        auto next = progressionState;
        if (!progression::submit(next, state.session.game) || !progressionStore.save(next)) {
            state = previous; refresh(); message("Error SD: jugada del modo sin guardar"); return false;
        }
        progressionState = std::move(next);
        syncNull();
        reconcileProgressionRewards();
        if (state.session.game.result != chess::Result::Playing) showResult();
        return true;
    }
    if (developerSandbox) {
        if (previous.session.game.result == chess::Result::Playing && state.session.game.result != chess::Result::Playing) showResult();
        return true;
    }
    if (tournamentActive) {
        auto next = tournamentState;
        if (!tournament::submitPlayerGame(next, state.session.game) || !tournamentStore.save(next)) {
            state = previous; refresh(); message("Error SD: jugada del torneo no guardada"); return false;
        }
        // The tournament's administrative ply cap can resolve an otherwise
        // ongoing chess game. Keep that terminal result visible until returning
        // to the bracket, rather than accepting a 241st move into a new fixture.
        if (state.session.game.result == chess::Result::Playing &&
            (next.activeIndex != tournamentState.activeIndex ||
             next.matches[size_t(tournamentState.activeIndex)].games != tournamentState.matches[size_t(tournamentState.activeIndex)].games)) {
            state.session.game.result = chess::Result::Draw;
            state.session.game.reason = "Limite de torneo: 240 medias jugadas";
        }
        tournamentState = std::move(next);
        if (state.session.game.result != chess::Result::Playing) showResult();
        return true;
    }
    if (trainingActive) {
        if (previous.session.game.result == chess::Result::Playing &&
            state.session.game.result != chess::Result::Playing) {
            trainingComplete();
            showResult();
        }
        return true;
    }
    bool ok = store.save(state);
    lastClockSample =
        clockNow(); // SD I/O is outside either player's thinking time.
    if (ok) {
        recordCompleted();
        if (state.session.active && state.session.id == previous.session.id &&
            previous.session.game.result == chess::Result::Playing &&
            state.session.game.result != chess::Result::Playing)
            showResult();
        return true;
    }
    state = previous;
    message("Error SD: cambios no guardados");
    refresh();
    return false;
}
bool App::save() {
    if (view == View::ComposerWorkshop && composerUI && !composerUI->save()) return false;
    if (view == View::VariantWorkshop && variantsUI && !variantsUI->save()) return false;
    if (profilesEnabled && activeProfile < 0) return true;
    if (developerSandbox) return store.save(developerOriginal);
    if (openingActive) return store.save(normalState);
    if (progressionActive) { syncNull(); reconcileProgressionRewards(); return store.save(normalState)&&!nullPending; }
    if (trainingActive || tournamentActive) {
        const bool ok = store.save(normalState);
        if (!ok) message("Error al guardar la partida normal");
        return ok;
    }
    syncClock();
    if (progressionReady) reconcileProgressionRewards();
    auto previousProfile = state.profile;
    economy::settle(state);
    bool ok = store.save(state);
    lastClockSample = clockNow();
    if (ok) { recordCompleted(); return !secretPending && !nullPending; }
    state.profile = previousProfile;
    message("Error al guardar en SD");
    return false;
}
void App::suspend() {
    if (composerUI) composerUI->suspend();
    if (variantsUI) variantsUI->suspend();
    syncClock();
    diagnostics::journal().add(diagnostics::Level::Info, "APP_SUSPEND", "lifecycle");
    clockSuspended = true;
    tournamentController.stop();
    bot.stop();
    hints.cancel();
    hintMove = {};
    audio.music(false);
    save();
    diagnostics::journal().flush();
}
void App::resume() {
    diagnostics::journal().add(diagnostics::Level::Info, "APP_RESUME", "lifecycle");
    lastClockSample = clockNow();
    clockSuspended = false;
}
void App::back() {
    // One parent route for B and visible Back/Cancel controls. A game uses B
    // to cancel selection; exiting a game remains a deliberate pause action.
    View parent = View::Home;
    int parentRow = 0;
    switch (view) {
    case View::ComposerWorkshop: parent = View::Modes; parentRow = 4; break;
    case View::VariantWorkshop: parent = View::Modes; parentRow = 5; break;
    case View::Language: if(firstLanguage)return; parent=View::Options;parentRow=7;break;
    case View::Openings: parent = View::Modes; parentRow = 3; break;
    case View::OpeningStudy: parent = View::Openings; break;
    case View::Modes: parent = View::Home; break;
    case View::Career: parent = View::Modes; break;
    case View::Survival: parent = View::Modes; parentRow = 1; break;
    case View::Trophies: parent = trophyReturn; parentRow = 2; break;
    case View::ConfirmEndAttempt: parent = View::Survival; parentRow = 1; break;
    case View::DeveloperPassword: parent = View::Options; parentRow = 6; break;
    case View::Developer: parent = View::Game; break;
    case View::Profiles:
        if (activeProfile >= 0) { parent = View::Home; parentRow = 6; } else return;
        break;
    case View::ProfileSlot: parent = View::Profiles; parentRow = profileChoice; break;
    case View::ConfirmProfileDelete: parent = View::ProfileSlot; parentRow = 3; break;
    case View::Title:
        if (profilesEnabled) { parent = View::Profiles; parentRow = activeProfile >= 0 ? activeProfile : 0; break; }
        return;
    case View::Home: parent = View::Title; break;
    case View::Game:
        selected = -1; claimIntent = false; hints.cancel(); hintMove = {}; return;
    case View::Pause: parent = View::Game; break;
    case View::Help:
        parent = helpReturn;
        parentRow = parent == View::Title ? 1 : parent == View::Pause ? 6 : 5;
        break;
    case View::Review: parent = reviewReturn; parentRow = parent == View::Result ? 1 : 0; break;
    case View::Preferences: parent = View::Options; parentRow = 5; break;
    case View::Tournament: parent = View::Training; parentRow = 4; break;
    case View::Library: parent = View::Training; parentRow = 5; break;
    case View::Lessons:
        parent = View::Training;
        parentRow = academyMode ? 3 : trainingKind == training::Kind::Puzzle ? 0 :
                    trainingKind == training::Kind::Campaign ? 1 : 2;
        break;
    case View::TrainingEnd: parent = View::Lessons; break;
    case View::Training: parentRow = 8; break;
    case View::Tools: parent = View::Pause; parentRow = 5; break;
    case View::Bots: parent = View::NewGame; parentRow = 2; break;
    case View::AdvancedStats: parent = View::Stats; break;
    case View::ConfirmDelete: parent = View::Library; parentRow = 2; break;
    case View::ConfirmResign: parent = View::Pause; parentRow = 4; break;
    case View::OfferDraw: parent = View::Pause; parentRow = 3; break;
    case View::ConfirmQuit:
        parent = confirmReturn; parentRow = parent == View::Title ? 2 : 7; break;
    case View::Promotion: parent = View::Game; break;
    case View::Result:
        if (!endScene.finished()) { endScene.skip(); return; }
        parent = progressionActive ? (progressionState.kind == progression::Kind::Career ? View::Career : View::Survival) : tournamentActive ? View::Tournament : trainingActive ? View::Lessons : View::Home;
        break;
    case View::Shop: parentRow = 2; break;
    case View::Options: parentRow = 4; break;
    case View::Stats: parentRow = 3; break;
    case View::NewGame:
    case View::ConfirmNew: break;
    }
    go(parent);
    // A flag can replace the requested transition with Result. Do not change
    // that screen's selection if syncClock() did so during go().
    if (view == parent) focus = parentRow;
}
void App::go(View v) {
    if (view == View::ComposerWorkshop && v != view && composerUI) composerUI->suspend();
    if (view == View::VariantWorkshop && v != view && variantsUI) variantsUI->suspend();
    bool wasPlaying = state.session.game.result == chess::Result::Playing;
    syncClock();
    if (wasPlaying && state.session.game.result != chess::Result::Playing && view == View::Result)
        return;
    if (v == View::Promotion && state.session.game.result != chess::Result::Playing)
        v = View::Game;
    tournamentController.stop();
    if (v != View::Game) {
        bot.stop();
        hints.cancel();
        hintMove = {};
    }
    if (tournamentActive && (v == View::Home || v == View::Training || v == View::Tournament || v == View::NewGame)) leaveTournament();
    if (trainingActive && (v == View::Home || v == View::Lessons || v == View::Training || v == View::NewGame))
        leaveTraining();
    if (v == View::Home || v == View::NewGame || v == View::Training || v == View::Lessons)
        claimIntent = false;
    if (developerSandbox && (v == View::Home || v == View::NewGame || v == View::Training)) {
        state = developerOriginal; developerSandbox = false; refresh();
        selected = -1; animationFrames = 0; hintMove = {};
    }
    if (openingActive && (v == View::Home || v == View::Training || v == View::NewGame || v == View::Profiles || v == View::Modes || v == View::Openings)) leaveOpening();
    if (openingActive && v == View::OpeningStudy) openingPractice = false;
    if (progressionActive && (v == View::Home || v == View::Training || v == View::NewGame ||
        v == View::Profiles || v == View::Modes || v == View::Career || v == View::Survival || v == View::Trophies)) leaveProgression();
    if (view != v) diagnostics::journal().add(diagnostics::Level::Info, "ENTER_SCREEN", diagnosticArea(v), int(v));
    view = v;
    if (v == View::Tournament) tournamentSaved = true;
    focus = v==View::Language&&currentLanguage==localization::Language::English?1:0;
    lastClockSample = clockNow();
}
int App::rows() const {
    switch (view) {
    case View::Title:
        return 3;
    case View::Result:
        return 4;
    case View::Home:
        return 9;
    case View::NewGame:
        return 8;
    case View::Pause:
        return 8;
    case View::Training:
        return 6;
    case View::Tournament:
        return 4;
    case View::Library:
        return 4;
    case View::ConfirmDelete:
        return 2;
    case View::TrainingEnd:
        return 3;
    case View::Lessons:
        return 2;
    case View::Tools:
        return 4;
    case View::Modes: return 7;
    case View::Openings:
    case View::OpeningStudy: return 4;
    case View::Career:
    case View::Survival: return 4;
    case View::ConfirmEndAttempt: return 2;
    case View::Trophies: return 1;
    case View::Profiles: return 4;
    case View::ProfileSlot: return 5;
    case View::ConfirmProfileDelete: return 2;
    case View::Developer:
        return 7;
    case View::Language: return 2;
    case View::Options:
        return 8;
    case View::Preferences:
        return 4;
    case View::Shop:
        return 3;
    case View::Bots:
        return 2;
    case View::Promotion:
        return 4;
    case View::ConfirmResign:
    case View::OfferDraw:
    case View::ConfirmNew:
    case View::ConfirmQuit:
        return 2;
    default:
        return 0;
    }
}
int App::rowY() const {
    if(view==View::Language)return 96;
    if (view == View::Title)
        return 72;
    if (view == View::Tournament || view == View::Library || view == View::Career || view == View::Survival || view == View::Openings || view == View::OpeningStudy) return 128;
    if (view == View::Trophies) return 200;
    if (view == View::ConfirmDelete || view == View::ConfirmProfileDelete || view == View::ConfirmEndAttempt) return 122;
    if (view == View::Result || view == View::TrainingEnd)
        return 130;
    if(view==View::Bots&&botChoice==49)return 178;
    if (view == View::Bots || view == View::Lessons)
        return 172;
    return view == View::Shop        ? 154
           : view == View::Promotion ? 72
           : (view == View::ConfirmResign || view == View::OfferDraw || view == View::ConfirmNew ||
              view == View::ConfirmQuit)
               ? 122
               : 40;
}
int App::rowH() const {
    if (view == View::Title)
        return 36;
    if (view == View::Result)
        return 24;
    if (view == View::Home) return 20;
    return (view == View::NewGame || view == View::Pause || view == View::Options) ? 21 : 24;
}
void App::startGame() {
    if(state.session.active&&state.session.game.result!=chess::Result::Playing&&!save())return;
    if(state.settings.bot==49&&!nullUnlocked()){message("NULL: Byte, Elon, GLaDOS y Carrera Diamante");return;}
    if(state.settings.bot==40&&!secretUnlocked()){message("Rival secreto: vence a Byte y Jensen en IA");return;}
    auto old = state;
    hints.cancel();
    hintMove = {};
    economy::start(state, state.settings.mode, state.settings.level, state.settings.color,
                   state.settings.bot);
    if (state.session.mode == 2) state.session.clock.reset(0);
    if (!commit(old))
        return;
    selected = -1;
    cursor = state.session.human == -1 ? 52 : 12;
    claimIntent = false;
    reaction = 0;
    reactionFrames = 0;
    refresh();
    go(View::Game);
    if(state.session.bot==49)message(nullState.games<3?"NULL: sin historial suficiente":"NULL esta estudiando tu estilo");
}
void App::finish() {
    if (trainingActive || tournamentActive || developerSandbox || progressionActive) return;
    int earned = economy::settle(state);
    if (earned > 0) {
        message("Partida: +" + std::to_string(earned) + " Chess Coins");
        flash = 100;
    } else if (state.session.game.result != chess::Result::Playing)
        message(state.session.game.reason);
}
void App::play(chess::Move m) {
    if (openingActive && openingPractice) { openingMove(m); return; }
    syncClock();
    if (state.session.game.result != chess::Result::Playing)
        return;
    if (m.from < 0 || m.from >= 64 || m.to < 0 || m.to >= 64) {
        message("Movimiento no permitido");
        return;
    }
    hints.cancel();
    hintMove = {};
    if (trainingActive && academyMode) {
        const auto attempt = academyRun.submit(m);
        selected = -1;
        if (attempt == training::Attempt::Illegal || attempt == training::Attempt::Incorrect) {
            message(academyRun.feedback()); return;
        }
        state.session.game = academyRun.game(); refresh(); animationFrames = 0;
        if (attempt == training::Attempt::Completed) {
            trainingComplete(); trainingMessage = academyRun.feedback(); go(View::TrainingEnd);
        } else message(academyRun.feedback());
        return;
    }
    if (trainingActive && trainingKind != training::Kind::Campaign) {
        const auto attempt = trainingRun.submit(m);
        if (attempt == training::Attempt::Illegal || attempt == training::Attempt::Incorrect) {
            message(attempt == training::Attempt::Illegal ? "Movimiento no permitido" :
                    ai::botDialogue(state.session.bot, ai::Dialogue::Mistake));
            audio.play(2, state.settings.sound);
            selected = -1;
            return;
        }
        state.session.game = trainingRun.game();
        selected = -1;
        refresh();
        animationFrames = 0;
        audio.play(3, state.settings.sound);
        if (attempt == training::Attempt::Completed) {
            trainingComplete();
            trainingMessage = ai::botDialogue(state.session.bot, ai::Dialogue::PuzzleSolved);
            go(View::TrainingEnd);
        } else message("Bien. Sigue buscando la mejor jugada.");
        return;
    }
    auto previous = state;
    int capture = state.session.game.position.board[m.to];
    int moving = state.session.game.position.board[m.from];
    if (!capture && std::abs(moving) == chess::Pawn && m.to == state.session.game.position.ep)
        capture = -state.session.game.position.side * chess::Pawn;
    int movingSide = state.session.game.position.side;
    uint32_t achievements = state.profile.achievements;
    economy::moveAchievements(state, m);
    if (!state.session.game.play(m)) {
        state = previous;
        message("Movimiento no permitido");
        return;
    }
    state.session.clock.moved(movingSide);
    finish();
    if (!commit(previous))
        return;
    selected = -1;
    refresh();
    animMove = m;
    animPiece = moving;
    animationFrames = preferences.reducedMotion ? 0 : 8;
    bool check = state.session.game.position.check(state.session.game.position.side);
    if (capture || check) {
        reaction = movingSide == state.session.human ? 3 : 2;
        reactionFrames = 150;
    }
    if (state.session.game.result != chess::Result::Playing) {
        bool won =
            (state.session.game.result == chess::Result::WhiteWin) == (state.session.human == 1);
        audio.play(won ? 4 : 5, state.settings.sound);
    } else
        audio.play(check ? 3 : capture ? 2 : 1, state.settings.sound);
    if (achievements != state.profile.achievements) {
        message("Logro desbloqueado: +25 monedas");
        flash = 100;
    }
}
void App::setName(const std::string &name) {
    wantsName = false;
    diagnostics::journal().add(diagnostics::Level::Debug, "PROFILE_NAME_INPUT", "confirmed_text_omitted");
    if (profilesEnabled && view == View::ProfileSlot &&
        !profileManager.slots[size_t(profileChoice)].occupied) {
        createProfile(name);
        return;
    }
    std::string clean;
    for (unsigned char c : name)
        if (c >= 32 && c <= 126 && clean.size() < 24)
            clean += c;
    if (clean.empty())
        return;
    if (profilesEnabled) {
        if (activeProfile != profileChoice || activeProfile < 0) return;
        if (!profileManager.rename(activeProfile, clean)) { message("No se pudo renombrar"); return; }
        state.profile.name = clean;
    } else {
        auto old = state; state.profile.name = clean; commit(old);
    }
}
void App::openHelp(View origin) {
    helpReturn = origin;
    helpPage = 0;
    go(View::Help);
}
void App::showResult() {
    if (!state.session.active || state.session.game.result == chess::Result::Playing)
        return;
    bot.stop();
    hints.cancel();
    hintMove = {};
    selected = -1;
    diagnostics::journal().add(diagnostics::Level::Info, "MATCH_RESULT", "terminal_scene", int(state.session.game.result));
    endScene.begin(state.session,pendingScene,pendingNullScene,pendingNullReward);
    pendingNullScene=0;pendingNullReward=false;
    pendingScene=space::Scene::None;
    if (preferences.reducedMotion) endScene.skip();
    view = View::Result;
    focus = 0;
    lastClockSample = clockNow();
}
void App::requestHint() {
    if (openingActive && openingPractice) {
        hintMove = openingRun.hint();
        if (hintMove.from >= 0) { selected = cursor = hintMove.from; message("Pista: reproduce la siguiente jugada"); }
        return;
    }
    if (trainingActive && (academyMode || trainingKind != training::Kind::Campaign)) {
        hintMove = academyMode ? academyRun.hint() : trainingRun.hint();
        if (hintMove.from >= 0) {
            selected = cursor = hintMove.from;
            message("Pista: sigue la flecha con la cruceta");
        }
        return;
    }
    syncClock();
    if (view != View::Game || state.session.game.result != chess::Result::Playing) {
        message("La partida ya termino");
        return;
    }
    if (state.session.mode != 1 && state.session.game.position.side != state.session.human) {
        message("La pista es para tu turno");
        return;
    }
    if (hints.running())
        return;
    hintMove = {};
    if (hints.request(state.session.game))
        message("Buscando una pista...");
    else {
        diagnostics::journal().add(diagnostics::Level::Warn, "HINT_START_FAILED", "search_request");
        message("No hay una pista disponible");
    }
}
void App::activate() {
    if(view==View::Language){selectLanguage();return;}
    if (view == View::Openings || view == View::OpeningStudy) { openingAction(); return; }
    if (view == View::Pause && openingActive) {
        if (focus == 0) go(View::Game);
        else if (focus == 1) { openingPly = int(openingRun.ply()); go(View::OpeningStudy); }
        else if (focus == 2) beginOpening(true);
        else if (focus == 3) message("Reproduce la linea; X muestra la siguiente jugada");
        else if (focus == 4) go(View::Openings);
        else if (focus == 5) openReview(View::Pause);
        else if (focus == 6) openHelp(View::Pause);
        else { if (save()) exitRequested = true; }
        return;
    }
    if (view == View::Modes || view == View::Career || view == View::Survival || view == View::ConfirmEndAttempt || view == View::Trophies) { progressionAction(); return; }
    if (view == View::Profiles || view == View::ProfileSlot || view == View::ConfirmProfileDelete) { profileAction(); return; }
    if (view == View::Developer) { developerAction(); return; }
    audio.play(0, state.settings.sound);
    if (view == View::Title) {
        if (focus == 0)
            go(View::Home);
        else if (focus == 1)
            openHelp(View::Title);
        else {
            confirmReturn = View::Title;
            go(View::ConfirmQuit);
        }
        return;
    }
    if (view == View::Result) {
        if (progressionActive && focus != 1 && endScene.finished()) {
            go(focus == 3 ? View::Home : progressionState.kind == progression::Kind::Career ? View::Career : View::Survival); return;
        }
        if (tournamentActive && focus != 1 && endScene.finished()) {
            go(focus == 3 ? View::Home : View::Tournament); return;
        }
        if (!endScene.finished()) {
            endScene.skip();
            return;
        }
        if (focus == 0)
            rematch();
        else if (focus == 1)
            openReview(View::Result);
        else if (focus == 2) {
            if (state.session.mode == 2 && !trainingActive) undoPractice();
            else go(View::NewGame);
        } else
            go(View::Home);
        return;
    }
    if (view == View::Home) {
        switch (focus) {
        case 0:
            if (state.session.active && state.session.game.result == chess::Result::Playing)
                go(View::ConfirmNew);
            else
                go(View::NewGame);
            break;
        case 1:
            if (state.session.active) {
                refresh();
                if (state.session.game.result == chess::Result::Playing)
                    go(View::Game);
                else
                    showResult();
            } else
                message("No hay partida guardada");
            break;
        case 2:
            go(View::Shop);
            break;
        case 3:
            go(View::Stats);
            break;
        case 4:
            go(View::Options);
            break;
        case 5:
            openHelp(View::Home);
            break;
        case 6:
            if (profilesEnabled) {
                if (save()) go(View::Profiles);
            } else wantsName = true;
            break;
        case 8:
            go(View::Training);
            break;
        case 7:
            confirmReturn = View::Home;
            go(View::ConfirmQuit);
            break;
        }
        return;
    }
    if (view == View::ConfirmNew) {
        go(focus == 0 ? View::NewGame : View::Home);
        return;
    }
    if (view == View::ConfirmQuit) {
        if (focus == 0) {
            if (save())
                exitRequested = true;
        } else
            back();
        return;
    }
    if (view == View::NewGame) {
        auto old = state;
        switch (focus) {
        case 0:
            state.settings.mode = (state.settings.mode + 1) % 3;
            break;
        case 1:
            state.settings.color = -state.settings.color;
            break;
        case 2:
            botChoice = state.settings.bot;
            go(View::Bots);
            return;
        case 3: {
            int current = state.profile.equipped[0];
            for (int n = 1; n <= 5; ++n) {
                int id = (current + n) % 5;
                if (state.profile.unlocked & (1u << id)) {
                    state.profile.equip(id);
                    break;
                }
            }
            break;
        }
        case 4: {
            const int ids[] = {0, 5, 6};
            int current = state.profile.equipped[1] == 5   ? 1
                          : state.profile.equipped[1] == 6 ? 2
                                                           : 0;
            for (int n = 1; n <= 3; ++n) {
                int id = ids[(current + n) % 3];
                if (state.profile.unlocked & (1u << id)) {
                    state.profile.equipped[1] = id;
                    break;
                }
            }
            break;
        }
        case 5:
            state.settings.clockPreset = (state.settings.clockPreset + 1) % chessclock::presetCount;
            break;
        case 6:
            startGame();
            return;
        case 7:
            go(View::Home);
            return;
        }
        commit(old);
        return;
    }
    if (view == View::Bots) {
        if (focus == 0) {
            auto old = state;
            if(botChoice==49&&!nullUnlocked()){message("NULL: Byte, Elon, GLaDOS y Carrera Diamante");return;}
            if(botChoice==40&&!secretUnlocked()){message("Rival secreto: vence a Byte y Jensen en IA");return;}
            state.settings.bot = botChoice;
            state.settings.level = ai::botAt(botChoice).engineLevel;
            if (!commit(old))
                return;
        }
        go(View::NewGame);
        focus = 2;
        return;
    }
    if (view == View::Game) {
        auto &session = state.session;
        auto &g = session.game;
        if (g.result != chess::Result::Playing) {
            showResult();
            return;
        }
        if (bot.running || (session.mode != 1 && g.position.side != session.human))
            return;
        if (selected < 0) {
            if (g.position.board[cursor] * g.position.side > 0) {
                selected = cursor;
                if (hintMove.from != selected) {
                    hints.cancel();
                    hintMove = {};
                }
            }
            return;
        }
        auto list = g.position.legal();
        for (auto m : list) {
            if (m.from != selected || m.to != cursor)
                continue;
            if (claimIntent) {
                auto old = state;
                if (g.claim(&m)) {
                    claimIntent = false;
                    selected = -1;
                    finish();
                    commit(old);
                } else
                    message("La jugada no permite reclamar");
                return;
            }
            if (m.promotion) {
                const int recommended = hintMove.from == selected && hintMove.to == cursor
                                            ? hintMove.promotion : 0;
                promotion = 0;
                go(View::Promotion);
                if (view == View::Promotion) {
                    const int choices[] = {chess::Queen, chess::Rook, chess::Bishop, chess::Knight};
                    for (int i = 0; i < 4; ++i)
                        if (choices[i] == recommended) focus = i;
                }
                return;
            }
            play(m);
            return;
        }
        selected = g.position.board[cursor] * g.position.side > 0 ? cursor : -1;
        if (hintMove.from != selected) {
            hints.cancel();
            hintMove = {};
        }
        return;
    }
    if (view == View::Promotion) {
        chess::Move m{
            selected, cursor,
            std::array<int, 4>{chess::Queen, chess::Rook, chess::Bishop, chess::Knight}[focus]};
        go(View::Game);
        play(m);
        return;
    }
    if (view == View::Pause) {
        switch (focus) {
        case 0:
            go(View::Game);
            break;
        case 1:
            if (save()) {
                go(View::Home);
                message("Partida guardada");
            }
            break;
        case 2:
            if (state.session.game.result != chess::Result::Playing) {
                message("La partida ya termino");
                break;
            }
            if (state.session.mode != 1 &&
                state.session.game.position.side != state.session.human) {
                message("Espera tu turno");
                break;
            }
            {
                auto old = state;
                if (state.session.game.claim()) {
                    finish();
                    commit(old);
                } else {
                    claimIntent = true;
                    selected = -1;
                    go(View::Game);
                    message("Elige la jugada para reclamar");
                }
            }
            break;
        case 3:
            if (state.session.game.result != chess::Result::Playing)
                break;
            if (state.session.mode == 1)
                go(View::OfferDraw);
            else if (state.session.game.position.side != state.session.human)
                message("Espera tu turno");
            else if (state.session.game.moves.size() >= 10 &&
                     ai::Search::evaluate(state.session.game.position) >= 0) {
                auto old = state;
                state.session.game.agreeDraw();
                finish();
                commit(old);
            } else
                message("El rival rechaza las tablas");
            break;
        case 4:
            if (state.session.game.result == chess::Result::Playing)
                go(View::ConfirmResign);
            break;
        case 5:
            go(View::Tools);
            break;
        case 6:
            openHelp(View::Pause);
            break;
        case 7:
            confirmReturn = View::Pause;
            go(View::ConfirmQuit);
            break;
        }
        return;
    }
    if (view == View::ConfirmResign || view == View::OfferDraw) {
        if (focus == 0) {
            auto old = state;
            if (view == View::ConfirmResign)
                state.session.game.resign(state.session.mode == 1 ? state.session.game.position.side : state.session.human);
            else
                state.session.game.agreeDraw();
            finish();
            commit(old);
        } else
            back();
        return;
    }
    if (view == View::Tournament) {
        if (focus == 0) {
            if (!tournamentState.size || tournamentState.finished()) {
                auto next = tournamentState;
                if (!tournament::start(next, tournamentSize, state.settings.bot, uint32_t(state.profile.nextId + tournamentState.id * 104729)) ||
                    !tournamentStore.save(next)) { message("No se pudo guardar el torneo"); return; }
                tournamentState = std::move(next);
            }
            if (tournamentState.playerMatch()) beginTournamentMatch();
            else message("Simulando partidas reales de IA...");
        } else if (focus == 1) tournamentSize = tournamentSize == 4 ? 8 : 4;
        else if (focus == 2) message(tournament::drawRules());
        else back();
        return;
    }
    if (view == View::Library) {
        if (focus == 3) { back(); return; }
        if (archiveState.games.empty()) { message("Aun no hay partidas archivadas"); return; }
        if (focus == 0) openLibraryReview();
        else if (focus == 1) message(archiveStore.exportPgn(archiveState.games[size_t(libraryIndex)]) ? "PGN exportado a la tarjeta SD" : "Error al exportar PGN");
        else go(View::ConfirmDelete);
        return;
    }
    if (view == View::ConfirmDelete) {
        if (focus != 0) { back(); return; }
        if (focus == 0 && !archiveState.games.empty()) {
            auto next = archiveState;
            archive::erase(next, next.games[size_t(libraryIndex)].id);
            if (archiveStore.save(next)) {
                archiveState = std::move(next);
                libraryIndex = std::max(0, std::min(libraryIndex, int(archiveState.games.size()) - 1));
            } else message("Error SD: partida conservada");
        }
        go(View::Library); return;
    }
    if (view == View::Training) {
        if (focus == 4) { go(View::Tournament); return; }
        if (focus == 5) { libraryIndex = 0; go(View::Library); return; }
        academyMode = focus == 3;
        trainingKind = focus == 0 ? training::Kind::Puzzle : focus == 1 ? training::Kind::Campaign : training::Kind::Challenge;
        lessonIndex = 0;
        go(View::Lessons);
        return;
    }
    if (view == View::Lessons) {
        if (focus == 0) beginLesson();
        else back();
        return;
    }
    if (view == View::TrainingEnd) {
        if (focus == 0) beginLesson();
        else if (focus == 1) { lessonIndex = (lessonIndex + 1) % lessonCount(); beginLesson(); }
        else go(View::Lessons);
        return;
    }
    if (view == View::Tools) {
        if (focus == 0) openReview(View::Tools);
        else if (focus == 1) undoPractice();
        else if (focus == 2)
            message(trainingActive ? "Entrenamiento: no se exporta PGN" : store.exportPgn(state) ? "PGN exportado a la tarjeta SD" : "PGN: termina primero la partida");
        else back();
        return;
    }
    if (view == View::Shop) {
        auto old = state;
        if (focus == 0) {
            if (state.profile.buy(shopIndex)) {
                if (commit(old)) {
                    message("Compra realizada");
                    flash = 80;
                    audio.play(4, state.settings.sound);
                }
            } else
                message((state.profile.unlocked & (1u << shopIndex)) ? "Ya lo tienes"
                                                                     : "Monedas insuficientes");
        } else if (focus == 1) {
            if (state.profile.equip(shopIndex)) {
                commit(old);
                message("Equipado");
            } else
                message("Primero desbloquea el articulo");
        } else
            back();
        return;
    }
    if (view == View::Preferences) {
        if (focus == 3) { back(); return; }
        const auto old = preferences;
        if (focus == 0) preferences.reducedMotion = !preferences.reducedMotion;
        else if (focus == 1) preferences.pieceInfo = !preferences.pieceInfo;
        else preferences.styleOverride = (preferences.styleOverride + 2) % 7 - 1;
        if (!preferences.save(dataDirectory + "/ui.dat")) {
            preferences = old;
            message("Error SD: opciones sin guardar");
        }
        return;
    }
    if (view == View::Options) {
        if(focus==7){go(View::Language);return;}
        if (focus == 6) {
            if (developerEnabled) { developerEnabled = false; message("Developer desactivado"); }
            else { developerCode = {}; developerDigit = 0; go(View::DeveloperPassword); }
            return;
        }
        if (focus == 5) { go(View::Preferences); return; }
        auto old = state;
        if (focus == 0)
            state.settings.spanish = !state.settings.spanish;
        else if (focus == 1)
            state.settings.sound = !state.settings.sound;
        else if (focus == 2)
            state.settings.music = !state.settings.music;
        else if (focus == 3) {
            state.profile.equipped = {0, 0, 0, 0, 0};
            message("Estilo clasico equipado");
        } else {
            back();
            return;
        }
        if (commit(old))
            refresh();
        return;
    }
}
void App::update(Input in, int64_t elapsedMs) {
    localization::setLanguage(currentLanguage);
    if(unlockFrames>0)--unlockFrames;
    if(nullUnlockFrames>0)--nullUnlockFrames;
    battery.update(uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(clockNow().time_since_epoch()).count()));
    const View entryView = view;
    struct ScopedClockMode {
        bool &value;
        bool previous;
        ~ScopedClockMode() { value = previous; }
    } restoreClockMode{explicitClockStep, explicitClockStep};
    explicitClockStep = elapsedMs >= 0;
    if (elapsedMs < 0)
        syncClock();
    else {
        lastClockSample = clockNow();
        advanceClock(elapsedMs);
    }
    if (clockSuspended)
        return;
    if (view == View::Result && entryView != View::Result)
        return; // Do not reuse a late move as a result-menu click.
    const bool matchMusic = view == View::Game || view == View::Promotion || view == View::Pause ||
                            view == View::Result || view == View::Review || view == View::Tools;
    audio.music(state.settings.music, view == View::Bots ? botChoice :
                matchMusic && state.session.mode != 1 ? state.session.bot : -1);
    ++ticks;
    if (reactionFrames > 0)
        --reactionFrames;
    if (noticeFrames > 0)
        --noticeFrames;
    if (preferences.reducedMotion) animationFrames = 0;
    else if (animationFrames > 0) --animationFrames;
    if (flash > 0)
        --flash;
    if (view == View::Result) {
        if (preferences.reducedMotion) endScene.skip();
        endScene.update();
    }
    if (view == View::ComposerWorkshop && composerUI) { if (!composerUI->update(in)) back(); return; }
    if (view == View::VariantWorkshop && variantsUI) { if (!variantsUI->update(in)) back(); return; }
    if (view == View::DeveloperPassword) { developerPassword(in); return; }
    if (view == View::Help) {
        if (in.keys & B)
            back();
        else if (in.keys & Left)
            helpPage = (helpPage + helpPageCount - 1) % helpPageCount;
        else if (in.keys & (Right | A))
            helpPage = (helpPage + 1) % helpPageCount;
        else if (in.touch && in.y >= 198 && in.y < 219) {
            if (in.x >= 12 && in.x < 94)
                helpPage = (helpPage + helpPageCount - 1) % helpPageCount;
            else if (in.x >= 113 && in.x < 207)
                back();
            else if (in.x >= 226 && in.x < 308)
                helpPage = (helpPage + 1) % helpPageCount;
        }
        return;
    }
    if (view == View::Tournament && !tournamentActive && !tournamentState.playerMatch() && !tournamentState.finished() && tournamentSaved) {
        auto before = tournamentState;
        if (tournamentController.tick(tournamentState) && !tournamentStore.save(tournamentState)) {
            tournamentState = std::move(before);
            tournamentController.stop(); tournamentSaved = false;
            message("Error SD: simulacion pausada; vuelve a entrar");
        }
    }
    if (view == View::Review) {
        if (in.keys & (B | Start)) back();
        else if (in.keys & Left) seekReview(int(reviewPly) - 1);
        else if (in.keys & Right) seekReview(int(reviewPly) + 1);
        else if (in.keys & Up) seekReview(0);
        else if (in.keys & Down) seekReview(int(reviewSource.moves.size()));
        else if (in.touch && in.y >= 194 && in.y < 215) {
            if (in.x >= 12 && in.x < 80) seekReview(int(reviewPly) - 1);
            else if (in.x >= 84 && in.x < 156) seekReview(int(reviewPly) + 1);
            else if (in.x >= 160 && in.x < 232) seekReview(0);
            else if (in.x >= 236 && in.x < 308) back();
        }
        return;
    }
    if (view == View::Openings && (in.keys & (Left | Right))) {
        openingIndex = (openingIndex + ((in.keys & Left) ? openings::Count-1 : 1)) % openings::Count;
        openingBranch = 0;
    }
    if (view == View::OpeningStudy && (in.keys & (Left | Right))) {
        seekOpening(openingPly + ((in.keys & Left) ? -1 : 1)); return;
    }
    if (view == View::Home && ((in.keys & Select) || (in.touch && in.x >= 12 && in.x < 308 && in.y >= 224 && in.y < 240))) { go(View::Modes); return; }
    if (view == View::Trophies && in.touch && in.y >= 170 && in.y < 191) {
        if (in.x >= 12 && in.x < 152) trophyIndex = (trophyIndex + 19) % 20;
        else if (in.x >= 166 && in.x < 308) trophyIndex = (trophyIndex + 1) % 20;
        return;
    }
    if (view == View::Trophies && (in.keys & (Left | Right))) trophyIndex = (trophyIndex + ((in.keys & Left) ? 19 : 1)) % 20;
    if ((in.keys & Select) && view == View::Game) {
        if (developerEnabled) go(View::Developer);
        else openReview(View::Game);
        return;
    }
    if ((in.keys & Select) && view == View::Stats) { go(View::AdvancedStats); return; }
    if (view == View::AdvancedStats && (in.keys & (Left | Right))) {
        statisticsBot = (statisticsBot + ((in.keys & Left) ? ai::botCount-1 : 1)) % ai::botCount;
    }
    if (view == View::Library && !archiveState.games.empty() && (in.keys & (Left | Right))) {
        libraryIndex = (libraryIndex + ((in.keys & Left) ? int(archiveState.games.size()) - 1 : 1)) % int(archiveState.games.size());
    }
    if (in.keys & Select) context = !context;
    if (in.keys & Start) {
        if (view == View::Title)
            go(View::Home);
        else if (view == View::Game)
            go(View::Pause);
        else if (view == View::Pause)
            go(View::Game);
        return;
    }
    if (in.keys & B) {
        back();
        return;
    }
    if (view == View::Game) {
        if (in.keys & Up)
            cursor = (cursor + (flipped() ? 56 : 8)) % 64;
        if (in.keys & Down)
            cursor = (cursor + (flipped() ? 8 : 56)) % 64;
        if (in.keys & Left)
            cursor = (cursor / 8) * 8 + (cursor + (flipped() ? 1 : 7)) % 8;
        if (in.keys & Right)
            cursor = (cursor / 8) * 8 + (cursor + (flipped() ? 7 : 1)) % 8;
        if (in.touch && in.y >= 185 && in.y < 206) {
            if (in.x >= 12 && in.x < 104)
                go(View::Pause);
            else if (in.x >= 114 && in.x < 206) {
                if (openingActive && openingPractice) { beginOpening(true); return; }
                if (trainingActive && (academyMode || trainingKind != training::Kind::Campaign)) {
                    beginLesson();
                    return;
                }
                go(View::Pause);
                if (view == View::Pause) {
                    focus = 3;
                    activate();
                }
            } else if (in.x >= 216 && in.x < 308)
                requestHint();
        }
        if (in.keys & Hint)
            requestHint();
        chess::Move suggested;
        if (view == View::Game && hints.poll(state.session.game, suggested)) {
            syncClock();
            if (view != View::Game)
                return;
            if (view == View::Game && state.session.game.result == chess::Result::Playing) {
                hintMove = suggested;
                selected = suggested.from;
                cursor = suggested.from;
                message("Pista: sigue la flecha con la cruceta");
            }
        }
        if ((in.keys & A) && view == View::Game)
            activate();
        if (view == View::Game && state.session.game.result == chess::Result::Playing &&
            state.session.mode != 1 && state.session.game.position.side != state.session.human &&
            (!trainingActive || (!academyMode && trainingKind == training::Kind::Campaign))) {
            ai::Answer answer;
            if (bot.poll(state.session.game, answer)) {
                lastSearch = answer;
                syncClock();
                if (state.session.game.result != chess::Result::Playing)
                    return;
                if (state.session.game.canClaim() && answer.score <= 0) {
                    auto old = state;
                    state.session.game.claim();
                    finish();
                    commit(old);
                } else if (answer.move.from >= 0)
                    play(answer.move);
            } else if (!bot.running) {
                int aiTimeBudget = 0;
                if (state.session.clock.preset) {
                    int side = state.session.game.position.side;
                    int64_t remaining = state.session.clock.remainingMs[side == 1 ? 0 : 1];
                    int64_t increment = chessclock::presets[state.session.clock.preset].incrementMs;
                    int64_t reserve = std::min<int64_t>(100, remaining / 10);
                    aiTimeBudget = int(std::max<int64_t>(
                        1, std::min(remaining - reserve, remaining / 24 + increment / 2)));
                }
                if (!bot.start(state.session.game, state.session.level,
                               uint32_t(state.session.id * 7919 +
                                        state.session.game.moves.size() * 104729 +
                                        ai::botAt(state.session.bot).seedSalt),
                               state.session.bot, aiTimeBudget, preferences.styleOverride < 0 ?
                               (state.session.bot==49&&nullReady?nullboss::style(nullState):ai::styleForBot(state.session.bot)) : ai::Style(preferences.styleOverride),
                               state.session.bot==49&&nullReady?nullboss::opening(nullState,state.session.game,uint32_t(state.session.id*7919+state.session.game.moves.size()*104729)):chess::Move{}))
                    {
                        diagnostics::journal().add(diagnostics::Level::Error, "AI_THREAD_START_FAILED", "match_worker");
                        message("No se pudo iniciar la IA");
                    }
            }
        }
        return;
    }
    if (view == View::Stats && in.touch && in.x >= 12 && in.x < 308 && in.y >= 205 && in.y < 226) {
        back();
        return;
    }
    if (view == View::Lessons && (in.keys & (Left | Right))) {
        lessonIndex = (lessonIndex + ((in.keys & Left) ? lessonCount() - 1 : 1)) % lessonCount();
    }
    if (view == View::Bots) {
        if (in.keys & Left)
            botChoice = (botChoice + ai::botCount - 1) % ai::botCount;
        if (in.keys & Right)
            botChoice = (botChoice + 1) % ai::botCount;
        if (in.touch && in.y >= 112 && in.y < 154 && in.x >= 12 && in.x < 308) {
            int tapped = (botChoice / 5) * 5 + (in.x - 12) / 60;
            if (tapped < ai::botCount)
                botChoice = tapped;
        } else if (in.touch && in.y >= 36 && in.y < 62) {
            botChoice = (botChoice + (in.x < 160 ? ai::botCount - 1 : 1)) % ai::botCount;
        }
    }
    int count = rows();
    if (count) {
        if (in.keys & Up)
            focus = (focus + count - 1) % count;
        if (in.keys & Down)
            focus = (focus + 1) % count;
        if (view == View::Shop && (in.keys & (Left | Right))) {
            shopIndex = (shopIndex + ((in.keys & Left) ? economy::productCount-1 : 1)) % economy::productCount;
        } else if (view == View::NewGame && (in.keys & (Left | Right)) && focus < 6) {
            if (focus == 5 && (in.keys & Left)) {
                auto old = state;
                state.settings.clockPreset =
                    (state.settings.clockPreset + chessclock::presetCount - 1) %
                    chessclock::presetCount;
                commit(old);
            } else
                activate();
        }
        if (in.touch) {
            int row = (in.y - rowY()) / rowH();
            const int height = view == View::Home ? 19 : 21;
            if (in.x >= 12 && in.x < 308 && in.y >= rowY() && row >= 0 && row < count &&
                (in.y - rowY()) % rowH() < height) {
                focus = row;
                activate();
            } else if (view == View::Shop && in.y >= 36 && in.y < 62) {
                shopIndex = (shopIndex + (in.x < 160 ? economy::productCount-1 : 1)) % economy::productCount;
            }
        }
        if ((in.keys & A) && !in.touch)
            activate();
    }
}
void App::button(Renderer &r, int x, int y, int w, const std::string &label, bool active, bool literalLabel) {
    const int height = view == View::Home ? 19 : 21;
    r.rect(x, y, w, height, active ? rgb(65, 71, 75) : panel);
    r.border(x, y, w, height, active ? gold : rgb(55, 65, 78));
    if(literalLabel)r.rawText(x+8,y+7,label,active?gold:ink,1,(w-16)/6);
    else r.text(x + 8, y + 7, label, active ? gold : ink, 1, (w - 16) / 6);
}
int App::portraitExpression() const {
    if (view == View::Bots)
        return int((ticks / 90) % 4);
    if (!state.session.active || (view == View::Home || view == View::NewGame))
        return 0;
    if (state.session.game.result != chess::Result::Playing) {
        if (state.session.game.result == chess::Result::Draw)
            return 0;
        bool humanWon =
            (state.session.game.result == chess::Result::WhiteWin) == (state.session.human == 1);
        return humanWon ? 3 : 2;
    }
    if (bot.running)
        return 1;
    if (reactionFrames > 0)
        return reaction;
    return 0;
}
void App::board(Renderer &r) {
    const auto &display = view == View::Review ? reviewGame : state.session.game;
    const auto &p = display.position;
    int theme = state.profile.equipped[0];
    const Color light[] = {rgb(179, 169, 130), rgb(118, 174, 193), rgb(176, 194, 144),
                           rgb(190, 173, 212), rgb(209, 180, 107)};
    const Color dark[] = {rgb(88, 86, 96), rgb(32, 61, 101), rgb(53, 83, 67), rgb(80, 55, 108),
                          rgb(97, 69, 41)};
    Color l = theme==economy::NullBoard?rgb(105,69,135):light[std::clamp(theme,0,4)], d = theme==economy::NullBoard?rgb(23,18,35):dark[std::clamp(theme,0,4)];
    if (state.profile.equipped[2] == 7) {
        l = rgb(205, 193, 182);
        d = rgb(116, 122, 148);
    } else if (state.profile.equipped[2] == 8) {
        l = rgb(224, 186, 93);
        d = rgb(43, 60, 99);
    }
    r.screenBegin(0, bg);
    r.border(10, 6, 228, 228, state.profile.equipped[3] == 10 ? gold : muted,
             state.profile.equipped[3] ? 2 : 1);
    if(theme==economy::NullBoard){
        for(int i=0;i<14;++i){r.rect(12+i*16,5,5,2,rgb(181,101,238));r.rect(12+i*16,234,5,2,rgb(181,101,238));}
    }
    auto legal = selected >= 0 && view != View::Review ? p.legal() : chess::MoveList{};
    int skin = state.profile.equipped[1] == 5 ? 1 : state.profile.equipped[1] == 6 ? 2 : 0;
    bool check = p.check(p.side);
    chess::Move last =
        display.moves.empty() ? chess::Move{} : display.moves.back();
    for (int sq = 0; sq < 64; ++sq) {
        int x = 12 + boardFile(sq) * 28, y = 8 + boardRow(sq) * 28;
        r.rect(x, y, 28, 28, ((sq / 8 + sq % 8) & 1) ? l : d);
        if (sq == last.from || sq == last.to)
            r.rect(x, y, 28, 28, rgb(214, 176, 69, 72));
        if (sq == selected && view != View::Review)
            r.border(x + 1, y + 1, 26, 26, gold, 2);
        if (check && p.board[sq] == p.side * chess::King)
            r.border(x + 1, y + 1, 26, 26, red, 2);
        if (!(view == View::Game && animationFrames && sq == animMove.to))
            r.piece(p.board[sq], x + 2, y + 2, skin);
        for (auto m : legal)
            if (m.from == selected && m.to == sq) {
                if (p.board[sq])
                    r.border(x + 2, y + 2, 24, 24, blue);
                else
                    r.rect(x + 12, y + 12, 4, 4, blue);
                break;
            }
        if (sq == cursor && view != View::Review && view != View::OpeningStudy)
            r.border(x, y, 28, 28, (flash % 20 < 10) ? ink : gold);
    }
    if (view == View::Game && hintMove.from >= 0 && hintMove.to >= 0 &&
        state.session.game.result == chess::Result::Playing) {
        int x1 = 26 + boardFile(hintMove.from) * 28, y1 = 22 + boardRow(hintMove.from) * 28;
        int x2 = 26 + boardFile(hintMove.to) * 28, y2 = 22 + boardRow(hintMove.to) * 28;
        r.arrow(x1, y1, x2, y2, blue);
        r.border(x2 - 13, y2 - 13, 26, 26, blue, 2);
        r.text(252, 201, "PISTA DISPONIBLE", blue);
    }
    if (view == View::Game && animationFrames && animPiece) {
        int t = 8 - animationFrames;
        int x = 12 + (boardFile(animMove.from) * 28 * (8 - t) + boardFile(animMove.to) * 28 * t) / 8;
        int y = 8 + (boardRow(animMove.from) * 28 * (8 - t) + boardRow(animMove.to) * 28 * t) / 8;
        r.piece(animPiece, x + 2, y + 2, skin);
    }
    for (int f = 0; f < 8; ++f)
        r.text(23 + f * 28, 232, std::string(1, char('A' + (flipped() ? 7 - f : f))), muted);
    for (int rank = 0; rank < 8; ++rank)
        r.text(2, 19 + (7 - rank) * 28, std::to_string(flipped() ? 8 - rank : rank + 1), muted);
    if (view == View::Review) {
        r.text(252, 14, "REVISION", gold, 2);
        int y = 46;
        for (size_t i = 0; i < std::min(size_t(4), reviewExplanation.size()); ++i) {
            paragraph(r, 252, y, reviewExplanation[i], i == 0 ? gold : ink, 22, 2);
            y += 30;
        }
        r.text(252, 222, "B: VOLVER", muted);
        return;
    }
    const bool currentGame =
        state.session.active && view != View::Home && view != View::NewGame && view != View::Bots;
    int id = view == View::Bots ? botChoice : currentGame ? state.session.bot : state.settings.bot;
    const auto &opponent = ai::botAt(id);
    const bool local = openingActive || (currentGame && state.session.mode == 1);
    int expression = portraitExpression();
    if (local) {
        r.text(252, 12, "3DS", gold, 2);
        r.text(252, 34, "CHESS", ink, 2);
        r.text(252, 66, openingActive ? "APERTURAS" : "DOS JUGADORES", muted);
        if (openingActive) paragraph(r,252,84,openings::line(openingIndex,openingBranch).name,ink,22,2);
    } else {
        r.portrait(opponent.portraitIndex, expression, 284, 8, 2);
        r.border(282, 6, 68, 68, gold);
        r.rawText(252, 83, opponent.name, ink, 1, 23);
        r.text(252, 97, std::string("RANGO: ") + opponent.strengthLabel, gold, 1, 23);
    }
    r.rect(252, 110, 136, 1, muted);
    r.text(252, 121, p.side == 1 ? "TURNO DORADO" : "TURNO AZUL", p.side == 1 ? gold : blue);
    r.text(252, 137, check ? "JAQUE!" : "OFFLINE / 8-BIT", check ? red : muted);
    if (openingActive) paragraph(r,252,156,openings::line(openingIndex,openingBranch).goal,muted,22,5);
    if (!local) {
        r.text(252, 156,
               bot.running        ? "IA: PENSANDO..."
               : opponent.source  ? "BOT DE CHESS.COM"
               : opponent.tribute ? "HOMENAJE NO OFICIAL"
                                  : "PERSONAJE ORIGINAL",
               muted, 1, 23);
        ai::Dialogue event = bot.running ? ai::Dialogue::Thinking : ai::Dialogue::Greeting;
        if (display.result != chess::Result::Playing && currentGame) {
            const bool humanWon = (display.result == chess::Result::WhiteWin && state.session.human == 1) ||
                                  (display.result == chess::Result::BlackWin && state.session.human == -1);
            event = humanWon ? ai::Dialogue::Loss : ai::Dialogue::Win;
        } else if (trainingActive && trainingKind != training::Kind::Campaign) event = ai::Dialogue::PuzzleStart;
        else if (check) event = ai::Dialogue::Check;
        else if (reactionFrames > 0) event = ai::Dialogue::Capture;
        paragraph(r, 252, 171, ai::botDialogue(id, event), blue, 22, 2);
        if (opponent.referenceElo > 0)
            r.text(252, 194, "ELO REF: " + std::to_string(opponent.referenceElo), muted, 1, 23);
    }
    if (state.session.game.result != chess::Result::Playing && currentGame) {
        r.text(252, 197,
               state.session.game.result == chess::Result::Draw       ? "TABLAS"
               : state.session.game.result == chess::Result::WhiteWin ? "GANA DORADO"
                                                                      : "GANA AZUL",
               gold);
        r.text(252, 207, state.session.game.reason, muted, 1, 23);
    }
    if (context) {
        r.text(252, 215, "NODOS " + std::to_string(lastSearch.nodes), muted, 1, 23);
        r.text(252, 227,
               "IA " + std::to_string(lastSearch.milliseconds) + "MS D" +
                   std::to_string(lastSearch.depth),
               muted, 1, 23);
    } else if (preferences.pieceInfo && selected >= 0) {
        const char *names[] = {"", "PEON 1", "CABALLO 3", "ALFIL 3", "TORRE 5", "DAMA 9", "REY: PROTEGER"};
        int piece = std::abs(p.board[selected]);
        r.text(252, 222, names[piece], gold, 1, 23);
    } else r.text(252, 222, developerEnabled ? "SELECT: DEVELOPER" : "SELECT: HISTORIAL", muted);
}
void App::bottom(Renderer &r) {
    Color background = state.profile.equipped[4] == 11   ? rgb(15, 36, 54)
                       : state.profile.equipped[4] == 12 ? rgb(22, 42, 33)
                                                         : bg;
    r.screenBegin(1, background);
    if (view == View::Title) {
        r.text(12, 19, "AJEDREZ PARA LLEVAR", gold, 1, 49);
        r.text(12, 41, "Elige cuando comenzar tu partida.", muted, 1, 49);
        button(r, 12, 72, 296, "Entrar al juego", focus == 0);
        button(r, 12, 108, 296, "Como jugar", focus == 1);
        button(r, 12, 144, 296, "Guardar y salir", focus == 2);
        r.text(12, 187, "Sin limite de tiempo por defecto.", muted, 1, 49);
        r.text(12, 207, "A: CONFIRMAR   CRUCETA: ELEGIR", ink, 1, 49);
        r.text(12, 226, appinfo::version, muted);
        return;
    }
    const bool choosingProfile = view == View::Profiles || view == View::ProfileSlot || view == View::ConfirmProfileDelete;
    r.rawText(12, 12, choosingProfile ? localization::translate("PERFILES") : developerSandbox ? localization::translate("DEVELOPER / TEMPORAL") : state.profile.name, ink, 1, 24);
    if (!choosingProfile) r.text(202, 12, std::to_string(state.profile.coins) + " CC", gold, 1, 18);
    r.rect(12, 29, 296, 1, muted);
    auto menu = [&](const std::vector<std::string> &labels) {
        for (int i = 0; i < int(labels.size()); ++i)
            button(r, 12, rowY() + i * rowH(), 296, labels[i], i == focus);
    };
    if(view==View::Language){
        r.rawText(12,48,firstLanguage?"SELECT LANGUAGE / SELECCIONAR IDIOMA":localization::translate("SELECCIONAR IDIOMA"),gold,1,49);
        button(r,12,96,296,"Español",focus==0);button(r,12,120,296,"English",focus==1);
    } else if (view == View::Openings || view == View::OpeningStudy) {
        drawOpeningBottom(r);
    } else if (view == View::Modes || view == View::Career || view == View::Survival || view == View::ConfirmEndAttempt || view == View::Trophies) {
        drawProgressionBottom(r);
    } else if (view == View::Profiles) {
        for (int i = 0; i < 3; ++i) {
            const auto &slot = profileManager.slots[size_t(i)];
            button(r, 12, rowY() + i * rowH(), 296,
                std::to_string(i + 1) + ". " + (slot.occupied ? slot.name : localization::translate("Vacio / crear perfil")), i == focus,true);

        }
        button(r,12,rowY()+3*rowH(),296,activeProfile >= 0 ? "Volver al jugador actual" : "Elige una ranura para comenzar",focus==3);
        paragraph(r,12,150,"Tres jugadores independientes. Monedas, compras y partidas no se comparten.",muted,49,3);
    } else if (view == View::ProfileSlot) {
        const auto &slot = profileManager.slots[size_t(profileChoice)];
        menu({slot.occupied ? "Jugar con este perfil" : "Crear perfil nuevo",
              slot.occupied ? "Renombrar perfil" : "Crear con nombre sugerido",
              "Cambiar avatar", "Eliminar perfil...", "Volver"});
        r.rawText(12,177,localization::translate("Ranura ")+std::to_string(profileChoice+1)+": "+(slot.occupied?slot.name:localization::translate("vacia")),gold,1,49);
        paragraph(r,12,195,"Para renombrar, entra primero en el perfil. Los borrados conservan una copia de recuperacion.",muted,49,2);
    } else if (view == View::ConfirmProfileDelete) {
        r.text(12,65,"ELIMINAR ESTE PERFIL?",gold,1,49);
        paragraph(r,12,86,"Se retira esta ranura. Los otros perfiles se conservan. B cancela.",muted,49,2);
        menu({"Si, retirar perfil", "Cancelar"});
    } else if (view == View::Help) {
        drawHelpBottom(r, helpPage);
    } else if (view == View::Result) {
        r.text(12, 42, endScene.finished() ? endScene.title() : "FIN DE LA PARTIDA", gold, 2, 24);
        r.text(12, 69, state.session.game.reason, ink, 1, 49);
        r.text(12, 88, "Monedas totales: " + std::to_string(state.profile.coins), muted, 1, 49);
        r.text(12, 106,
               endScene.finished() ? "Elige que quieres hacer ahora."
                                   : "A o B para saltar la animacion.",
               muted, 1, 49);
        menu({progressionActive ? "Volver al modo" : tournamentActive ? "Continuar torneo" : "Revancha rapida", "Revisar la partida", progressionActive ? "Resumen del modo" : state.session.mode == 2 && !trainingActive && !tournamentActive && !progressionActive ? "Deshacer en practica" : tournamentActive ? "Cuadro de torneo" : "Nueva partida", "Menu principal"});
    } else if (view == View::Home) {
        menu({"Nueva partida", "Continuar partida", "Tienda retro", "Estadisticas y logros",
              "Opciones", "Como jugar", profilesEnabled ? "Perfiles / cambiar jugador" : "Cambiar nombre", "Guardar y salir", "Entrenamiento y campana"});
    } else if (view == View::NewGame) {
        menu({std::string("Modo: ") + modes[state.settings.mode],
              std::string("Color: ") +
                  (state.settings.color == 1 ? "dorado / blancas" : "azul / negras"),
              std::string("Rival: ") + ai::botAt(state.settings.bot).name + " / " +
                  ai::botAt(state.settings.bot).strengthLabel,
              "Tablero: " + std::string(economy::products[state.profile.equipped[0]].name),
              "Piezas: " + std::string(state.profile.equipped[1]
                                           ? economy::products[state.profile.equipped[1]].name
                                           : "clasicas"),
              std::string("Reloj: ") + (state.settings.mode == 2 ? "sin limite (practica)" : chessclock::presets[state.settings.clockPreset].name),
              "Comenzar", "Volver"});
    } else if (view == View::Bots) {
        const auto &opponent = ai::botAt(botChoice);
        r.text(12, 41, "< ELIGE TU RIVAL >", gold);
        r.text(222, 41,
               std::to_string(botChoice / 5 + 1) + "/" + std::to_string((ai::botCount + 4) / 5) +
                   " PAG",
               muted);
        r.portrait(opponent.portraitIndex, portraitExpression(), 16, 66, 1);
        r.rawText(62, 64, opponent.name, ink, 1, 40);
        r.text(62, 78, std::string("Rango ") + opponent.strengthLabel, gold);
        r.text(62, 93,
               opponent.referenceElo > 0
                   ? "Elo de referencia: " + std::to_string(opponent.referenceElo)
                   : opponent.tagline,
               muted, 1, 40);
        const int firstBot = (botChoice / 5) * 5;
        for (int id = firstBot; id < std::min(ai::botCount, firstBot + 5); ++id) {
            int x = 12 + (id - firstBot) * 60;
            r.rect(x, 112, 56, 43, id == botChoice ? rgb(65, 71, 75) : panel);
            r.portrait(id, 0, x + 12, 114, 1);
            if (id == botChoice)
                r.border(x, 112, 56, 43, gold, 2);
            r.text(x + 20, 147, std::to_string(id + 1), id == botChoice ? gold : muted);
        }
        if(botChoice==49){
            r.text(12,157,std::string(nullState.requirements&1?"[X] ":"[ ] ")+"Byte  "+(nullState.requirements&2?"[X] ":"[ ] ")+"Elon  "+(nullState.requirements&4?"[X] ":"[ ] ")+"GLaDOS",muted,1,49);
            r.text(12,168,nullUnlocked()?"NULL disponible / estudia tu estilo":nullState.requirements&8?"[X] Carrera: division Diamante":"[ ] Carrera: division Diamante (36 puntos)",muted,1,49);
        } else r.text(12, 160,
               botChoice==40&&!secretUnlocked()?"Rival secreto: vence a Byte y Jensen en IA":
               botChoice==40?"INVITADO ESPECIAL / FICTICIO":
               opponent.source    ? "Elo de referencia; motor propio."
               : opponent.tribute ? "Homenaje. No imita su fuerza real."
                                  : "Rangos de juego, sin ELO medido.",
               muted, 1, 49);
        menu({"Elegir este rival", "Volver"});
    } else if (view == View::Review) {
        r.text(12, 40, "REVISION BASICA / APROXIMADA", gold, 1, 49);
        r.text(12, 55, "Jugada " + std::to_string(reviewPly) + "/" + std::to_string(reviewSource.moves.size()) + "  ARRIBA: INICIO  ABAJO: FIN", muted, 1, 49);
        int y = 78;
        for (size_t i = 0; i < std::min(size_t(6), reviewLines.size()); ++i) {
            r.text(12, y, reviewLines[i], blue, 1, 49);
            y += 11;
        }
        const auto &taken = reviewCaptured;
        r.text(12, 158, capturesLine(taken, 1), gold, 1, 49);
        r.text(12, 171, capturesLine(taken, -1), blue, 1, 49);
        button(r, 12, 194, 68, "< ANTES", false);
        button(r, 84, 194, 72, "DESPUES >", false);
        button(r, 160, 194, 72, "INICIO", false);
        button(r, 236, 194, 72, "VOLVER", false);
    } else if (view == View::Game) {
        auto &s = state.session;
        const bool timed = s.clock.preset != 0;
        if (timed) {
            for (int color = 0; color < 2; ++color) {
                int x = color ? 164 : 12;
                bool active = s.game.result == chess::Result::Playing &&
                              s.game.position.side == (color == 0 ? 1 : -1);
                Color accent = color ? blue : gold;
                r.rect(x, 35, 144, 31, panel);
                if (active)
                    r.border(x, 35, 144, 31, accent);
                r.text(x + 7, 39, color ? "AZUL" : "DORADO", accent);
                Color timeColor = s.clock.remainingMs[color] <= 10000 ? red : ink;
                auto display = chessclock::format(s.clock.remainingMs[color]);
                r.text(x + 65, 44, display, timeColor, display.size() <= 6 ? 2 : 1, 12);
            }
        }
        int headerY = timed ? 71 : 40;
        r.text(12, headerY,
               openingPractice ? "PRACTICA: " + std::string(openings::line(openingIndex,openingBranch).name) : s.mode == 1 ? "LOCAL - JUGADOR 1 / JUGADOR 2"
                           : std::string("TU: ") + (s.human == 1 ? "DORADO" : "AZUL") + "  " +
                                 ai::botAt(s.bot).name + " / " + ai::botAt(s.bot).strengthLabel,
               ink, 1, 49);
        r.text(12, timed ? 84 : 53,
               "JUGADA " + std::to_string(s.game.position.fullmove) +
                   (claimIntent ? "  RECLAMAR CON JUGADA"
                    : timed     ? "  HISTORIAL"
                                : "  SIN LIMITE"),
               muted, 1, 49);
        int boxY = timed ? 96 : 67;
        r.rect(12, boxY, 296, 148 - boxY, panel);
        int visible = timed ? 3 : 4;
        int pairs = (int(history.size()) + 1) / 2, start = std::max(0, pairs - visible);
        const bool exerciseGame = (openingActive && openingPractice) || (trainingActive && (academyMode || trainingKind != training::Kind::Campaign));
        if (exerciseGame) {
            const auto description = openingActive ? openings::line(openingIndex,openingBranch).goal : academyMode ? academy::lesson(lessonIndex).objective : training::exercise(trainingKind, lessonIndex).description;
            paragraph(r, 20, boxY + 6, description, gold, 46, 2);
            paragraph(r, 20, boxY + 34, openingActive ? "Reproduce la linea de ambos colores. X: pista. Error: se conserva la posicion." : academyMode ? academyRun.feedback() : "Encuentra la mejor jugada. X muestra una pista. B cancela la seleccion.", muted, 46, 3);
        }
        for (int row = start; !exerciseGame && row < pairs; ++row) {
            int y = boxY + 6 + (row - start) * (timed ? 14 : 16);
            r.text(20, y, std::to_string(row + 1) + ".", muted);
            r.text(56, y, history[row * 2], gold, 1, 19);
            if (row * 2 + 1 < int(history.size()))
                r.text(181, y, history[row * 2 + 1], blue, 1, 19);
        }
        const auto &taken = gameCaptured;
        r.text(12, 153, capturesLine(taken, 1), gold, 1, 49);
        r.text(12, 166, capturesLine(taken, -1), blue, 1, 49);
        button(r, 12, 185, 92, "PAUSA", false);
        button(r, 114, 185, 92, exerciseGame ? "REINTENTAR" : "TABLAS", false);
        button(r, 216, 185, 92, hints.running() ? "PISTA..." : "PISTA [X]", hintMove.from >= 0);
    } else if (view == View::Pause) {
        if (openingActive) menu({"Continuar","Volver al estudio","Reiniciar linea","Objetivo","Volver al catalogo","Repasar linea","Como jugar","Guardar y salir"});
        else menu({"Continuar", "Guardar y menu", "Reclamar tablas", "Ofrecer tablas", "Rendirse",
              "Revisar / herramientas", "Como jugar", "Guardar y salir"});
        if (state.session.clock.preset)
            r.text(12, 211, "PAUSA: RELOJES DETENIDOS", gold, 1, 49);
    } else if (view == View::Tournament) {
        r.text(12, 42, "TORNEOS OFFLINE / ELIMINACION DIRECTA", gold, 1, 49);
        std::string status = !tournamentState.size ? "Elige 4 u 8 participantes para empezar." :
            tournamentState.finished() ? (tournamentState.trophy() ? "CAMPEON! Trofeo simbolico obtenido." : "Campeon: " + std::string(ai::botAt(tournamentState.entrants[size_t(tournamentState.winner())]).name)) :
            tournamentState.playerMatch() ? "Tu enfrentamiento esta listo." : "IA contra IA: partidas reales en curso.";
        paragraph(r, 12, 59, status, ink, 49, 2);
        paragraph(r, 12, 86, "Tablas: revancha invirtiendo colores. Dos tablas: avanza la mejor cabeza de serie. Tope: 240 medias jugadas.", muted, 49, 3);
        menu({!tournamentState.size || tournamentState.finished() ? "Comenzar torneo" : tournamentState.playerMatch() ? "Jugar enfrentamiento" : "Continuar simulacion",
              "Nuevo torneo: " + std::to_string(tournamentSize) + " participantes", "Reglas de desempate", "Guardar y volver"});
    } else if (view == View::Library) {
        r.text(12, 41, "BIBLIOTECA " + std::to_string(archiveState.games.size()) + "/30", gold, 1, 49);
        if (!archiveState.games.empty()) {
            const auto &entry = archiveState.games[size_t(libraryIndex)];
            r.text(12, 56, "< " + std::to_string(libraryIndex + 1) + " > " + ai::botAt(entry.bot).name, ink, 1, 49);
            r.text(12, 71, std::string(entry.human == 1 ? "Blancas" : "Negras") + " / " + (entry.result == chess::Result::Draw ? "Tablas" : (entry.result == chess::Result::WhiteWin) == (entry.human == 1) ? "Victoria" : "Derrota") + " / " + entry.reason, muted, 1, 49);
            r.text(12, 86, entry.date.empty() ? "Fecha no disponible" : entry.date, muted, 1, 49);
            if (archiveState.games.size() == archive::Capacity) r.text(12, 115, "LLENA: NO SE REEMPLAZAN PARTIDAS", red, 1, 49);
            r.text(12, 101, "ID " + std::to_string(entry.id) + " / " + chessclock::presets[entry.clock.preset].name, muted, 1, 49);
        } else paragraph(r, 12, 64, archiveStore.corrupt ? "Archivo danado. Perfil y partida normal conservados." : "Las nuevas partidas terminadas aparecen aqui. Al llenarse, debes eliminar una antes de guardar otra.", muted, 49, 4);
        menu({"Repasar partida", "Exportar PGN", "Eliminar partida...", "Volver"});
    } else if (view == View::ConfirmDelete) {
        r.text(12, 64, "ELIMINAR ESTA PARTIDA?", gold, 1, 49);
        paragraph(r, 12, 85, "Se elimina solo el registro. Tus estadisticas y la partida normal se conservan.", muted, 49, 2);
        menu({"Si, eliminar", "Cancelar"});
    } else if (view == View::AdvancedStats) {
        const auto &stats = archiveState.statistics;
        const auto &rival = stats.byOpponent[size_t(statisticsBot)];
        const auto total = stats.total.wins + stats.total.losses + stats.total.draws;
        r.text(12, 41, "ESTADISTICAS DESDE ESTA ACTUALIZACION", gold, 1, 49);
        r.text(12, 58, "V " + std::to_string(stats.total.wins) + " D " + std::to_string(stats.total.losses) + " T " + std::to_string(stats.total.draws) + " / " + std::to_string(total ? stats.total.wins * 100 / total : 0) + "% victorias", ink, 1, 49);
        r.text(12, 75, "Racha " + std::to_string(stats.currentStreak) + " / mejor " + std::to_string(stats.bestStreak), muted, 1, 49);
        auto count = [](const archive::Counters &c) { return c.wins + c.losses + c.draws; };
        r.text(12, 92, "Blancas " + std::to_string(count(stats.byColor[0])) + " / negras " + std::to_string(count(stats.byColor[1])), muted, 1, 49);
        r.text(12, 112, "< " + std::string(ai::botAt(statisticsBot).name) + " >", gold, 1, 49);
        r.text(12, 129, "V " + std::to_string(rival.wins) + " D " + std::to_string(rival.losses) + " T " + std::to_string(rival.draws), ink, 1, 49);
        std::string levels = "Victorias niveles 1-6: ";
        for (auto n : stats.winsByLevel) levels += std::to_string(n) + " ";
        r.text(12, 148, levels, muted, 1, 49);
        r.text(12, 168, "Torneos " + std::to_string(tournamentState.completedTournaments) + " / trofeos " + std::to_string(tournamentState.trophies), muted, 1, 49);
        r.text(12, 186, "Academia " + std::to_string(academyStore.progress.count()) + "/10 (entrenamiento)", muted, 1, 49);
        r.text(12, 204, "IZQ/DER: RIVAL  B: LOGROS", gold, 1, 49);
    } else if (view == View::Training) {
        menu({"Puzzles de ajedrez", "Campana: ruta de rivales", "Retos especiales", "Academia interactiva", "Torneos offline", "Biblioteca de partidas"});
        r.text(12, 192, "Tu partida normal queda guardada.", muted, 1, 49);
        r.text(12, 207, "Progreso separado; sin monedas.", muted, 1, 49);
    } else if (view == View::Lessons && academyMode) {
        const auto &lesson = academy::lesson(lessonIndex);
        r.text(12, 42, "ACADEMIA < " + std::to_string(lessonIndex + 1) + "/10 >", gold, 1, 49);
        r.text(12, 59, lesson.name, ink, 1, 49);
        paragraph(r, 12, 78, lesson.introduction, muted, 49, 3);
        paragraph(r, 12, 115, lesson.objective, gold, 49, 2);
        r.text(12, 148, std::to_string(academyStore.progress.count()) + "/10 completas / " + (academyStore.progress.completed(lessonIndex) ? "COMPLETADA" : academyStore.progress.unlocked(lessonIndex) ? "DISPONIBLE" : "BLOQUEADA"), muted, 1, 49);
        menu({"Comenzar leccion", "Volver"});
    } else if (view == View::Lessons) {
        const bool campaign = trainingKind == training::Kind::Campaign;
        const auto &exercise = training::exercise(campaign ? training::Kind::Puzzle : trainingKind, campaign ? 0 : lessonIndex);
        const std::string name = campaign ? training::chapter(lessonIndex).name : exercise.name;
        r.text(12, 42, "< " + std::to_string(lessonIndex + 1) + "/" + std::to_string(lessonCount()) + " > " + name, gold, 1, 49);
        const int mentor = campaign ? training::chapter(lessonIndex).botId : exercise.botId;
        r.portrait(mentor, 0, 16, 64, 2);
        r.rawText(90, 67, ai::botAt(mentor).name, ink, 1, 35);
        paragraph(r, 90, 86, campaign ? "Vence para abrir la siguiente etapa." : exercise.description, muted, 35, 4);
        r.text(12, 139, trainingStore.progress.completed(trainingKind, lessonIndex) ? "COMPLETADO" : trainingStore.progress.unlocked(trainingKind, lessonIndex) ? "DISPONIBLE" : "COMPLETA LA ETAPA ANTERIOR", gold, 1, 49);
        menu({"Comenzar", "Volver"});
    } else if (view == View::TrainingEnd) {
        r.text(12, 43, academyMode ? "LECCION COMPLETADA" : "RETO COMPLETADO", gold, 2, 24);
        paragraph(r, 12, 76, trainingMessage, ink, 49, 2);
        r.text(12, 101, trainingProgressSaved ? "Progreso guardado en entrenamiento." : "Error SD: progreso sin guardar.", trainingProgressSaved ? muted : red, 1, 49);
        menu({"Reintentar", academyMode ? "Siguiente leccion" : "Siguiente reto", "Volver al catalogo"});
    } else if (view == View::Tools) {
        menu({"Repasar jugadas", "Deshacer (solo practica)", "Exportar PGN", "Volver a pausa"});
        r.text(12, 164, "Practica: sin monedas ni logros.", muted, 1, 49);
    } else if (view == View::Shop) {
        auto &item = economy::products[shopIndex];
        r.text(12, 41, "< " + std::string(item.name) + " >", gold, 1, 49);
        r.text(12,58,shopIndex==economy::NullBoard?"Premio: vence a NULL":"PRECIO "+std::to_string(item.price)+" CC",muted,1,27);
        r.text(180, 58, (state.profile.unlocked & (1u << shopIndex)) ? "DESBLOQUEADO" : "BLOQUEADO",
               muted, 1, 21);
        const Color lights[] = {rgb(179, 169, 130), rgb(118, 174, 193), rgb(176, 194, 144),
                                rgb(190, 173, 212), rgb(209, 180, 107)};
        const Color darks[] = {rgb(88, 86, 96), rgb(32, 61, 101), rgb(53, 83, 67), rgb(80, 55, 108),
                               rgb(97, 69, 41)};
        int palette = item.category == 0 ? item.palette : state.profile.equipped[0];
        for (int y = 0; y < 3; ++y)
            for (int x = 0; x < 8; ++x)
                r.rect(64 + x * 24, 78 + y * 24, 24, 24,
                       (x+y)%2 ? (palette==5||palette==economy::NullBoard?rgb(105,69,135):lights[std::clamp(palette,0,4)]) : (palette==5||palette==economy::NullBoard?rgb(23,18,35):darks[std::clamp(palette,0,4)]));
        r.piece(chess::Knight, 87, 102, item.category == 1 ? item.palette : 0);
        r.piece(-chess::Queen, 183, 102, item.category == 1 ? item.palette : 0);
        if (item.category == 2)
            r.rect(64, 78, 192, 4, item.palette == 1 ? rgb(205, 193, 182) : gold);
        if (item.category == 3)
            r.border(62, 76, 196, 76, item.palette == 2 ? gold : blue, 2);
        if (item.category == 4)
            r.rect(64, 78, 192, 4, item.palette == 1 ? blue : rgb(100, 180, 100));
        menu({"Comprar", "Equipar", "Volver"});
    } else if (view == View::Options) {
        menu({std::string("Notacion: ") + (state.settings.spanish ? "espanola" : "internacional"),
              std::string("Efectos: ") + (state.settings.sound ? "activados" : "desactivados"),
              std::string("Musica: ") + (state.settings.music ? "activada" : "desactivada"),
              "Restaurar estilo clasico", "Volver", "Accesibilidad / informacion",
              developerEnabled ? "Developer: activo (desactivar)" : "Developer: bloqueado", "Idioma / Language"});
        r.text(12, 213, audioReady ? "AUDIO DSP DISPONIBLE" : "SIN DSP: CONTINUA EN SILENCIO",
               muted, 1, 49);
    } else if (view == View::DeveloperPassword) {
        r.text(12, 46, "ACTIVAR DEVELOPER", gold);
        r.text(12, 67, "Codigo de cuatro digitos", ink);
        for (int i = 0; i < 4; ++i) {
            int x = 48 + i * 56;
            button(r, x, 104, 44, std::to_string(developerCode[size_t(i)]), i == developerDigit);
        }
        paragraph(r, 12, 146, "IZQ/DER: digito. ARRIBA/ABAJO: valor. A: confirmar. B: cancelar.", muted, 49, 3);
        button(r,12,181,296,"Confirmar codigo",false);
        button(r,12,205,296,"Cancelar",false);
    } else if (view == View::Developer) {
        menu({"Ganar partida de prueba", "Forzar tablas de prueba", "Perder partida de prueba",
              "Cosmeticos y monedas de prueba", "Abrir historial", "Volver al tablero", "Diagnosticos"});
        paragraph(r, 12, 211, "Los trucos son temporales.", gold, 49, 1);
    } else if (view == View::Preferences) {
        menu({std::string("Reducir animaciones: ") + (preferences.reducedMotion ? "si" : "no"),
              std::string("Informacion de pieza: ") + (preferences.pieceInfo ? "si" : "no"),
              std::string("Estilo IA: ") + (preferences.styleOverride < 0 ? "por rival" : ai::styleName(ai::Style(preferences.styleOverride))), "Volver"});
        paragraph(r, 12, 148, developerEnabled ? "SELECT abre Developer. El historial sigue disponible dentro del panel y en Herramientas." : "SELECT durante la partida abre el historial. Las opciones no cambian las reglas ni el reloj.", muted, 49, 3);
    } else if (view == View::Stats) {
        auto &p = state.profile;
        r.text(12, 41,
               "V " + std::to_string(p.wins) + "  D " + std::to_string(p.losses) + "  T " +
                   std::to_string(p.draws),
               gold);
        r.text(12, 57,
               "PARTIDAS " + std::to_string(p.completed) + " / MIN " +
                   std::to_string(p.seconds / 60),
               muted, 1, 49);
        r.text(12, 73,
               std::string("MAYOR NIVEL: ") +
                   (p.highest < 0 ? "NINGUNO" : ai::levels[p.highest].name),
               muted, 1, 49);
        const char *names[] = {"Primera victoria", "10 victorias",   "50 partidas",
                               "Vencer avanzado",  "Primer enroque", "Primera promocion",
                               "Primer jaque mate", "ERROR 404"};
        for (int i = 0; i < 8; ++i)
            r.text(12, 96 + i * 13,
                   std::string(p.achievements & (1u << i) ? "[X] " : "[ ] ") + names[i],
                   p.achievements & (1u << i) ? gold : muted);
        button(r, 12, 205, 296, "Volver / SELECT: estadisticas nuevas", false);
    } else if (view == View::Promotion) {
        r.text(12, 43, "ELIGE PROMOCION", gold);
        menu({"Dama / Queen", "Torre / Rook", "Alfil / Bishop", "Caballo / Knight"});
    } else {
        std::string title = view == View::ConfirmResign ? "CONFIRMAR RENDICION"
                            : view == View::OfferDraw   ? "RIVAL: ACEPTAS LAS TABLAS?"
                            : view == View::ConfirmNew  ? "REEMPLAZAR PARTIDA ACTUAL?"
                                                        : "GUARDAR Y CERRAR EL JUEGO?";
        r.text(12, 65, title, gold, 1, 49);
        r.text(12, 86,
               view == View::ConfirmNew ? "Se perdera el progreso de esa partida."
                                        : "Puedes cancelar con el boton B.",
               muted, 1, 49);
        menu({"Si, confirmar", "Cancelar"});
    }
    // Only actual AI-AI tournament pairings get a contextual line. The bracket
    // remains visible; there is no popup and no player win/defeat cinematic.
    const char *interaction = nullptr;
    int interactionSpeaker = 40;
    if (view == View::Tournament && tournamentState.size &&
        !tournamentState.finished() && !tournamentState.playerMatch()) {
        const int white = tournamentState.entrants[size_t(tournamentState.whiteEntrant())];
        const int black = tournamentState.entrants[size_t(tournamentState.blackEntrant())];
        const auto event = tournamentState.game.moves.empty() ? ai::Dialogue::Greeting : ai::Dialogue::Thinking;
        interaction = ai::rivalInteraction(white, black, event);
        interactionSpeaker = white;
        if (!interaction) { interaction = ai::rivalInteraction(black, white, event); interactionSpeaker = black; }
    }
    if (noticeFrames > 0) {
        if (view == View::ProfileSlot && notice.rfind("PROFILE_", 0) == 0)
            r.text(12, 214, profileManager.error, red, 1, 49);
        r.text(12, 226, notice, flash ? gold : muted, 1, 49);
    }
    else if (interaction)
        r.rawText(12, 226, std::string(interactionSpeaker == 40 ? "Elon" :
                    interactionSpeaker == 41 ? "Kasparov" : interactionSpeaker == 42 ? "Tal" :
                    interactionSpeaker == 44 ? "Ada" : interactionSpeaker == 45 ? "Wesker" :
                    interactionSpeaker == 48 ? "Linus" : ai::bots[interactionSpeaker].name) +
                    ": " + localization::translate(interaction), blue, 1, 49);
    else
        r.text(12, 226,
               view == View::Home ? "SELECT: CARRERA / SUPERVIVENCIA / TROFEOS"
               : view == View::Openings ? "IZQ/DER: APERTURA  A: ELEGIR  B: VOLVER"
               : view == View::OpeningStudy ? "IZQ/DER: REPASAR LINEA  B: CATALOGO"
               : view == View::Trophies ? "IZQ/DER: TROFEO  A/B: VOLVER"
               : view == View::Review ? "IZQ/DER: REPASAR  ARRIBA/ABAJO: INICIO/FIN"
               : view == View::Lessons ? "IZQ/DER: RETO  A: ELEGIR  B: VOLVER"
               : view == View::Game   ? "A: ELEGIR  B: CANCELAR  X: PISTA"
               : view == View::Shop ? "IZQ/DER: ARTICULO  A: ELEGIR  B: VOLVER"
               : view == View::Bots ? "IZQ/DER: RIVAL  A: ELEGIR  B: VOLVER"
                                    : "CRUCETA: NAVEGAR  A: ELEGIR  B: VOLVER",
               muted, 1, 49);
}
void App::drawBattery(Renderer &r) {
    const auto &value=battery.reading();
    if (!value.available) return;
    r.border(282,1,23,8,muted);r.rect(305,3,2,4,muted);
    for(int i=0;i<int(value.level);++i) r.rect(284+i*4,3,3,4,value.level<=1?red:blue);
    if(value.charging)r.text(273,1,"+",gold,1,1);
    if(battery.warning()){r.rect(0,211,320,11,panel);r.text(6,213,"BATERIA BAJA - GUARDA TU PROGRESO",gold,1,49);}
}
void App::draw(Renderer &r) {
    localization::setLanguage(currentLanguage);
    r.frame();
    if ((view == View::ComposerWorkshop && composerUI) || (view == View::VariantWorkshop && variantsUI)) {
        const int skin=state.profile.equipped[1]==5?1:state.profile.equipped[1]==6?2:0;
        if(view==View::ComposerWorkshop)composerUI->draw(r,skin);else variantsUI->draw(r,skin);
        drawBattery(r);r.end();return;
    }
    if (view == View::Modes || view == View::Career || view == View::Survival || view == View::ConfirmEndAttempt || view == View::Trophies) {
        drawProgressionTop(r);
    } else if (view == View::Profiles || view == View::ProfileSlot || view == View::ConfirmProfileDelete) {
        r.screenBegin(0, bg);
        r.text(12,12,"TRES PERFILES",gold,2,30);
        for (int i=0;i<3;++i) {
            const auto &slot = profileManager.slots[size_t(i)];
            const int x=12+i*130;
            r.rect(x,62,122,142,panel);
            if (slot.occupied) r.portrait(slot.avatar,0,x+29,76,2);
            r.text(x+5,151,"PERFIL " + std::to_string(i+1),gold,1,18);
            paragraph(r,x+5,170,slot.occupied ? slot.name : localization::translate("Ranura vacia"),ink,18,2,true);
            if ((view==View::Profiles ? focus : profileChoice)==i) r.border(x,62,122,142,blue);
        }
    } else if (view == View::Tournament) {
        r.screenBegin(0, bg);
        r.text(12, 9, "CUADRO DEL TORNEO", gold, 2, 30);
        auto label = [&](int entrant) -> std::string {
            if (entrant < 0) return "Por decidir";
            int id = tournamentState.entrants[size_t(entrant)];
            return "#" + std::to_string(entrant + 1) + " " + (id == -1 ? std::string("Tu") : ai::botAt(id).name);
        };
        const int n = tournamentState.size;
        if (!n) r.text(12, 70, "4 U 8 PARTICIPANTES / SIN CONEXION", muted);
        int index = 0;
        for (int round = 0, count = n / 2; count > 0; ++round, count /= 2) {
            const int x = 8 + round * 132;
            r.text(x, 32, count == 1 ? "FINAL" : count == 2 ? "SEMIFINAL" : "CUARTOS", gold, 1, 20);
            for (int j = 0; j < count; ++j, ++index) {
                const auto &match = tournamentState.matches[size_t(index)];
                const int y = 54 + (2 * j + 1) * 176 / (2 * count) - 19;
                r.rect(x, y, 124, 38, panel);
                r.text(x + 3, y + 3, label(match.left), match.winner == match.left && match.left >= 0 ? gold : ink, 1, 19);
                r.text(x + 3, y + 17, label(match.right), match.winner == match.right && match.right >= 0 ? gold : ink, 1, 19);
                if (index == tournamentState.activeIndex) r.border(x, y, 124, 38, blue);
            }
        }
    } else if (view == View::Language || view == View::Title || view == View::Home || view == View::Training || view == View::Lessons || view == View::TrainingEnd || view == View::Library || view == View::AdvancedStats || view == View::ConfirmDelete || view == View::Openings || view == View::Profiles || view == View::ProfileSlot || view == View::ConfirmProfileDelete)
        drawTitleTop(r, preferences.reducedMotion ? 0 : ticks);
    else if (view == View::Help)
        drawHelpTop(r, helpPage);
    else if (view == View::Result) {
        int skin = state.profile.equipped[1] == 5 ? 1 : state.profile.equipped[1] == 6 ? 2 : 0;
        endScene.drawTop(r, skin);
    } else
        board(r);
    bottom(r);
    drawBattery(r);
    drawUnlock(r);
    r.end();
}
} // namespace ui
