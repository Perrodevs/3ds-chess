#include "diagnostics/log.hpp"
#include "localization/localization.hpp"
#include "ui/composer_ui.hpp"
#include "ui/app.hpp"
#include <algorithm>
#include <limits>
#ifndef __3DS__
#include <system_error>
#endif
namespace ui {
namespace {
constexpr auto bg=graphics::rgb(20,25,36),panel=graphics::rgb(34,43,57),ink=graphics::rgb(236,227,192),gold=graphics::rgb(235,185,78),dim=graphics::rgb(142,158,169),blue=graphics::rgb(90,180,211);
int pieceFor(int n){return n==0?0:n<=6?n:-(n-6);}
const char *pieceName(int n){static const char*names[]={"Borrar","Peon blanco","Caballo blanco","Alfil blanco","Torre blanca","Dama blanca","Rey blanco","Peon negro","Caballo negro","Alfil negro","Torre negra","Dama negra","Rey negro"};return names[std::clamp(n,0,12)];}
bool isBoard(ComposerUI::Screen s){using S=ComposerUI::Screen;return s==S::Editor||s==S::Record||s==S::PuzzlePlay||s==S::PositionPlay;}
std::string status(composer::Validation v){return v==composer::Validation::Valid?"VALIDADO":v==composer::Validation::Pending?"PENDIENTE DE VALIDACION":"SOLUCION INVALIDA";}
}
ComposerUI::ComposerUI(const std::string&p):store(p+"/composer.dat"),directory(p){
    ready=store.load()||!store.exists;
    if(!ready)notice="Datos danados: se conservan sin reemplazar";
    else if(store.recovered)notice="Coleccion recuperada desde respaldo";
}
ComposerUI::~ComposerUI(){stopJob();}
void ComposerUI::go(Screen s){current=s;focus=0;selected=-1;}
const chess::Position&ComposerUI::position()const{
    const Screen visible=current==Screen::Proof?jobReturn:current==Screen::PlayMenu?returnScreen:current;
    if(visible==Screen::Record||visible==Screen::RecordMenu||visible==Screen::PuzzlePlay||visible==Screen::PositionPlay||visible==Screen::Promotion)return game.position;
    return draft;
}
bool ComposerUI::persist(){if(!ready){notice="Coleccion danada: guardado bloqueado";return false;}if(!store.save()){diagnostics::journal().add(diagnostics::Level::Error,"COMPOSER_SAVE_FAILED","collection");notice="No se guardo: revisa la tarjeta SD";return false;}return true;}
bool ComposerUI::save(){return !ready?false:!store.exists&&store.data.positions.empty()&&store.data.puzzles.empty()?true:persist();}
void ComposerUI::suspend(){stopJob();if(current==Screen::Proof)go(jobReturn);save();}
void ComposerUI::entry(void*arg){auto*w=static_cast<ComposerUI*>(arg);constexpr uint32_t budget=50000;
    if(w->job==Job::Validate)composer::validatePuzzle(w->jobPuzzle,budget,&w->cancel);
    else if(w->job==Job::Start)w->answerStarted=w->jobRun.start(w->jobPuzzle,budget,&w->cancel);
    else if(w->job==Job::Submit)w->answerAttempt=w->jobRun.submit(w->jobMove,budget,&w->cancel);
    else if(w->job==Job::Hint)w->answerHint=w->jobRun.hint(budget,&w->cancel);
    w->done.store(true,std::memory_order_release);
}
bool ComposerUI::startJob(Job next,Screen origin){
    stopJob();job=next;jobReturn=origin;cancel=false;done=false;
    answerHint={};answerStarted=false;answerAttempt=composer::Attempt::Illegal;
    if(next==Job::Validate||next==Job::Start)jobPuzzle=puzzle;
    if(next==Job::Submit||next==Job::Hint)jobRun=run;
#ifdef __3DS__
    // At most five plies of proof, fixed 256-move lists: no recursive engine search.
    thread=threadCreate(entry,this,64*1024,0x31,0,false);working=thread!=nullptr;
#else
    try{thread=std::thread(entry,this);working=true;}catch(const std::system_error&){working=false;}
#endif
    if(!working){notice="No se pudo iniciar la comprobacion";return false;}
    go(Screen::Proof);return true;
}
void ComposerUI::stopJob(){cancel.store(true,std::memory_order_relaxed);
#ifdef __3DS__
    if(thread){threadJoin(thread,U64_MAX);threadFree(thread);thread=nullptr;}
#else
    if(thread.joinable())thread.join();
#endif
    working=false;
}
void ComposerUI::pollJob(){
    if(!working||!done.load(std::memory_order_acquire))return;
    const Job completed=job;stopJob();
    if(completed==Job::Validate){puzzle=jobPuzzle;go(Screen::DraftResult);notice=status(puzzle.validation);}
    else if(completed==Job::Start){
        if(!answerStarted){go(Screen::PuzzleMenu);notice="No se pudo probar: pendiente o invalido";return;}
        auto before=store.data;
        bool counted=false;
        for(auto&p:store.data.puzzles)if(p.id==puzzle.id&&p.attempts<std::numeric_limits<uint32_t>::max()){++p.attempts;counted=true;}
        // A playable run must own a newly committed attempt. Otherwise its
        // completion could be credited to an earlier unfinished attempt.
        if(!counted||!persist()){
            store.data=std::move(before);run=composer::Run{};game=chess::Game{};
            go(Screen::PuzzleMenu);notice="No se inicio: intento no guardado";return;
        }
        run=jobRun;game=run.game();cursor=game.position.side==1?12:52;go(Screen::PuzzlePlay);
        notice="Practica ambos bandos. X: pista. Sin monedas.";
    }else if(completed==Job::Hint){go(jobReturn);if(answerHint.from>=0){cursor=answerHint.from;selected=answerHint.from;notice="Pista: "+answerHint.uci();}else notice="Sin pista demostrada con este presupuesto";}
    else if(completed==Job::Submit){
        go(Screen::PuzzlePlay);
        if(answerAttempt==composer::Attempt::Correct||answerAttempt==composer::Attempt::Complete){run=jobRun;game=run.game();cursor=jobMove.to;notice=answerAttempt==composer::Attempt::Complete?"Puzzle completado. Sin monedas.":"Correcto. Juega ahora el otro color.";
            if(answerAttempt==composer::Attempt::Complete){auto before=store.data;for(auto&p:store.data.puzzles)if(p.id==puzzle.id&&p.completed<p.attempts)++p.completed;if(!persist())store.data=std::move(before);}
        }else notice=answerAttempt==composer::Attempt::Pending?"No demostrado: pide pista o reintenta":"No cumple el objetivo; intenta otra jugada";
    }
}
void ComposerUI::requestName(TextAction a,const std::string&prompt,const std::string&value){textAction=a;requestPrompt=prompt;name=value;request=true;}
void ComposerUI::acceptText(const std::string&text,bool confirmed){
    if(!request)return;
    request=false;
    if(!confirmed){textAction=TextAction::None;return;}
    if(text.empty()||text.size()>24||std::any_of(text.begin(),text.end(),[](unsigned char c){return c<32||c>126;})){notice="Usa un nombre ASCII de 1 a 24 caracteres";return;}
    name=text;
    if(textAction==TextAction::Position){if(editingPosition){confirm=Confirm::OverwritePosition;confirmReturn=current;go(Screen::Confirm);}else storePosition();}
    else if(textAction==TextAction::Puzzle){puzzle.name=name;if(editingPuzzle){confirm=Confirm::OverwritePuzzle;confirmReturn=current;go(Screen::Confirm);}else storePuzzle();}
    else if(textAction==TextAction::RenamePosition){auto old=store.data;auto&p=store.data.positions[size_t(index)];p.name=name;if(!persist())store.data=std::move(old);}
    else if(textAction==TextAction::RenamePuzzle){auto old=store.data;auto&p=store.data.puzzles[size_t(index)];p.name=name;if(!persist())store.data=std::move(old);}
    textAction=TextAction::None;
}
void ComposerUI::storePosition(){auto old=store.data;if(!ready||!store.putPosition(name,draft,editingPosition)||!persist()){store.data=std::move(old);notice="Posicion no guardada: invalida, limite o SD";return;}if(!editingPosition)editingPosition=store.data.positions.back().id;notice="Posicion guardada";go(Screen::Editor);}
void ComposerUI::storePuzzle(){auto old=store.data;if(!ready||puzzle.validation==composer::Validation::Invalid||!store.putPuzzle(puzzle,editingPuzzle)||!persist()){store.data=std::move(old);notice="Puzzle no guardado: invalido, limite o SD";return;}if(!editingPuzzle)editingPuzzle=store.data.puzzles.back().id;notice="Puzzle guardado: "+status(puzzle.validation);go(Screen::Puzzles);const auto saved=std::find_if(store.data.puzzles.begin(),store.data.puzzles.end(),[&](const composer::Puzzle&p){return p.id==editingPuzzle;});index=int(saved-store.data.puzzles.begin());}
void ComposerUI::recordStart(){auto error=composer::validate(draft);if(!error.empty()){notice=error;return;}if(!composer::makeGame(draft,game)||game.result!=chess::Result::Playing){notice="La posicion ya esta terminada";return;}if(puzzle.name.empty())puzzle.name="Puzzle";puzzle.fen=draft.fen();puzzle.goal=composer::Goal(goal);puzzle.solution.clear();puzzle.validation=composer::Validation::Pending;cursor=draft.side==1?12:52;go(Screen::Record);notice="Graba ambos bandos. START: comprobar solucion.";}
void ComposerUI::undoRecording(){if(game.moves.empty())return;const auto moves=game.moves;chess::Position p;if(!composer::parseFen(puzzle.fen,p)||!composer::makeGame(p,game))return;for(size_t i=0;i+1<moves.size();++i)game.play(moves[i]);puzzle.solution=game.moves;selected=-1;}
void ComposerUI::moveOnBoard(chess::Move m){
    if(current==Screen::PuzzlePlay){if(run.completed()){notice="Completado. START para repetir.";return;}jobMove=m;startJob(Job::Submit,current);return;}
    if(current==Screen::Record&&game.moves.size()>=5){notice="Maximo cinco medias jugadas. START: comprobar";return;}
    if(game.play(m)){cursor=m.to;selected=-1;if(current==Screen::Record)puzzle.solution=game.moves;notice=game.result==chess::Result::Playing?"Jugada legal":game.reason;}
    else notice="Movimiento ilegal";
}
void ComposerUI::chooseSquare(){
    if(current==Screen::Editor){draft.board[size_t(cursor)]=int8_t(pieceFor(palette));notice=std::string(pieceName(palette))+" en "+chess::squareName(cursor);return;}
    const auto&p=game.position;if(game.result!=chess::Result::Playing){notice=game.reason;return;}
    if(selected<0){if(p.board[size_t(cursor)]*p.side>0)selected=cursor;return;}
    const auto legal=p.legal();promotionMoves.clear();for(auto m:legal)if(m.from==selected&&m.to==cursor)promotionMoves.push_back(m);
    if(promotionMoves.size()>1){promotionReturn=current;promotion=0;go(Screen::Promotion);return;}
    if(promotionMoves.size()==1){moveOnBoard(promotionMoves[0]);return;}
    selected=p.board[size_t(cursor)]*p.side>0?cursor:-1;notice="Elige un destino legal";
}
std::vector<std::string>ComposerUI::labels()const{
    switch(current){
    case Screen::Home:return {"Editor de posiciones","Mis posiciones ("+std::to_string(store.data.positions.size())+"/30)","Mis puzzles ("+std::to_string(store.data.puzzles.size())+"/30)","Volver a modos"};
    case Screen::EditorMenu:return {"Guardar posicion...","Turno, enroque y al paso",editingPuzzle?"Editar solucion del puzzle":"Crear puzzle desde aqui","Importar import.fen","Exportar export.fen","Jugar local sin monedas","Volver al editor","Volver al taller"};
    case Screen::PlayMenu:return returnScreen==Screen::PuzzlePlay?std::vector<std::string>{"Continuar puzzle","Repetir desde inicio","Pista","Volver a mis puzzles"}:std::vector<std::string>{"Continuar partida local","Reiniciar desde posicion","Volver al editor"};
    case Screen::Settings:return {std::string("Turno: ")+(draft.side==1?"blancas":"negras"),std::string("Enroque blanco corto: ")+(draft.castling&1?"si":"no"),std::string("Enroque blanco largo: ")+(draft.castling&2?"si":"no"),std::string("Enroque negro corto: ")+(draft.castling&4?"si":"no"),std::string("Enroque negro largo: ")+(draft.castling&8?"si":"no"),"Al paso: "+(draft.ep<0?std::string("ninguna"):chess::squareName(draft.ep)),"Limpiar tablero","Restaurar posicion inicial","Volver al editor"};
    case Screen::Positions:return {"< Posicion anterior","Posicion siguiente >","Abrir posicion","Volver"};
    case Screen::PositionMenu:return {"Editar posicion","Jugar local sin monedas","Exportar FEN","Renombrar","Eliminar...","Volver"};
    case Screen::Puzzles:return {"< Puzzle anterior","Puzzle siguiente >","Abrir puzzle","Crear desde editor","Volver"};
    case Screen::PuzzleMenu:return {"Jugar puzzle","Editar posicion y solucion","Renombrar","Eliminar...","Volver"};
    case Screen::Goal:return {std::string("Objetivo: ")+composer::goalName(composer::Goal(goal)),"Grabar solucion con tablero","Volver al editor"};
    case Screen::RecordMenu:return {"Comprobar solucion","Deshacer ultima jugada","Grabar de nuevo","Volver al tablero","Volver al editor"};
    case Screen::DraftResult:return {"Guardar puzzle con nombre","Revisar / corregir solucion","Volver al editor"};
    case Screen::Confirm:return {"Si, confirmar","Cancelar"};
    case Screen::Promotion:return {"Dama","Torre","Alfil","Caballo","Cancelar"};
    default:return {};
    }
}
int ComposerUI::rowHeight()const{auto n=labels().size();return n?std::min(24,int(168/n)):24;}
void ComposerUI::activate(){
    if(current==Screen::Home){index=0;if(focus==0)go(Screen::Editor);else if(focus==1)go(Screen::Positions);else if(focus==2)go(Screen::Puzzles);return;}
    if(current==Screen::EditorMenu){
        if(focus==0){const auto error=composer::validate(draft);if(!error.empty()){notice=error;return;}requestName(TextAction::Position,"Nombre de posicion",name);}
        else if(focus==1)go(Screen::Settings);
        else if(focus==2)go(Screen::Goal);
        else if(focus==3){chess::Position p;if(composer::importFen(directory+"/import.fen",p)){draft=p;editingPosition=editingPuzzle=0;notice="FEN importado";go(Screen::Editor);}else notice="import.fen ausente o invalido";}
        else if(focus==4)notice=composer::exportFen(directory+"/export.fen",draft)?"FEN exportado a export.fen":"No se exporto: valida posicion o revisa SD";
        else if(focus==5){if(composer::makeGame(draft,game)){cursor=draft.side==1?12:52;go(Screen::PositionPlay);notice="Partida local de practica, sin monedas";}else notice=composer::validate(draft);}
        else if(focus==6)go(Screen::Editor);else go(Screen::Home);
    }else if(current==Screen::Settings){
        if(focus==0){draft.side=-draft.side;draft.ep=-1;}
        else if(focus<=4)draft.castling^=1<<(focus-1);
        else if(focus==5){if(draft.ep<0)draft.ep=(draft.side==1?40:16);else if(draft.ep%8==7)draft.ep=-1;else ++draft.ep;draft.halfmove=0;}
        else if(focus==6){draft=chess::Position{};draft.board.fill(0);draft.castling=0;draft.ep=-1;editingPosition=editingPuzzle=0;}
        else if(focus==7){draft=chess::Position::initial();editingPosition=editingPuzzle=0;}
        else go(Screen::Editor);
    }else if(current==Screen::Positions){
        const int count=int(store.data.positions.size());
        if(focus==0&&count)index=(index+count-1)%count;
        else if(focus==1&&count)index=(index+1)%count;
        else if(focus==2){if(count)go(Screen::PositionMenu);else notice="Aun no hay posiciones guardadas";}
        else if(focus==3)go(Screen::Home);
    }else if(current==Screen::PositionMenu){
        if(index<0||size_t(index)>=store.data.positions.size()){go(Screen::Positions);return;}
        auto&p=store.data.positions[size_t(index)];
        if(focus<=2){chess::Position q;if(!composer::parseFen(p.fen,q)){notice="Registro invalido";return;}
            if(focus==0){draft=q;name=p.name;editingPosition=p.id;editingPuzzle=0;go(Screen::Editor);}
            else if(focus==1){draft=q;if(composer::makeGame(q,game)){cursor=q.side==1?12:52;go(Screen::PositionPlay);notice="Local: ambos colores. Sin monedas.";}}
            else notice=composer::exportFen(directory+"/export.fen",q)?"FEN exportado a export.fen":"Error al exportar";
        }else if(focus==3)requestName(TextAction::RenamePosition,"Renombrar posicion",p.name);
        else if(focus==4){confirm=Confirm::DeletePosition;confirmReturn=current;go(Screen::Confirm);}
        else go(Screen::Positions);
    }else if(current==Screen::Puzzles){
        const int count=int(store.data.puzzles.size());
        if(focus==0&&count)index=(index+count-1)%count;
        else if(focus==1&&count)index=(index+1)%count;
        else if(focus==2){if(count)go(Screen::PuzzleMenu);else notice="Crea uno desde una posicion valida";}
        else if(focus==3){editingPuzzle=0;puzzle=composer::Puzzle{};go(Screen::Editor);}
        else if(focus==4)go(Screen::Home);
    }else if(current==Screen::PuzzleMenu){
        if(index<0||size_t(index)>=store.data.puzzles.size()){go(Screen::Puzzles);return;}
        const auto p=store.data.puzzles[size_t(index)];
        if(focus==0){puzzle=p;if(p.validation!=composer::Validation::Valid){notice="Pendiente: edita y comprueba su solucion";return;}startJob(Job::Start,current);}
        else if(focus==1){puzzle=p;editingPuzzle=p.id;editingPosition=0;goal=int(p.goal);name=p.name;composer::parseFen(p.fen,draft);go(Screen::Editor);notice="Edita y usa START > Editar solucion";}
        else if(focus==2)requestName(TextAction::RenamePuzzle,"Renombrar puzzle",p.name);
        else if(focus==3){confirm=Confirm::DeletePuzzle;confirmReturn=current;go(Screen::Confirm);}
        else go(Screen::Puzzles);
    }else if(current==Screen::Goal){if(focus==0)goal=(goal+1)%5;else if(focus==1)recordStart();else go(Screen::Editor);}
    else if(current==Screen::RecordMenu){
        if(focus==0){puzzle.solution=game.moves;if(puzzle.solution.empty()){notice="Graba al menos una jugada legal";return;}startJob(Job::Validate,current);}
        else if(focus==1){undoRecording();go(Screen::Record);}
        else if(focus==2)recordStart();else if(focus==3)go(Screen::Record);else go(Screen::Editor);
    }else if(current==Screen::DraftResult){
        if(focus==0){if(puzzle.validation==composer::Validation::Invalid){notice="Corrige la solucion antes de guardarla";return;}requestName(TextAction::Puzzle,"Nombre de puzzle",puzzle.name);}
        else if(focus==1)go(Screen::Record);else go(Screen::Editor);
    }else if(current==Screen::Confirm){
        if(focus!=0){go(confirmReturn);return;}
        if(confirm==Confirm::OverwritePosition)storePosition();
        else if(confirm==Confirm::OverwritePuzzle)storePuzzle();
        else{
            auto old=store.data;bool ok=false;
            if(confirm==Confirm::DeletePosition&&index>=0&&size_t(index)<store.data.positions.size()){auto id=store.data.positions[size_t(index)].id;ok=store.erasePosition(id);if(editingPosition==id)editingPosition=0;}
            else if(confirm==Confirm::DeletePuzzle&&index>=0&&size_t(index)<store.data.puzzles.size()){auto id=store.data.puzzles[size_t(index)].id;ok=store.erasePuzzle(id);if(editingPuzzle==id)editingPuzzle=0;}
            if(!ok||!persist()){store.data=std::move(old);notice="No se elimino el registro";}
            else notice="Registro eliminado";
            index=0;go(confirm==Confirm::DeletePosition?Screen::Positions:Screen::Puzzles);
        }
        confirm=Confirm::None;
    }else if(current==Screen::Promotion){
        if(focus==4){go(promotionReturn);return;}
        static const int pieces[]={chess::Queen,chess::Rook,chess::Bishop,chess::Knight};
        for(auto m:promotionMoves)if(m.promotion==pieces[focus]){go(promotionReturn);moveOnBoard(m);break;}
    }else if(current==Screen::PlayMenu){
        if(focus==0)go(returnScreen);
        else if(focus==1){if(returnScreen==Screen::PuzzlePlay)startJob(Job::Start,Screen::PuzzleMenu);else if(composer::makeGame(draft,game))go(Screen::PositionPlay);}
        else if(focus==2&&returnScreen==Screen::PuzzlePlay)startJob(Job::Hint,Screen::PuzzlePlay);
        else go(returnScreen==Screen::PuzzlePlay?Screen::Puzzles:Screen::Editor);
    }
}
void ComposerUI::back(){
    if(current==Screen::Proof){stopJob();go(jobReturn);notice="Comprobacion cancelada";}
    else if(current==Screen::Editor)go(Screen::Home);
    else if(current==Screen::EditorMenu||current==Screen::Settings||current==Screen::Goal)go(Screen::Editor);
    else if(current==Screen::Positions||current==Screen::Puzzles)go(Screen::Home);
    else if(current==Screen::PositionMenu)go(Screen::Positions);
    else if(current==Screen::PuzzleMenu)go(Screen::Puzzles);
    else if(current==Screen::Record){undoRecording();notice="START: comprobar o volver al editor";}
    else if(current==Screen::RecordMenu||current==Screen::DraftResult)go(Screen::Record);
    else if(current==Screen::PositionPlay||current==Screen::PuzzlePlay){selected=-1;returnScreen=current;go(Screen::PlayMenu);}
    else if(current==Screen::PlayMenu)go(returnScreen);
    else if(current==Screen::Confirm)go(confirmReturn);
    else if(current==Screen::Promotion)go(promotionReturn);
}
bool ComposerUI::update(const Input&in){
    pollJob();if(request)return true;
    if(current==Screen::Proof){if(in.keys&B)back();return true;}
    if(current==Screen::Home&&((in.keys&B)||((in.keys&A)&&focus==3)))return save(),false;
    if(in.keys&B){back();return true;}
    if(isBoard(current)){
        if(in.keys&Start){if(current==Screen::Editor)go(Screen::EditorMenu);else if(current==Screen::Record)go(Screen::RecordMenu);else{returnScreen=current;go(Screen::PlayMenu);}return true;}
        if((in.keys&Up)&&cursor<56)cursor+=8;
        if((in.keys&Down)&&cursor>=8)cursor-=8;
        if(in.keys&Left)cursor=cursor%8?cursor-1:cursor;
        if(in.keys&Right)cursor=cursor%8<7?cursor+1:cursor;
        if(in.keys&Hint){if(current==Screen::Editor)palette=(palette+1)%13;else if(current==Screen::PuzzlePlay)startJob(Job::Hint,current);else if(current==Screen::Record)undoRecording();}
        if(in.keys&A)chooseSquare();
        if(in.touch){
            if(current==Screen::Editor&&in.y>=40&&in.y<96&&in.x>=12&&in.x<306){const int n=(in.y-40)/28*7+(in.x-12)/42;if(n<13)palette=n;}
            else if(in.x>=12&&in.x<108&&in.y>=110&&in.y<206){cursor=(7-(in.y-110)/12)*8+(in.x-12)/12;}
            else if(in.x>=120&&in.x<308){if(in.y>=110&&in.y<134)chooseSquare();else if(in.y>=140&&in.y<164){if(current==Screen::Editor)go(Screen::EditorMenu);else if(current==Screen::Record)go(Screen::RecordMenu);else{returnScreen=current;go(Screen::PlayMenu);}}else if(in.y>=170&&in.y<194)back();}
        }
        return true;
    }
    const auto menu=labels();if(menu.empty())return true;
    if(in.keys&Up)focus=(focus+int(menu.size())-1)%int(menu.size());
    if(in.keys&Down)focus=(focus+1)%int(menu.size());
    if(current==Screen::Goal&&(in.keys&(Left|Right)))goal=(goal+((in.keys&Left)?4:1))%5;
    if(current==Screen::Settings&&(in.keys&(Left|Right))&&focus==5){if(in.keys&Right)activate();else{if(draft.ep<0)draft.ep=(draft.side==1?47:23);else if(draft.ep%8==0)draft.ep=-1;else --draft.ep;draft.halfmove=0;}}
    if(in.keys&A)activate();
    if(in.touch&&in.x>=12&&in.x<308&&in.y>=46){int row=(in.y-46)/rowHeight();if(row>=0&&row<int(menu.size())&&in.y<46+rowHeight()*int(menu.size())){focus=row;if(current==Screen::Home&&focus==3)return save(),false;activate();}}
    return true;
}
void ComposerUI::drawBoard(graphics::Renderer&r,int skin){
    const auto&p=position();
    r.screenBegin(0,bg);r.border(10,6,228,228,dim);
    for(int sq=0;sq<64;++sq){int x=12+sq%8*28,y=8+(7-sq/8)*28;r.rect(x,y,28,28,((sq/8+sq%8)&1)?graphics::rgb(179,169,130):graphics::rgb(88,86,96));r.piece(p.board[size_t(sq)],x+2,y+2,skin);if(sq==cursor)r.border(x+1,y+1,26,26,gold,2);if(sq==selected)r.border(x+3,y+3,22,22,blue,2);}
    r.text(248,12,"TALLER",gold,2,11);r.text(248,42,p.side==1?"Turno blanco":"Turno negro",ink,1,23);
    r.text(248,62,"LOCAL / SIN PREMIOS",dim,1,23);
    const auto err=localization::translate(composer::validate(p));
    if(!err.empty()){r.text(248,92,"BORRADOR",gold,1,23);r.text(248,110,err,dim,1,23);if(err.size()>23)r.text(248,122,err.substr(23),dim,1,23);}
    else r.text(248,92,"POSICION VALIDA",blue,1,23);
    if(current==Screen::Record||current==Screen::RecordMenu){r.text(248,144,"Solucion: "+std::to_string(game.moves.size())+"/5",ink,1,23);r.text(248,166,composer::goalName(puzzle.goal),dim,1,23);}
    r.text(248,205,"CRUCETA + A",ink,1,23);r.text(248,221,"START: OPCIONES",dim,1,23);
}
void ComposerUI::draw(graphics::Renderer&r,int skin){
    drawBoard(r,skin);r.screenBegin(1,bg);
    r.text(12,10,"TALLER DE AJEDREZ",gold,1,49);r.text(12,25,notice,dim,1,49);
    if(isBoard(current)){
        if(current==Screen::Editor){for(int n=0;n<13;++n){int x=12+n%7*42,y=40+n/7*28;r.rect(x,y,40,26,n==palette?graphics::rgb(93,79,43):panel);if(n)r.piece(pieceFor(n),x+7,y+1,skin);else r.text(x+5,y+8,"BOR",ink); }r.text(12,98,pieceName(palette),ink,1,49);}
        else{r.text(12,48,current==Screen::Record?"GRABANDO SOLUCION":current==Screen::PuzzlePlay?composer::goalName(puzzle.goal):"PARTIDA LOCAL DE PRACTICA",ink,1,49);r.text(12,66,"A: origen/destino. X: pista o deshacer",dim,1,49);r.text(12,83,"B: opciones / deshacer al grabar",dim,1,49);}
        for(int sq=0;sq<64;++sq){int x=12+sq%8*12,y=110+(7-sq/8)*12;r.rect(x,y,12,12,((sq/8+sq%8)&1)?graphics::rgb(126,121,99):graphics::rgb(66,69,82));if(position().board[size_t(sq)])r.rect(x+4,y+4,4,4,position().board[size_t(sq)]>0?gold:blue);if(sq==cursor)r.border(x,y,12,12,ink);}
        const char*buttons[]={current==Screen::Editor?"Colocar pieza":"Elegir casilla","Opciones (START)",current==Screen::Record?"Deshacer (B)":"Volver (B)"};for(int i=0;i<3;++i){int y=110+i*30;r.rect(120,y,188,24,panel);r.text(126,y+8,buttons[i],ink,1,29);}
        r.text(12,219,current==Screen::Editor?"X: pieza siguiente. Toca mini-tablero.":"Toca mini-tablero y luego Elegir casilla.",dim,1,49);return;
    }
    if(current==Screen::Proof){r.text(12,70,"COMPROBANDO SIN BLOQUEAR CONTROLES",ink,1,49);r.text(12,96,"Busqueda limitada a 50000 nodos.",dim,1,49);r.text(12,122,"B: cancelar. No modifica la partida normal.",dim,1,49);return;}
    const auto menu=labels();const int h=rowHeight();for(size_t i=0;i<menu.size();++i){int y=46+int(i)*h;r.rect(12,y,296,h-2,i==size_t(focus)?graphics::rgb(70,65,50):panel);if(i==size_t(focus))r.border(12,y,296,h-2,gold);r.text(18,y+(h-8)/2,menu[i],ink,1,47);}
    int infoY=46+int(menu.size())*h+4;
    if(current==Screen::Positions&&!store.data.positions.empty()){index=std::clamp(index,0,int(store.data.positions.size())-1);const auto&p=store.data.positions[size_t(index)];r.rawText(12,infoY,std::to_string(index+1)+"/"+std::to_string(store.data.positions.size())+" "+p.name,gold,1,49);r.rawText(12,infoY+14,p.fen,dim,1,49);}
    else if((current==Screen::Puzzles||current==Screen::PuzzleMenu)&&!store.data.puzzles.empty()){index=std::clamp(index,0,int(store.data.puzzles.size())-1);const auto&p=store.data.puzzles[size_t(index)];r.rawText(12,infoY,p.name+" / "+localization::translate(status(p.validation)),gold,1,49);r.text(12,infoY+14,"Intentos "+std::to_string(p.attempts)+" / completados "+std::to_string(p.completed),dim,1,49);}
    else if(current==Screen::Goal)r.text(12,infoY,"Mate: prueba todas las defensas, no solo la linea.",dim,1,49);
    else if(current==Screen::DraftResult)r.text(12,infoY,status(puzzle.validation),gold,1,49);
    r.text(12,227,"A: ELEGIR   B: VOLVER",dim,1,49);
}
}
