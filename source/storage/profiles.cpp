#include "storage/profiles.hpp"
#include "storage/store.hpp"
#include "diagnostics/log.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace profiles {
namespace {
constexpr size_t MaxFile = 4 * 1024 * 1024;
constexpr uint64_t MaxGeneration = 1000000000;
uint32_t checksum(const std::string &bytes) {
    uint32_t sum = 2166136261u;
    for (unsigned char c : bytes) { sum ^= c; sum *= 16777619u; }
    return sum;
}
bool exists(const std::string &path) { struct stat s; return lstat(path.c_str(), &s) == 0; }
bool directory(const std::string &path) {
    if (::mkdir(path.c_str(), 0777) == 0) return true;
    struct stat s;
    const int mkdirError = errno;
    if (lstat(path.c_str(), &s) == 0 && S_ISDIR(s.st_mode)) return true;
    errno = mkdirError;
    return false;
}
bool read(const std::string &path, std::string &out, size_t limit = MaxFile) {
    struct stat s;
    if (lstat(path.c_str(), &s) != 0 || !S_ISREG(s.st_mode) || s.st_size < 0 ||
        uint64_t(s.st_size) > limit) return false;
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::string bytes(size_t(s.st_size), '\0');
    if (!bytes.empty()) f.read(&bytes[0], std::streamsize(bytes.size()));
    if (!f || f.peek() != std::char_traits<char>::eof()) return false;
    out = std::move(bytes); return true;
}
bool write(const std::string &path, const std::string &bytes,
           storage::SaveFault fault = storage::SaveFault::None) {
    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    const auto amount = fault == storage::SaveFault::Write ? bytes.size()/2 : bytes.size();
    bool ok = std::fwrite(bytes.data(), 1, amount, f) == bytes.size();
    if (std::fflush(f) != 0 || fault == storage::SaveFault::Flush) ok = false;
    if (fsync(fileno(f)) != 0 || fault == storage::SaveFault::Sync) ok = false;
    if (std::fclose(f) != 0 || fault == storage::SaveFault::Close) ok = false;
    if (!ok) return false;
    // Verify in fixed-size blocks rather than allocating a second 4 MiB archive.
    std::ifstream check(path, std::ios::binary);
    char block[4096];
    for (size_t offset = 0; offset < bytes.size();) {
        const auto length = std::min(sizeof(block), bytes.size()-offset);
        check.read(block, std::streamsize(length));
        if (!check || !std::equal(block, block+length, bytes.begin()+offset)) return false;
        offset += length;
    }
    return check && check.peek() == std::char_traits<char>::eof();
}
std::string encode(const std::array<Slot, capacity> &slots, bool migrated) {
    std::ostringstream body;
    body << "PROFILES1 " << (migrated ? 1 : 0) << '\n';
    for (const auto &slot : slots)
        body << (slot.occupied ? 1 : 0) << ' ' << slot.generation << ' '
             << slot.avatar << ' ' << std::quoted(slot.name) << '\n';
    const auto bytes = body.str();
    return bytes + std::to_string(checksum(bytes)) + "\n";
}
bool decode(const std::string &bytes, std::array<Slot, capacity> &slots, bool &migrated) {
    if (bytes.size() > 1024 || bytes.size() < 10 || bytes.back() != '\n') return false;
    auto end = bytes.rfind('\n', bytes.size()-2);
    if (end == std::string::npos) return false;
    std::istringstream tail(bytes.substr(end+1)); uint64_t sum; std::string extra;
    if (!(tail >> sum) || (tail >> extra) || sum != checksum(bytes.substr(0,end+1))) return false;
    std::istringstream in(bytes.substr(0,end+1)); std::string magic; int migration;
    if (!(in >> magic >> migration) || magic != "PROFILES1" || (migration != 0 && migration != 1)) return false;
    std::array<Slot, capacity> candidate{};
    for (auto &s : candidate) {
        int occupied;
        if (!(in >> occupied >> s.generation >> s.avatar >> std::quoted(s.name)) ||
            (occupied != 0 && occupied != 1) || s.generation > MaxGeneration ||
            s.avatar < 0 || s.avatar >= 40) return false;
        s.occupied = occupied != 0;
        if (s.occupied ? (!s.generation || !validName(s.name)) : (!s.name.empty() || s.avatar != 0)) return false;
    }
    if (in >> extra) return false;
    slots = std::move(candidate); migrated = migration != 0; return true;
}
bool registry(const std::string &file, std::array<Slot, capacity> &slots, bool &migrated) {
    std::string bytes; return read(file, bytes, 1024) && decode(bytes, slots, migrated);
}
}
bool validName(const std::string &name) {
    if (name.empty() || name.size() > 24) return false;
    bool visible = false;
    for (unsigned char c : name) {
        if (c < 32 || c > 126) return false;
        visible = visible || c != ' ';
    }
    return visible;
}
std::string Manager::generationPath(int slot, uint64_t generation) const {
    return root + "/profiles/slot_0" + std::to_string(slot+1) + "/generation_" + std::to_string(generation);
}
std::string Manager::path(int slot) const {
    return slot >= 0 && slot < capacity && slots[size_t(slot)].occupied
        ? generationPath(slot, slots[size_t(slot)].generation) : std::string();
}
bool Manager::init(bool readOnly) {
    error.clear(); recovered = false; ready = false;
    const auto base = root + "/profiles";
    if (!readOnly && (!directory(root) || !directory(base))) {
        diagnostics::journal().add(diagnostics::Level::Error, "PROFILE_DIRECTORY_FAILED", "mkdir_profiles", errno);
        error = "No se puede abrir perfiles"; return false;
    }
    std::array<Slot, capacity> candidate{}; bool migration = false;
    const auto file = base + "/index.dat";
    if (!registry(file,candidate,migration)) {
        if (registry(file+".bak",candidate,migration)) recovered = true;
        else if (exists(file) || exists(file+".bak")) { diagnostics::journal().add(diagnostics::Level::Error, "PROFILE_INDEX_CORRUPT", "index_and_backup"); error = "Indice de perfiles danado; archivos conservados"; return false; }
    }
    slots = std::move(candidate); migrated = migration; ready = !readOnly;
    return true;
}
bool Manager::publish(const std::array<Slot, capacity> &candidate, bool migration,
                      storage::SaveFault fault) {
    if (!ready) { error = "Perfiles no inicializados"; return false; }
    const auto file = root + "/profiles/index.dat", tmp = file+".tmp";
    const auto bytes = encode(candidate,migration);
    std::array<Slot, capacity> verified{}; bool verifiedMigration;
    if (!decode(bytes,verified,verifiedMigration)) { error = "Datos de perfiles no validos"; return false; }
    if (*createCode) createCode = "PROFILE_INDEX_WRITE_FAILED";
    if (!write(tmp,bytes,fault)) { const int result = errno; std::remove(tmp.c_str()); diagnostics::journal().add(diagnostics::Level::Error, "PROFILE_INDEX_WRITE_FAILED", "index_temporary", result); error = "Error al escribir perfiles"; return false; }
    if (*createCode) createCode = "PROFILE_INDEX_BACKUP_FAILED";
    if (fault == storage::SaveFault::Backup) {
        diagnostics::journal().add(diagnostics::Level::Error, "PROFILE_INDEX_BACKUP_FAILED", "backup_injected", EIO);
        std::remove(tmp.c_str()); error = "Respaldo de perfiles pendiente"; return false;
    }
    std::array<Slot, capacity> old{}; bool oldMigration;
    std::string oldBytes;
    if (registry(file,old,oldMigration)) {
        if (!read(file,oldBytes,1024) || !write(file+".bak.tmp",oldBytes) ||
            std::rename((file+".bak.tmp").c_str(),(file+".bak").c_str()) != 0) {
            diagnostics::journal().add(diagnostics::Level::Error, "PROFILE_INDEX_BACKUP_FAILED", "index_backup", errno);
            std::remove(tmp.c_str()); error = "Error al respaldar perfiles"; return false;
        }
    }
    if (*createCode) createCode = "PROFILE_INDEX_COMMIT_FAILED";
    if (fault == storage::SaveFault::Commit || std::rename(tmp.c_str(),file.c_str()) != 0) {
        diagnostics::journal().add(diagnostics::Level::Error, "PROFILE_INDEX_COMMIT_FAILED", "index_rename", fault == storage::SaveFault::Commit ? EIO : errno);
        std::remove(tmp.c_str()); error = "Error al confirmar perfiles"; return false;
    }
    slots = candidate; migrated = migration; error.clear(); return true;
}
bool Manager::reserve(int slot, uint64_t &generation, std::string &out) {
    const auto base = root + "/profiles/slot_0" + std::to_string(slot+1);
    if (!directory(base)) { diagnostics::journal().add(diagnostics::Level::Error, "PROFILE_SLOT_MKDIR_FAILED", "slot_directory", errno); error = "No se puede crear la ranura"; return false; }
    generation = slots[size_t(slot)].generation;
    for (int tries = 0; tries < 1024 && generation < MaxGeneration; ++tries) {
        ++generation; out = generationPath(slot,generation);
        if (::mkdir(out.c_str(),0777) == 0) return true;
        if (errno != EEXIST) break;
    }
    diagnostics::journal().add(diagnostics::Level::Error, "PROFILE_RESERVE_FAILED", "generation_directory", errno);
    error = "No se puede reservar un perfil nuevo"; return false;
}
bool Manager::create(int slot, const std::string &name, int avatar, storage::SaveFault fault) {
    using diagnostics::Level;
    auto &log = diagnostics::journal();
    error.clear();
    createCode = "PROFILE_SLOT_INVALID";
    log.add(Level::Info, "PROFILE_CREATE_REQUEST", "slot_zero_based", slot);
    if (!ready || slot < 0 || slot >= capacity || slots[size_t(slot)].occupied || avatar < 0 || avatar >= 40) {
        error = "Perfil o ranura no valido";
        log.add(Level::Error, createCode, "slot_validation", slot);
        return false;
    }
    if (!validName(name)) {
        createCode = "PROFILE_NAME_INVALID";
        error = "Nombre o perfil no valido";
        log.add(Level::Error, createCode, "name_validation");
        return false;
    }
    log.add(Level::Debug, "PROFILE_NAME_VALIDATED", "name_omitted");
    std::string target;
    uint64_t generation;
    createCode = "PROFILE_RESERVE_FAILED";
    if (!reserve(slot, generation, target)) return false;
    log.add(Level::Debug, "PROFILE_DIR_READY", "generation_reserved");
    economy::State state, check;
    state.profile.name = name;
    storage::Store store(target);
    createCode = "PROFILE_DATA_WRITE_FAILED";
    log.add(Level::Debug, "PROFILE_DATA_READY", "initial_state");
    if (!store.save(state)) {
        error = "Error al escribir perfiles";
        return false;
    }
    createCode = "PROFILE_VERIFY_FAILED";
    if (!store.load(check) || storage::encode(state) != storage::encode(check)) {
        log.add(Level::Error, createCode, "reopen_and_compare");
        error = "No se pudo verificar el perfil nuevo";
        return false;
    }
    auto candidate = slots;
    candidate[size_t(slot)] = {true, name, avatar, generation};
    createCode = "PROFILE_INDEX_COMMIT_FAILED";
    if (!publish(candidate, migrated, fault)) return false;
    createCode = "";
    log.add(Level::Info, "PROFILE_CREATED", "selector_published", slot);
    return true;
}
bool Manager::rename(int slot, const std::string &name, storage::SaveFault fault) {
    if (!ready || slot < 0 || slot >= capacity || !slots[size_t(slot)].occupied || !validName(name)) {
        error = "Nombre o perfil no valido"; return false;
    }
    auto candidate = slots; candidate[size_t(slot)].name = name;
    return publish(candidate,migrated,fault);
}
bool Manager::setAvatar(int slot, int avatar, storage::SaveFault fault) {
    if (!ready || slot < 0 || slot >= capacity || !slots[size_t(slot)].occupied ||
        avatar < 0 || avatar >= 40) { error = "Avatar o perfil no valido"; return false; }
    auto candidate = slots; candidate[size_t(slot)].avatar = avatar;
    return publish(candidate,migrated,fault);
}
bool Manager::remove(int slot, storage::SaveFault fault) {
    if (!ready || slot < 0 || slot >= capacity || !slots[size_t(slot)].occupied) {
        error = "Perfil no disponible"; return false;
    }
    auto candidate = slots;
    candidate[size_t(slot)] = {false,"",0,slots[size_t(slot)].generation};
    return publish(candidate,migrated,fault);
}
bool Manager::migrate(storage::SaveFault fault) {
    error.clear();
    if (!ready) { error = "Perfiles no inicializados"; return false; }
    if (migrated) return true;
    if (!exists(root+"/profile.dat") && !exists(root+"/profile.dat.bak"))
        return publish(slots,true,fault);
    economy::State old;
    storage::Store original(root);
    if (!original.load(old)) { error = "Perfil antiguo danado; originales conservados"; return false; }
    int slot = 0;
    while (slot < capacity && slots[size_t(slot)].occupied) ++slot;
    if (slot == capacity) { error = "No hay ranura libre para migrar"; return false; }
    std::string target; uint64_t generation;
    if (!reserve(slot,generation,target)) return false;
    // Snapshot the complete known 0.8.x/0.9.0 progress family, including backups.
    // Corrupt optional sidecars remain byte-for-byte recoverable and their owning
    // modules decide recovery; the main profile must decode before publication.
    const char *names[] = {"profile.dat","training.dat","academy.dat","tournament.dat","archive.dat","ui.dat","progression.dat","openings.dat","composer.dat","variants.dat","language.dat","space.dat","null.dat"};
    for (const char *name : names) {
        for (const char *suffix : {"", ".bak"}) {
            const auto source = root+"/"+name+suffix;
            if (!exists(source)) continue;
            std::string bytes;
            if (!read(source,bytes) || !write(target+"/"+name+suffix,bytes,fault)) {
                error = "Migracion interrumpida; originales conservados"; return false;
            }
        }
    }
    economy::State check; storage::Store copied(target);
    if (!copied.load(check) || storage::encode(check) != storage::encode(old)) {
        error = "Migracion no verificable; originales conservados"; return false;
    }
    auto candidate = slots;
    candidate[size_t(slot)] = {true,old.profile.name,0,generation};
    return publish(candidate,true,fault);
}
} // namespace profiles
