#pragma once
#include "composer/composer.hpp"
#include "graphics/renderer.hpp"
#ifdef __3DS__
#include <3ds.h>
#else
#include <thread>
#endif
#include <atomic>
namespace ui {
struct Input;
// Isolated local workshop. No economy/session references and no competitive rewards.
class ComposerUI {
  public:
    enum class Screen { Home, Editor, EditorMenu, Settings, PlayMenu, Positions, PositionMenu, Puzzles,
        PuzzleMenu, Goal, Record, RecordMenu, PuzzlePlay, PositionPlay, Confirm,
        Proof, DraftResult, Promotion };
  private:
    composer::Store store;
    std::string directory, notice, name = "Posicion", requestPrompt;
    bool ready = false, request = false;
    enum class TextAction { None, Position, Puzzle, RenamePosition, RenamePuzzle } textAction = TextAction::None;
    Screen current = Screen::Home, returnScreen = Screen::Home, confirmReturn = Screen::Home;
    enum class Confirm { None, OverwritePosition, OverwritePuzzle, DeletePosition, DeletePuzzle } confirm = Confirm::None;
    chess::Position draft = chess::Position::initial();
    chess::Game game;
    composer::Puzzle puzzle;
    composer::Run run;
    int focus=0,cursor=12,palette=1,selected=-1,index=0,goal=0,promotion=0;
    uint64_t editingPosition=0,editingPuzzle=0;
    std::vector<chess::Move> promotionMoves;
    Screen promotionReturn=Screen::PositionPlay;
    enum class Job { None, Validate, Start, Submit, Hint } job=Job::None;
#ifdef __3DS__
    Thread thread=nullptr;
#else
    std::thread thread;
#endif
    std::atomic<bool> cancel{false},done{false};
    bool working=false;
    composer::Puzzle jobPuzzle;
    composer::Run jobRun;
    chess::Move jobMove,answerHint;
    composer::Attempt answerAttempt=composer::Attempt::Illegal;
    bool answerStarted=false;
    Screen jobReturn=Screen::Home;
    static void entry(void *);
    bool startJob(Job,Screen);
    void stopJob();
    void pollJob();
    void go(Screen);
    std::vector<std::string> labels()const;
    int rowHeight()const;
    void activate();
    void back();
    void requestName(TextAction,const std::string &,const std::string &);
    bool persist();
    void storePosition();
    void storePuzzle();
    void recordStart();
    void moveOnBoard(chess::Move);
    void chooseSquare();
    void undoRecording();
    void drawBoard(graphics::Renderer &,int);
  public:
    explicit ComposerUI(const std::string &);
    ~ComposerUI();
    ComposerUI(const ComposerUI&)=delete;
    ComposerUI&operator=(const ComposerUI&)=delete;
    bool update(const Input &);
    void draw(graphics::Renderer &,int skin=0);
    bool save();
    void suspend();
    bool textRequested()const{return request;}
    std::string textPrompt()const{return requestPrompt;}
    std::string textValue()const{return name;}
    void acceptText(const std::string &,bool confirmed);
    Screen screen()const{return current;}
    const chess::Position &position()const;
    const composer::Collection &collection()const{return store.data;}
    bool busy()const{return working;}
    int cursorSquare()const{return cursor;}
};
}
