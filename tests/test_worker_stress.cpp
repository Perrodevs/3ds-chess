#include "ai/worker.hpp"
#include "ai/hints.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
namespace {
bool consume(ai::Worker&w,const chess::Game&g,ai::Answer&a){
 auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
 while(w.running&&std::chrono::steady_clock::now()<deadline){
  if(w.poll(g,a))return true;
  std::this_thread::yield();
 }
 assert(!w.running);return false;
}
}
int main(){
 chess::Game g;ai::Worker w;ai::Answer a;
 for(int i=0;i<100;++i){assert(w.start(g,5,i+1,-1,5));w.stop();assert(!w.poll(g,a));}
 // Consuming a response for a different position or repetition history discards it.
 for(int mode=0;mode<3;++mode){
  assert(w.start(g,0,1,0));auto other=g;
  if(mode==0)assert(other.play({12,28,0}));
  if(mode==1)other.positions.push_back(other.position.key());
  if(mode==2)other.resign();
  a.move={3,3,0};assert(!consume(w,other,a));assert(a.move.from==3&&a.move.to==3);
 }
 assert(w.start(g,0,1,0));assert(consume(w,g,a));assert(a.move.from>=0);
 // Stop old task then start a new game: only the new snapshot may produce a result.
 chess::Game next=g;assert(next.play({12,28,0}));
 for(int i=0;i<50;++i){assert(w.start(g,5,i+1,-1,5));assert(w.start(next,0,1,0));assert(consume(w,next,a));
  auto legal=next.position.legal();bool valid=false;for(auto m:legal)valid|=m==a.move;assert(valid);}
 ai::Hints hints;
 for(int i=0;i<30;++i){assert(hints.request(g));hints.cancel();chess::Move m;assert(!hints.poll(next,m));}
 std::cout<<"Worker stress: 100 cancellations, stale position/history/terminal rejection, 50 replacements, 30 hint cancellations passed\n";
}
