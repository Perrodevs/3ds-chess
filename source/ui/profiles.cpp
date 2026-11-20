#include "diagnostics/log.hpp"
#include "ui/app.hpp"
namespace ui {
bool App::selectProfile(int index) {
    if (!profilesReady || index < 0 || index >= profiles::capacity ||
        !profileManager.slots[size_t(index)].occupied) return false;
    diagnostics::journal().add(diagnostics::Level::Info, "PROFILE_SWITCH_REQUEST", "slot_zero_based", index);
    economy::State candidate;
    storage::Store candidateStore(profileManager.path(index));
    if (!candidateStore.load(candidate)) {
        diagnostics::journal().add(diagnostics::Level::Error, "PROFILE_ACTIVATE_FAILED", "slot_reload", index);
        message("Perfil danado: prueba otra ranura; archivos conservados"); return false;
    }
    if (activeProfile >= 0 && !save()) return false;
    tournamentController.stop(); bot.stop(); hints.cancel();
    developerEnabled = false; developerSandbox = false; developerOriginal = {};
    openingActive = openingPractice = false;
    trainingActive = false; tournamentActive = false; progressionActive = false; academyMode = false;
    dataDirectory = profileManager.path(index);
    store = storage::Store(dataDirectory);
    trainingStore = training::ProgressStore(dataDirectory + "/training.dat");
    academyStore = academy::Store(dataDirectory + "/academy.dat");
    tournamentStore = tournament::Store(dataDirectory + "/tournament.dat");
    archiveStore = archive::Store(dataDirectory);
    progressionStore = progression::Store(dataDirectory + "/progression.dat");
    openingStore = openings::Store(dataDirectory + "/openings.dat");
    spaceStore=space::Store(dataDirectory+"/space.dat");
    nullStore=nullboss::Store(dataDirectory+"/null.dat");
    nullReady=false;nullState={};nullPending=false;nullUnlockFrames=pendingNullScene=0;pendingNullReward=false;
    activeProfile = profileChoice = index;
    selected = -1; hintMove = {}; claimIntent = false; animationFrames = 0;
    reviewGame = {}; reviewSource = {}; reviewPly = 0;
    reviewLines.clear(); reviewExplanation.clear();
    loadData();
    state.profile.name = profileManager.slots[size_t(index)].name;
    diagnostics::journal().add(diagnostics::Level::Info, "PROFILE_ACTIVATED", "slot_zero_based", index);
    go(View::Title);
    return true;
}
void App::profileAction() {
    if (!profilesReady) { message("Perfiles no disponibles: " + profileManager.error); return; }
    if (view == View::Profiles) {
        if (focus == 3) { back(); return; }
        profileChoice = focus;
        diagnostics::journal().add(diagnostics::Level::Debug, "PROFILE_SLOT_SELECTED", "slot_zero_based", focus);
        go(View::ProfileSlot); return;
    }
    if (view == View::ConfirmProfileDelete) {
        if (focus != 0) { back(); return; }
        if (!profileManager.remove(profileChoice)) { message("No se pudo retirar el perfil"); return; }
        if (activeProfile == profileChoice) {
            tournamentController.stop(); bot.stop(); hints.cancel();
            activeProfile = -1; developerEnabled = developerSandbox = false;
            trainingActive = tournamentActive = progressionActive = false;
            state = {}; preferences = {}; archiveState = {}; archiveReady = false;
            normalState = {}; developerOriginal = {};
        }
        go(View::Profiles); return;
    }
    const auto &slot = profileManager.slots[size_t(profileChoice)];
    if (focus == 0) {
        if (slot.occupied) selectProfile(profileChoice);
        else {
            diagnostics::journal().add(diagnostics::Level::Info, "PROFILE_KEYBOARD_OPEN", "create", profileChoice);
            wantsName = true;
        }
    } else if (focus == 1) {
        if (!slot.occupied) createProfile(profileNameInitial());
        else if (activeProfile == profileChoice) wantsName = true;
        else message("Entra al perfil antes de renombrarlo");
    } else if (focus == 2) {
        if (!slot.occupied) { message("Crea primero el perfil"); return; }
        if (!profileManager.setAvatar(profileChoice, (slot.avatar + 1) % 40)) message("Avatar sin guardar");
    } else if (focus == 3) {
        if (slot.occupied) go(View::ConfirmProfileDelete);
        else message("La ranura ya esta vacia");
    } else back();
}
std::string App::profileNameInitial() const {
    if (profilesEnabled && view == View::ProfileSlot &&
        !profileManager.slots[size_t(profileChoice)].occupied)
        return localization::translate("Jugador") + std::string(" ") + std::to_string(profileChoice + 1);
    return state.profile.name;
}
void App::cancelProfileName(bool failed, int result) {
    wantsName = false;
    diagnostics::journal().add(failed ? diagnostics::Level::Error : diagnostics::Level::Info,
        failed ? "PROFILE_KEYBOARD_FAILED" : "PROFILE_KEYBOARD_CANCELLED", "software_keyboard", result);
    if (failed) message("Teclado no disponible; usa el nombre sugerido.");
}
void App::createProfile(const std::string &name) {
    if (!profileManager.create(profileChoice, name, profileChoice)) {
        message(profileManager.createCode);
        diagnostics::journal().flush();
        return;
    }
    noticeFrames = 0;
    selectProfile(profileChoice);
}
} // namespace ui
