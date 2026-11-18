#pragma once
#include "ai/hints.hpp"
#include "ai/worker.hpp"
#include "graphics/audio.hpp"
#include "graphics/renderer.hpp"
#include "storage/store.hpp"
#include "storage/profiles.hpp"
#include "ui/end_scene.hpp"
#include "ui/onboarding.hpp"
#include "ui/review.hpp"
#include "ui/preferences.hpp"
#include "tournament/tournament.hpp"
#include "archive/archive.hpp"
#include "progression/progression.hpp"
#include "openings/openings.hpp"
#include "ui/variants_ui.hpp"
#include "ui/composer_ui.hpp"
#include "platform/battery.hpp"
#include "localization/localization.hpp"
#include "space/secret.hpp"
#include <memory>
#include "training/training.hpp"
#include "training/academy.hpp"
#include <chrono>
namespace ui {
enum Key : unsigned {
    Up = 1,
    Down = 2,
    Left = 4,
    Right = 8,
    A = 16,
    B = 32,
    Start = 64,
    Select = 128,
    Hint = 256
};
struct Input {
    unsigned keys = 0;
    bool touch = false;
    int x = 0, y = 0;
};
enum class View {
    Title,
    Help,
    Result,
    Home,
    NewGame,
    Bots,
    Game,
    Pause,
    Shop,
    Options,
    Stats,
    Promotion,
    ConfirmResign,
    OfferDraw,
    ConfirmNew,
    ConfirmQuit,
    Review,
    Tools,
    Training,
    Lessons,
    TrainingEnd,
    Preferences,
    Tournament,
    Library,
    ConfirmDelete,
    AdvancedStats,
    DeveloperPassword,
    Developer,
    Profiles,
    ProfileSlot,
    ConfirmProfileDelete,
    Modes,
    Career,
    Survival,
    ConfirmEndAttempt,
    Trophies,
    Openings,
    OpeningStudy,
    ComposerWorkshop,
    VariantWorkshop,
    Language
};
using ClockSource = std::chrono::steady_clock::time_point (*)(void *);
class App {
    localization::Language currentLanguage=localization::packageDefault, globalLanguage=localization::packageDefault;
    bool firstLanguage=false;
    std::string rootDirectory;
    space::Store spaceStore;
    space::State spaceState;
    bool spaceReady=false, secretPending=false;
    int unlockFrames=0;
    space::Scene pendingScene=space::Scene::None;
    void syncSecret();
    void selectLanguage();
    void drawUnlock(graphics::Renderer &);

    std::unique_ptr<ComposerUI> composerUI;
    std::unique_ptr<VariantsUI> variantsUI;
    platform::Battery battery;
    void drawBattery(graphics::Renderer &);
    bool developerEnabled = false, developerSandbox = false;
    economy::State developerOriginal;
    std::array<int, 4> developerCode{};
    int developerDigit = 0;
    bool beginDeveloperSandbox();
    void developerAction();
    void developerPassword(Input);
    bool profilesEnabled = false, profilesReady = false;
    profiles::Manager profileManager;
    int activeProfile = -1, profileChoice = 0;
    void loadData();
    bool selectProfile(int);
    void profileAction();
    openings::Store openingStore;
    openings::Run openingRun;
    bool openingReady = false, openingActive = false, openingPractice = false;
    int openingIndex = 0, openingBranch = 0, openingPly = 0;
    void beginOpening(bool practice);
    void seekOpening(int);
    void leaveOpening();
    void openingAction();
    void openingMove(chess::Move);
    void drawOpeningBottom(graphics::Renderer &);
    progression::Store progressionStore;
    progression::State progressionState;
    bool progressionReady = false, progressionActive = false;
    int trophyIndex = 0;
    View trophyReturn = View::Modes;
    void beginProgression(progression::Kind);
    void leaveProgression();
    bool reconcileProgressionRewards();
    void progressionAction();
    bool trophyUnlocked(int) const;
    void drawProgressionTop(graphics::Renderer &);
    void drawProgressionBottom(graphics::Renderer &);
    storage::Store store;
    std::string dataDirectory;
    ui::Preferences preferences;
    tournament::Store tournamentStore;
    tournament::Session tournamentState;
    tournament::Controller tournamentController;
    bool tournamentActive = false;
    bool tournamentSaved = true;
    int tournamentSize = 4;
    void beginTournamentMatch();
    void leaveTournament();
    archive::Store archiveStore;
    archive::State archiveState;
    int libraryIndex = 0, statisticsBot = 0;
    bool archiveReady = false;
    void recordCompleted();
    void openLibraryReview();
    ai::Worker bot;
    ai::Hints hints;
    EndScene endScene;
    chess::Move hintMove;
    View helpReturn = View::Title, confirmReturn = View::Home;
    int helpPage = 0;
    training::ProgressStore trainingStore;
    training::Run trainingRun;
    academy::Store academyStore;
    academy::Run academyRun;
    bool academyMode = false;
    void beginAcademy();
    training::Kind trainingKind = training::Kind::Puzzle;
    bool trainingActive = false, trainingProgressSaved = true;
    int lessonIndex = 0;
    economy::State normalState;
    std::string trainingMessage;
    void leaveTraining();
    void beginLesson();
    void trainingComplete();
    int lessonCount() const;
    chess::Game reviewGame, reviewSource;
    size_t reviewPly = 0;
    int reviewHuman = 1, reviewMode = 0;
    std::vector<std::string> reviewLines, reviewExplanation;
    review::Captured gameCaptured, reviewCaptured;
    View reviewReturn = View::Result;
    void openReview(View origin);
    void seekReview(int ply);
    void undoPractice();
    void rematch();
    bool flipped() const { return view == View::Review ? reviewMode != 1 && reviewHuman == -1 : state.session.mode != 1 && state.session.human == -1; }
    int boardFile(int sq) const { return flipped() ? 7 - sq % 8 : sq % 8; }
    int boardRow(int sq) const { return flipped() ? sq / 8 : 7 - sq / 8; }

    void requestHint();
    void openHelp(View origin);
    void showResult();
    graphics::Audio audio;
    int focus = 0, selected = -1, cursor = 12, promotion = 0, shopIndex = 0, animationFrames = 0,
        flash = 0;
    int botChoice = 0, reaction = 0, reactionFrames = 0;
    uint32_t ticks = 0;
    bool clockSuspended = false, explicitClockStep = false;
    ClockSource clockSource = nullptr;
    void *clockContext = nullptr;
    std::chrono::steady_clock::time_point clockNow() const {
        return clockSource ? clockSource(clockContext) : std::chrono::steady_clock::now();
    }
    std::chrono::steady_clock::time_point lastClockSample = std::chrono::steady_clock::now();
    bool clockRunning() const;
    void advanceClock(int64_t elapsedMs);
    void syncClock();
    chess::Move animMove;
    int animPiece = 0;
    bool context = false, claimIntent = false, audioReady = false;
    std::string notice;
    int noticeFrames = 0;
    std::vector<std::string> history;
    ai::Answer lastSearch;
    bool commit(const economy::State &previous);
    void refresh();
    void play(chess::Move);
    void activate();
    void startGame();
    void finish();
    void message(const std::string &);
    void go(View);
    void back();
    int rows() const;
    int rowY() const;
    int rowH() const;
    void board(graphics::Renderer &);
    void bottom(graphics::Renderer &);
    void button(graphics::Renderer &, int, int, int, const std::string &, bool, bool literalLabel=false);

  public:
    economy::State state;
    View view = View::Title;
    bool exitRequested = false, wantsName = false;
    explicit App(const std::string &path, bool useProfiles = false) : rootDirectory(path), spaceStore(path+"/space.dat"), profilesEnabled(useProfiles), profileManager(path), openingStore(path + "/openings.dat"), progressionStore(path + "/progression.dat"), store(path), dataDirectory(path), tournamentStore(path + "/tournament.dat"), archiveStore(path), trainingStore(path + "/training.dat"), academyStore(path + "/academy.dat") {}
    ~App() {
        if (composerUI) composerUI->suspend();
        if (variantsUI) variantsUI->suspend();
        tournamentController.stop();
        bot.stop();
        hints.cancel();
        audio.shutdown();
    }
    bool workshopTextRequested() const { return view == View::ComposerWorkshop && composerUI && composerUI->textRequested(); }
    std::string workshopTextPrompt() const { return composerUI ? composerUI->textPrompt() : std::string{}; }
    std::string workshopTextValue() const { return composerUI ? composerUI->textValue() : std::string{}; }
    void acceptWorkshopText(const std::string &text, bool confirmed) { if (composerUI) composerUI->acceptText(text,confirmed); }
    void setBatterySource(platform::BatterySource source, void *context=nullptr) { battery.setSource(source,context); }
    localization::Language uiLanguage() const { return currentLanguage; }
    bool secretUnlocked() const { return spaceReady && spaceState.unlocked; }
    void init();
    void update(Input, int64_t elapsedMs = -1);
    void draw(graphics::Renderer &);
    int portraitExpression() const;
    bool hintPending() const { return hints.running(); }
    chess::Move suggestedMove() const { return hintMove; }
    void setName(const std::string &);
    bool save();
    void suspend();
    void resume();
    // Deterministic monotonic source for integration tests/platform adapters.
    void setClockSource(ClockSource source, void *context = nullptr) {
        clockSource = source;
        clockContext = context;
        lastClockSample = clockNow();
    }
};
} // namespace ui
