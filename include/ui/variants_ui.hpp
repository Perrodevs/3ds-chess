#pragma once
#include "variants/variants.hpp"
#include "graphics/renderer.hpp"
namespace ui {
struct Input;
class VariantsUI {
    enum class Screen { Menu, Game, Pause, Review, Promotion, ConfirmNew, ConfirmResign };
    Screen screen=Screen::Menu, reviewReturn=Screen::Pause;
    variants::Store store;
    variants::State state;
    variants::Game replay;
    variants::Worker worker;
    bool ready=false;
    int kind=0,seed=518,human=1,level=0,focus=0,cursor=12,selected=-1,ply=0;
    chess::Move promotion;
    std::vector<std::string> notation;
    std::string notice;
    int noticeFrames=0;
    void start();
    bool persist(variants::State);
    void play(chess::Move);
    void action();
    void seek(int);
    int file(int sq)const;
    int row(int sq)const;
  public:
    explicit VariantsUI(const std::string &directory);
    bool update(const Input &);
    void draw(graphics::Renderer &,int skin=0);
    void suspend(){worker.stop();}
    bool save(){return ready&&store.save(state);}
    const variants::State &data()const{return state;}
};
}
