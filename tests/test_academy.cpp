#include "training/academy.hpp"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <unistd.h>
int main() {
    using training::Attempt;
    academy::Progress progress;
    assert(!progress.markComplete(1));
    int accepted=0;
    for(int id=0;id<academy::lessonCount;++id) {
        academy::Run run;assert(run.start(id));
        assert(run.hint().from>=0);assert(run.feedback()[0]);
        auto before=run.game().position.fen();
        assert(run.submit({-1,99,0})==Attempt::Illegal);assert(run.game().position.fen()==before);
        for(auto m:run.game().position.legal()) {
            academy::Run alternative;assert(alternative.start(id));
            auto result=alternative.submit(m);
            assert(result!=Attempt::Illegal);
            if(result==Attempt::Completed){++accepted;assert(alternative.completed());assert(alternative.feedback()[0]);}
            else {assert(result==Attempt::Incorrect);assert(alternative.game().moves.empty());assert(alternative.game().position.fen()==before);}
        }
        assert(run.submit(run.hint())==Attempt::Completed);assert(run.game().moves.size()==1);
        assert(run.hint().from<0);assert(run.submit({0,1,0})==Attempt::Illegal);
        assert(progress.unlocked(id));assert(progress.markComplete(id));assert(!progress.markComplete(id));
        assert(progress.completed(id));assert(progress.count()==id+1);
        assert(run.start(id));assert(!run.completed());
    }
    assert(accepted>academy::lessonCount); // Legal alternatives, not a fixed answer equality.
    academy::Run run;assert(!run.start(-1));assert(!run.start(10));
    std::string path="/tmp/3ds-academy-"+std::to_string(getpid());
    academy::Store store(path);assert(!store.load());assert(store.progress.count()==0);
    store.progress.markComplete(0);assert(store.save());
    for(auto fault:{storage::SaveFault::Write,storage::SaveFault::Flush,storage::SaveFault::Sync,storage::SaveFault::Close,storage::SaveFault::Backup,storage::SaveFault::Commit}) {
        store.progress.markComplete(1);assert(!store.save(fault));
        academy::Store restored(path);assert(restored.load());assert(restored.progress.lessons==1);
    }
    assert(store.save());
    academy::Store restored(path);assert(restored.load());assert(restored.progress.lessons==3);
    {std::ofstream f(path);f<<"ACADEMY1 1023";}
    assert(restored.load());assert(restored.recovered);assert(restored.progress.lessons==1);
    {std::ofstream f(path+".bak");f<<"corrupt";}
    assert(!restored.load());assert(restored.progress.lessons==0);
    store.progress.lessons=2;assert(!store.save());
    for(auto suffix:{"",".tmp",".bak",".bak.tmp"})std::remove((path+suffix).c_str());
    std::cout<<"Academy: 10 lessons, "<<accepted<<" accepted moves; unlocks, retry, rollback and 6 save failures passed\n";
}
