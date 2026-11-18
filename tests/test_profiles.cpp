#include "storage/profiles.hpp"
#include "storage/store.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
namespace fs = std::filesystem;
namespace {
std::string read(const fs::path &p) { std::ifstream f(p,std::ios::binary); return {std::istreambuf_iterator<char>(f),{}}; }
void write(const fs::path &p,const std::string &bytes) { std::ofstream f(p,std::ios::binary); f<<bytes; assert(bool(f)); }
void slots(const std::string &root) {
    profiles::Manager m(root); assert(m.init() && m.migrate());
    assert(m.path(-1).empty() && m.path(3).empty());
    assert(!m.create(-1,"No") && !m.create(3,"No"));
    assert(!m.create(0,"") && !m.create(0,std::string(25,'X')) && !m.create(0,"  "));
    assert(!m.create(0,"../",40) && !m.create(0,"Bad\nName"));
    assert(m.create(0,"Pollito",37));
    assert(m.create(1,"Alex",38));
    assert(m.create(2,"Steve",39));
    assert(!m.create(0,"Fourth"));
    assert(m.path(0) != m.path(1) && m.path(1) != m.path(2));
    economy::State one,two,three;
    storage::Store a(m.path(0)), b(m.path(1)), c(m.path(2));
    assert(a.load(one) && b.load(two) && c.load(three));
    assert(one.profile.name=="Pollito" && two.profile.name=="Alex" && three.profile.name=="Steve");
    assert(!one.session.active && one.profile.coins==0 && two.profile.coins==0);
    one.profile.coins=900; one.profile.completed=one.profile.wins=1;
    assert(one.profile.buy(1)); assert(a.save(one));
    assert(b.load(two) && c.load(three));
    assert(two.profile.coins==0 && two.profile.completed==0 && two.profile.unlocked==1);
    assert(three.profile.coins==0 && three.profile.completed==0 && three.profile.unlocked==1);
    write(fs::path(m.path(0))/"academy.dat","profile-one-academy");
    write(fs::path(m.path(1))/"academy.dat","profile-two-academy");
    const auto former=m.path(1);
    assert(m.rename(1,"Second"));
    assert(m.slots[1].name=="Second");
    assert(m.setAvatar(1,12) && m.slots[1].avatar==12);
    assert(!m.setAvatar(1,40) && !m.setAvatar(3,1));
    assert(m.remove(1)); assert(!m.slots[1].occupied && m.path(1).empty());
    assert(fs::exists(fs::path(former)/"academy.dat"));
    assert(m.create(1,"Replacement",1)); assert(m.path(1)!=former);
    assert(!fs::exists(fs::path(m.path(1))/"academy.dat"));
    assert(b.load(two) && two.profile.name=="Alex"); // old generation retained
    storage::Store fresh(m.path(1)); assert(fresh.load(two));
    assert(two.profile.coins==0 && !two.session.active && two.profile.unlocked==1);
    assert(a.load(one) && one.profile.coins==650 && one.profile.wins==1);
    profiles::Manager restart(root); assert(restart.init() && restart.migrate());
    assert(restart.path(1)==m.path(1) && restart.slots[1].name=="Replacement");
}
void migration(const std::string &root) {
    fs::create_directories(root);
    economy::State legacy; legacy.profile.name="Legacy"; legacy.profile.coins=1234;
    legacy.profile.completed=legacy.profile.wins=5;
    storage::Store original(root); assert(original.save(legacy));
    for(const auto *name:{"training.dat","academy.dat","tournament.dat","archive.dat","ui.dat"}) {
        write(fs::path(root)/name,std::string("payload for ")+name);
        write(fs::path(root)/(std::string(name)+".bak"),std::string("backup ")+name);
    }
    const auto originalBytes=read(fs::path(root)/"profile.dat");
    profiles::Manager m(root); assert(m.init() && m.migrate());
    assert(m.slots[0].occupied && m.slots[0].name=="Legacy");
    economy::State loaded; storage::Store copied(m.path(0)); assert(copied.load(loaded));
    assert(storage::encode(loaded)==storage::encode(legacy));
    for(const auto *name:{"profile.dat","training.dat","academy.dat","tournament.dat","archive.dat","ui.dat"}) {
        assert(read(fs::path(root)/name)==read(fs::path(m.path(0))/name));
    }
    assert(read(fs::path(root)/"profile.dat")==originalBytes);
    const auto path=m.path(0);
    assert(m.migrate() && m.path(0)==path && !m.slots[1].occupied);
    assert(m.remove(0));
    profiles::Manager restart(root); assert(restart.init() && restart.migrate());
    assert(!restart.slots[0].occupied && !restart.slots[1].occupied);
    assert(read(fs::path(root)/"profile.dat")==originalBytes); // no duplicate import after deletion
}
void firstFree(const std::string &root) {
    profiles::Manager m(root); assert(m.init()); assert(m.create(0,"Existing"));
    economy::State legacy; legacy.profile.name="Imported";
    storage::Store old(root); assert(old.save(legacy));
    assert(m.migrate() && m.slots[0].name=="Existing" && m.slots[1].name=="Imported");
}
void faults(const std::string &base) {
    int index=0;
    for(auto fault:{storage::SaveFault::Write,storage::SaveFault::Flush,storage::SaveFault::Sync,
                    storage::SaveFault::Close,storage::SaveFault::Backup,storage::SaveFault::Commit}) {
        const auto root=base+std::to_string(index++);
        profiles::Manager m(root); assert(m.init() && m.migrate() && m.create(0,"Keep"));
        const auto originalPath=m.path(0), originalRegistry=read(fs::path(root)/"profiles/index.dat");
        assert(!m.create(1,"Fail",0,fault));
        assert(!m.rename(0,"Lost",fault));
        assert(!m.setAvatar(0,3,fault));
        assert(!m.remove(0,fault));
        assert(m.slots[0].name=="Keep" && !m.slots[1].occupied);
        assert(read(fs::path(root)/"profiles/index.dat")==originalRegistry);
        profiles::Manager retry(root); assert(retry.init());
        assert(retry.path(0)==originalPath && !retry.slots[1].occupied);
        assert(retry.create(1,"Retry"));
        // A failed import must not publish a slot or modify the legacy source.
        const auto migrateRoot=root+"-migration"; fs::create_directories(migrateRoot);
        economy::State legacy; legacy.profile.coins=22;
        storage::Store old(migrateRoot); assert(old.save(legacy));
        const auto bytes=read(fs::path(migrateRoot)/"profile.dat");
        profiles::Manager migrate(migrateRoot); assert(migrate.init());
        assert(!migrate.migrate(fault) && !migrate.slots[0].occupied);
        assert(read(fs::path(migrateRoot)/"profile.dat")==bytes);
        profiles::Manager recovered(migrateRoot); assert(recovered.init() && recovered.migrate());
        assert(recovered.slots[0].occupied && !recovered.slots[1].occupied);
    }
}
void corruption(const std::string &root) {
    profiles::Manager m(root); assert(m.init() && m.migrate() && m.create(0,"Saved"));
    assert(m.rename(0,"Latest"));
    const auto file=fs::path(root)/"profiles/index.dat";
    const auto good=read(file),backup=read(file.string()+".bak");
    write(file,"truncated");
    profiles::Manager fallback(root); assert(fallback.init() && fallback.recovered);
    assert(fallback.slots[0].name=="Saved");
    write(file.string()+".bak","invalid");
    profiles::Manager broken(root); assert(!broken.init());
    assert(read(file)=="truncated"); // never reset corrupt registry
    fs::remove(file.string()+".bak");
    for(size_t length=0;length<good.size();++length) {
        write(file,good.substr(0,length)); profiles::Manager shortFile(root); assert(!shortFile.init());
    }
    for(size_t offset=0;offset<good.size();++offset) {
        auto bad=good;bad[offset]^=1;write(file,bad);profiles::Manager changed(root);assert(!changed.init());
    }
    write(file,good); assert(m.init());
    // A corrupt sidecar is preserved, not discarded by migration. A corrupt main
    // legacy profile blocks publication and keeps original bytes intact.
    const auto badRoot=root+"-legacy";fs::create_directories(badRoot);
    write(fs::path(badRoot)/"profile.dat","broken");
    profiles::Manager badLegacy(badRoot);assert(badLegacy.init()&&!badLegacy.migrate());
    assert(read(fs::path(badRoot)/"profile.dat")=="broken");
    // Recover the main profile through its own legacy backup.
    economy::State old;old.profile.name="Backup";storage::Store store(badRoot);
    assert(store.save(old)&&store.save(old));write(fs::path(badRoot)/"profile.dat","broken");
    assert(badLegacy.migrate());economy::State check;storage::Store copy(badLegacy.path(0));
    assert(copy.load(check)&&check.profile.name=="Backup"&&copy.recovered);
}
}
int main() {
    char dir[]="/tmp/chess-profiles-XXXXXX";assert(mkdtemp(dir));const std::string root=dir;
    slots(root+"/slots");migration(root+"/migration");firstFree(root+"/first-free");
    faults(root+"/fault-");corruption(root+"/corruption");
    fs::remove_all(root);
    std::cout<<"Profiles: three-slot isolation, generations, migration, retry, corruption and six I/O faults passed\n";
}
