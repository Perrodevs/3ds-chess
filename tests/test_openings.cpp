#include "openings/openings.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
namespace fs=std::filesystem;
namespace {
std::string read(const std::string &file){std::ifstream f(file,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
void write(const std::string &file,const std::string &bytes){std::ofstream f(file,std::ios::binary);f<<bytes;assert(bool(f));}
void lines() {
    using namespace openings;
    assert(Count==10&&branchCount(-1)==0&&branchCount(Count)==0);
    assert(moves(-1).empty()&&moves(Count).empty()&&moves(0,2).empty());
    for(int i=0;i<Count;++i) {
        assert(branchCount(i)==2);
        for(int branch=0;branch<branchCount(i);++branch) {
            const auto &entry=line(i,branch);assert(*entry.name&&*entry.idea&&*entry.goal&&*entry.uci);
            const auto sequence=moves(i,branch);assert(sequence.size()>=8&&sequence.size()<=16);
            chess::Game replay;
            for(size_t ply=0;ply<=sequence.size();++ply) {
                chess::Game at;assert(positionAt(i,branch,ply,at));
                assert(at.position.fen()==replay.position.fen());assert(at.moves==replay.moves);
                assert(at.result==chess::Result::Playing);
                if(ply<sequence.size())assert(replay.play(sequence[ply]));
            }
            const auto prior=replay.position.fen();
            assert(!positionAt(i,branch,sequence.size()+1,replay)&&replay.position.fen()==prior);
            Run run;assert(run.start(i,branch)&&run.ply()==0&&!run.completed());
            assert(run.game().position.fen()==chess::Position::initial().fen());
            assert(run.submit({-1,0,0})==Attempt::Illegal&&run.ply()==0);
            const auto legal=run.game().position.legal();chess::Move wrong;
            for(auto m:legal)if(!(m==sequence[0])){wrong=m;break;}
            assert(wrong.from>=0&&run.submit(wrong)==Attempt::Wrong&&run.ply()==0);
            assert(run.game().position.fen()==chess::Position::initial().fen());
            for(size_t ply=0;ply<sequence.size();++ply) {
                if(ply==1) {
                    const auto before=run.game().position.fen();
                    const auto replies=run.game().position.legal();chess::Move wrongReply;
                    for(auto m:replies)if(!(m==sequence[ply])){wrongReply=m;break;}
                    assert(wrongReply.from>=0&&run.submit(wrongReply)==Attempt::Wrong);
                    assert(run.ply()==ply&&run.game().position.fen()==before);
                }
                assert(run.hint()==sequence[ply]);
                assert(run.submit(sequence[ply])==(ply+1==sequence.size()?Attempt::Complete:Attempt::Correct));
                assert(run.ply()==ply+1);
            }
            assert(run.completed()&&run.hint().from<0&&run.game().position.fen()==prior);
            assert(run.submit(sequence.back())==Attempt::Illegal);
            assert(!run.start(-1)&&run.completed());
            assert(run.start(i,branch)&&!run.completed()&&run.ply()==0);
        }
        assert(line(i,0).uci!=line(i,1).uci);
    }
    chess::Game out;assert(!positionAt(Count,0,0,out));
}
void persistence(const std::string &root) {
    using namespace openings;
    const auto file=root+"/openings.dat";Store store(file);
    assert(!store.load()&&!store.corrupt&&!store.exists);
    store.progress={5,7,9};assert(store.save());const auto good=read(file);
    Store restored(file);assert(restored.load());
    assert(restored.progress.favorites==5&&restored.progress.studied==7&&restored.progress.lastOpening==9);
    for(size_t size=0;size<good.size();++size) {
        write(file,good.substr(0,size));assert(!restored.load()&&restored.corrupt&&restored.exists);
        assert(restored.progress.favorites==5&&restored.progress.studied==7&&restored.progress.lastOpening==9);
    }
    for(size_t i=0;i<good.size();++i) {
        auto bad=good;bad[i]^=1;write(file,bad);assert(!restored.load()&&restored.corrupt);
    }
    write(file,std::string(10000,'A'));assert(!restored.load()&&restored.corrupt);
    write(file,good);restored.progress.favorites=1u<<Count;assert(!restored.save());
    restored.progress={0,1u<<Count,0};assert(!restored.save());
    restored.progress={0,0,-1};assert(!restored.save());
    restored.progress={0,0,Count};assert(!restored.save());
    restored.progress={2,4,3};assert(restored.save());
    write(file,"bad");assert(restored.load()&&restored.recovered&&restored.progress.favorites==5);
    write(file+".bak","bad");assert(!restored.load()&&restored.corrupt);
    int index=0;
    for(auto fault:{storage::SaveFault::Write,storage::SaveFault::Flush,storage::SaveFault::Sync,
                    storage::SaveFault::Close,storage::SaveFault::Backup,storage::SaveFault::Commit}) {
        const auto target=root+"/fault"+std::to_string(index++);Store writer(target);
        writer.progress={1,1,1};assert(writer.save());writer.progress={3,3,2};assert(!writer.save(fault));
        Store loaded(target);assert(loaded.load()&&loaded.progress.favorites==1&&loaded.progress.lastOpening==1);
        assert(writer.save());assert(loaded.load()&&loaded.progress.favorites==3&&loaded.progress.lastOpening==2);
    }
    Store one(root+"/one"),two(root+"/two");one.progress={1,2,3};assert(one.save()&&two.save());
    assert(one.load()&&two.load()&&one.progress.studied==2&&two.progress.studied==0);
}
}
int main(){lines();char dir[]="/tmp/chess-openings-XXXXXX";assert(mkdtemp(dir));persistence(dir);fs::remove_all(dir);
std::cout<<"Openings: 20 legal lines, every replay prefix, two-color practice, mistakes, progress, corruption and six I/O faults passed\n";}
