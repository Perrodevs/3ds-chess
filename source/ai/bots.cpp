#include "localization/localization.hpp"
#include "ai/bots.hpp"
#include <array>
#include <string>
#include <algorithm>
namespace ai {
const Bot bots[botCount] = {
    {"Pollito", "Curioso y atrevido", "Bajo", "800-1200 orientativo", 1, 0, false, 0x706f6c6cu},
    {"Hamchow", "Calma y combinaciones", "Normal", "1300-1700 orientativo", 3, 1, false,
     0x68616d77u},
    {"Byte", "Calcula cada detalle", "Fuerte", "2200-2600: objetivo", 5, 2, false, 0x62797465u},
    {"Magnus", "Homenaje no oficial", "Fuerte", "Fuerza sin calibrar", 5, 3, true, 0x6d61676eu},
    {"Hikaru Nakamura", "Homenaje no oficial", "Fuerte", "Fuerza sin calibrar", 5, 4, true,
     0x68696b61u},
    {"Martin", "Bot de Chess.com", "Bajo", "250 de referencia", 0, 5, false, 0x6d617274u, 250,
     "Chess.com"},
    {"Wayne", "Bot de Chess.com", "Bajo", "250 de referencia", 0, 6, false, 0x7761796eu, 250,
     "Chess.com"},
    {"Fabian", "Bot de Chess.com", "Bajo", "250 de referencia", 0, 7, false, 0x66616269u, 250,
     "Chess.com"},
    {"Juan", "Bot de Chess.com", "Bajo", "400 de referencia", 1, 8, false, 0x6a75616eu, 400,
     "Chess.com"},
    {"Filip", "Bot de Chess.com", "Bajo", "400 de referencia", 1, 9, false, 0x66696c69u, 400,
     "Chess.com"},
    {"Elani", "Bot de Chess.com", "Bajo", "400 de referencia", 1, 10, false, 0x656c616eu, 400,
     "Chess.com"},
    {"Noel", "Bot de Chess.com", "Bajo", "550 de referencia", 1, 11, false, 0x6e6f656cu, 550,
     "Chess.com"},
    {"Oliver", "Bot de Chess.com", "Bajo", "550 de referencia", 1, 12, false, 0x6f6c6976u, 550,
     "Chess.com"},
    {"Danya", "Homenaje no oficial", "Fuerte", "2650 de referencia", 5, 13, true, 0xaa1e8366u, 2650,
     "Chess.com"},
    {"Kosteniuk", "Homenaje no oficial", "Fuerte", "2561 de referencia", 5, 14, true, 0x8b8b665eu,
     2561, "Chess.com"},
    {"Ding Liren", "Homenaje no oficial", "Fuerte", "2788 de referencia", 5, 15, true, 0x3e125165u,
     2788, "Chess.com"},
    {"Vishy", "Homenaje no oficial", "Fuerte", "2820 de referencia", 5, 16, true, 0xccbbb0feu, 2820,
     "Chess.com"},
    {"Bok", "Homenaje no oficial", "Fuerte", "2650 de referencia", 5, 17, true, 0xe4cb966fu, 2650,
     "Chess.com"},
    {"Lasker", "Homenaje no oficial", "Fuerte", "2640 de referencia", 5, 18, true, 0xa6ef7ec3u,
     2640, "Chess.com"},
    {"Abdusattorov", "Homenaje no oficial", "Fuerte", "2660 de referencia", 5, 19, true,
     0x2c8f86a9u, 2660, "Chess.com"},
    {"Giri", "Homenaje no oficial", "Fuerte", "2800 de referencia", 5, 20, true, 0x2581867cu, 2800,
     "Chess.com"},
    {"Anna Muzychuk", "Homenaje no oficial", "Fuerte", "2606 de referencia", 5, 21, true,
     0x41f147f1u, 2606, "Chess.com"},
    {"Irina Krush", "Homenaje no oficial", "Fuerte", "2502 de referencia", 5, 22, true, 0x60efae53u,
     2502, "Chess.com"},
    {"Paul Morphy", "Homenaje no oficial", "Fuerte", "2600 de referencia", 5, 23, true, 0xb85b5268u,
     2600, "Chess.com"},
    {"Vidit", "Homenaje no oficial", "Fuerte", "2730 de referencia", 5, 24, true, 0x765d6531u, 2730,
     "Chess.com"},
    {"Aronian", "Homenaje no oficial", "Fuerte", "2830 de referencia", 5, 25, true, 0xc7884669u,
     2830, "Chess.com"},
    {"Judit Polgar", "Homenaje no oficial", "Fuerte", "2735 de referencia", 5, 26, true,
     0xf807d708u, 2735, "Chess.com"},
    {"Hou Yifan", "Homenaje no oficial", "Fuerte", "2686 de referencia", 5, 27, true, 0x3435b688u,
     2686, "Chess.com"},
    {"Ian", "Homenaje no oficial", "Fuerte", "2795 de referencia", 5, 28, true, 0x006756a3u, 2795,
     "Chess.com"},
    {"Wesley So", "Homenaje no oficial", "Fuerte", "2820 de referencia", 5, 29, true, 0xd5f47bf2u,
     2820, "Chess.com"},
    {"Capablanca", "Homenaje no oficial", "Fuerte", "2725 de referencia", 5, 30, true, 0xc91eb6dfu,
     2725, "Chess.com"},
    {"Leon S. Kennedy", "Personaje invitado", "Fuerte", "Sin ELO oficial", 4, 31, true,
     0x38fcfe40u},
    {"Jill Valentine", "Personaje invitado", "Fuerte", "Sin ELO oficial", 4, 32, true, 0xe9d9271eu},
    {"Claire Redfield", "Personaje invitado", "Normal", "Sin ELO oficial", 3, 33, true,
     0xacd06296u},
    {"Chris Redfield", "Personaje invitado", "Fuerte", "Sin ELO oficial", 4, 34, true, 0x54fe68ddu},
    {"Joel", "Personaje invitado", "Normal", "Sin ELO oficial", 3, 35, true, 0x2de43b5fu},
    {"Ellie", "Personaje invitado", "Normal", "Sin ELO oficial", 2, 36, true, 0x8501ad12u},
    {"Steve", "Personaje invitado", "Bajo", "Sin ELO oficial", 1, 37, true, 0xbe66ec04u},
    {"Alex", "Personaje invitado", "Normal", "Sin ELO oficial", 2, 38, true, 0x65d14565u},
    {"Jensen Huang", "Fundador de NVIDIA", "Fuerte", "Sin ELO oficial", 4, 39, true, 0x4e564441u},
    {"Elon Musk", "Invitado tecnológico ficticio", "Fuerte", "Motor propio: sin Elo real", 4, 40, true, 0x53504143u},
};
const Bot &botAt(int id) {
    return bots[id >= 0 && id < botCount ? id : 0];
}
const char *botLine(int id, int expression) {
    static constexpr const char *lines[13][4] = {
        {"Vamos a jugar!", "Pio... estoy pensando.", "Eso estuvo bonito!", "Uy, se me escapo!"},
        {"Cada jugada cuenta.", "Buscando una idea...", "Buena combinacion!",
         "Toca reorganizarse."},
        {"Sistema listo.", "Calculando variantes...", "Plan completado.", "Recalculando el plan."},
        {"Una partida mas?", "Veamos esta posicion.", "Una buena posicion.", "Hay que resistir."},
        {"Vamos con energia!", "Buscando recursos...", "La tactica funciona!",
         "Aun quedan recursos."},
        {"Listo para jugar.", "Calculando...", "Buena jugada.", "Seguimos jugando."},
        {"Listo para jugar.", "Calculando...", "Buena jugada.", "Seguimos jugando."},
        {"Listo para jugar.", "Calculando...", "Buena jugada.", "Seguimos jugando."},
        {"Listo para jugar.", "Calculando...", "Buena jugada.", "Seguimos jugando."},
        {"Listo para jugar.", "Calculando...", "Buena jugada.", "Seguimos jugando."},
        {"Listo para jugar.", "Calculando...", "Buena jugada.", "Seguimos jugando."},
        {"Listo para jugar.", "Calculando...", "Buena jugada.", "Seguimos jugando."},
        {"Listo para jugar.", "Calculando...", "Buena jugada.", "Seguimos jugando."},
    };
    const int bot = id >= 0 && id < botCount ? id : 0;
    const int face = expression >= 0 && expression < 4 ? expression : 0;
    static constexpr Dialogue events[] = {Dialogue::Greeting, Dialogue::Thinking,
                                          Dialogue::Capture, Dialogue::Mistake};
    static const auto compact = [] {
        std::array<std::array<std::array<std::string, 4>, botCount>,2> result;
        const auto original=localization::language();
        for(int lang=0;lang<2;++lang){
        localization::setLanguage(lang?localization::Language::English:localization::Language::Spanish);
        for (int id = 13; id < botCount; ++id)
            for (int expression = 0; expression < 4; ++expression) {
                std::string line = localization::translate(botDialogue(id, events[expression]));
                if (line.size() > 23) {
                    size_t cut = line.rfind(' ', 22);
                    line = line.substr(0, cut == std::string::npos ? 22 : cut) + ".";
                }
                result[lang][id][expression] = std::move(line);
            }
        }
        localization::setLanguage(original);
        return result;
    }();
    return bot < 13 ? lines[bot][face] : compact[localization::language()==localization::Language::English?1:0][bot][face].c_str();
}
const char *botDialogue(int id, Dialogue event) {
    static constexpr const char *lines[botCount][9] = {
        {"Vamos a jugar!", "Pio... buscando ideas.", "Un granito para mi equipo.",
         "Tu rey necesita refugio.", "Hoy canto victoria!", "Aprendi algo nuevo.",
         "Busquemos la mejor jugada.", "Ese si fue un gran salto!", "Respira y prueba otra idea."},
        {"Cada jugada cuenta.", "Una pausa para calcular.", "Esta pieza cambia el plan.",
         "La calma tambien da jaque.", "La paciencia dio frutos.", "Buena partida, aprendamos.",
         "Ordenemos esta posicion.", "Encontraste la combinacion.", "Mira las piezas sin defensa."},
        {"Sistema listo.", "Explorando variantes.", "Material actualizado.",
         "Alerta para tu rey.", "Objetivo completado.", "Ajustare mis calculos.",
         "Ejecuta el mejor plan.", "Solucion verificada.", "Revisa la amenaza rival."},
        {"Exploremos el tablero.", "Busco una pequena ventaja.", "Ahora mejora mi final.",
         "Tu rey pide espacio.", "Una ventaja bien convertida.", "Buen trabajo hasta el final.",
         "Que pieza puede mejorar?", "Esa es una idea muy fina.", "Prueba mejorar tu peor pieza."},
        {"Vamos con energia!", "Hay recursos escondidos.", "Se abren nuevas variantes.",
         "La amenaza es directa.", "La tactica decidio esto.", "Encontraste buenos recursos.",
         "Busca una jugada activa.", "Viste toda la secuencia!", "Calcula la respuesta tambien."},
        {"Hoy probare algo nuevo.", "Donde pongo esta pieza?", "Me llevo una pieza!",
         "Creo que es jaque.", "No me lo esperaba!", "Buena, otra partida?",
         "Ayudame con esta posicion.", "Ahora entiendo la idea!", "Todavia puedes intentarlo."},
        {"Listo para la primera fila.", "Estoy mirando las casillas.", "Esta pieza queda conmigo.",
         "Tu rey tiene visita.", "Esta vez salio el plan.", "Tu plan funciono mejor.",
         "Un reto paso a paso.", "Cada paso tuvo sentido.", "Mira primero al rey."},
        {"A ver que descubrimos.", "Tengo una idea pequena.", "Gracias por esta pieza.",
         "Atencion a esa linea.", "Una sorpresa agradable.", "Aprendi de tu jugada.",
         "Descubre la amenaza.", "Buena mirada tactica!", "Quiza haya una captura."},
        {"Arranquemos con ganas.", "Voy a pensar otro poco.", "Una captura para empezar.",
         "Se estrecha tu refugio.", "Mi plan llego a tiempo.", "Te adelantaste a mi plan.",
         "Busca una entrada al rey.", "Abriste el camino!", "Revisa por donde entra."},
        {"Probemos un plan sencillo.", "Comparo dos caminos.", "Esta captura me ayuda.",
         "Mira esa diagonal.", "El camino era ese.", "Elegiste mejor camino.",
         "Elige entre dos ideas.", "Elegiste con cuidado!", "No olvides la diagonal."},
        {"Que empiece la aventura.", "Busco una casilla segura.", "Una pieza mas en mi lado.",
         "Jaque desde lejos.", "Llegue al objetivo.", "Llegaste antes que yo.",
         "Explora este pequeno reto.", "Encontraste la ruta!", "Busca una casilla segura."},
        {"Hoy quiero mejorar.", "Paso lento, idea clara.", "Mi posicion respira mejor.",
         "El rey necesita cuidado.", "Me sirvio practicar.", "Tu practica se nota.",
         "Entrenemos una idea.", "Eso merece celebrarse.", "Vuelve a mirar sin prisa."},
        {"Vamos a construir un plan.", "Reviso cada pieza.", "Una pieza menos enfrente.",
         "Mi torre tiene una idea.", "El plan aguanto.", "Tu plan fue mas fuerte.",
         "Encuentra la pieza clave.", "Esa pieza hizo la diferencia.", "Prueba con otra pieza."},
        {"Una leccion en el tablero.", "Busco la idea instructiva.", "Una captura con proposito.",
         "El jaque gana un tiempo.", "La actividad fue decisiva.", "Tu calculo fue preciso.",
         "Calcula hasta el final.", "Una secuencia muy clara.", "Busca la defensa mas fuerte."},
        {"Juguemos con iniciativa.", "Organizo mis fuerzas.", "La iniciativa tiene premio.",
         "Mi ataque toma forma.", "Las piezas cooperaron.", "Tu defensa fue excelente.",
         "Coordina a tus piezas.", "Buen trabajo en equipo.", "Una pieza sola no basta."},
        {"Dejemos hablar al tablero.", "Busco equilibrio y espacio.", "Una captura bien medida.",
         "Cambia la seguridad del rey.", "El equilibrio se rompio.", "Mantuviste la precision.",
         "Encuentra el punto debil.", "Encontraste ese detalle.", "Busca una debilidad real."},
        {"Una posicion para disfrutar.", "Reviso el orden de jugadas.", "El orden hizo la captura.",
         "Primero un jaque util.", "El orden fue importante.", "Ordenaste mejor tus ideas.",
         "Que debe ocurrir primero?", "Ese era el orden exacto.", "Invierte el orden y compara."},
        {"A jugar con decision.", "Voy a activar mis piezas.", "Gano material con ritmo.",
         "El ataque gana velocidad.", "La actividad dio resultado.", "Frenaste bien mi ataque.",
         "Busca actividad inmediata.", "Activaste la pieza correcta.", "Mira un movimiento activo."},
        {"Cada defensa tiene ideas.", "Reviso los recursos ocultos.", "Una oportunidad practica.",
         "Una pregunta para tu rey.", "Mi defensa encontro camino.", "Tuviste mucha resistencia.",
         "Encuentra un recurso oculto.", "Resististe con ingenio.", "Aun hay otra posibilidad."},
        {"Aprovechemos el momento.", "Calculo una ruptura.", "Esta captura abre el centro.",
         "Jaque en el momento justo.", "La ruptura fue decisiva.", "Calculaste muy bien.",
         "Abre una linea decisiva.", "La linea quedo abierta.", "Revisa las rupturas de peon."},
        {"Preparemos un buen plan.", "Compruebo todas las opciones.", "Una simplificacion util.",
         "Cambio el ritmo con jaque.", "El plan tuvo continuidad.", "Tu preparacion se noto.",
         "Compara las candidatas.", "Elegiste la mas concreta.", "Compara antes de elegir."},
        {"Piezas activas, ideas claras.", "Busco una buena coordinacion.", "La coordinacion gana pieza.",
         "Dos piezas miran a tu rey.", "Mi equipo llego junto.", "Coordinaste mejor el final.",
         "Une dos amenazas.", "Las amenazas se conectaron.", "Mira donde coinciden piezas."},
        {"Cada detalle importa.", "Reviso la seguridad.", "Una captura sin perder calma.",
         "Hay que responder al jaque.", "Los detalles decidieron.", "Cuidaste bien tus detalles.",
         "Encuentra el detalle tactico.", "No se te escapo el detalle.", "Mira las piezas clavadas."},
        {"Que las piezas salgan!", "Busco abrir nuevas lineas.", "Una captura con iniciativa.",
         "El desarrollo trae jaque.", "La iniciativa se impuso.", "Detuviste bien la iniciativa.",
         "Desarrolla un ataque rapido.", "Todas tus piezas participan.", "Cuenta las piezas activas."},
        {"Demos fuerza a las piezas.", "Busco presion constante.", "La presion gana material.",
         "Tu rey tiene poco aire.", "La presion hizo efecto.", "Soportaste bien la presion.",
         "Busca una amenaza concreta.", "Esa amenaza fue decisiva.", "Pregunta que amenaza el rival."},
        {"Hay espacio para crear.", "Pruebo una idea distinta.", "Una captura inesperada.",
         "Un jaque creativo.", "La idea encontro su camino.", "Me sorprendiste bien.",
         "Busca una idea poco obvia.", "Una solucion con imaginacion.", "Mira mas alla de la captura."},
        {"Vamos a buscar el ataque.", "Calculo hacia el rey.", "El ataque gana material.",
         "La red de mate se cierra.", "El ataque llego a tiempo.", "Defendiste con precision.",
         "Busca la red de mate.", "Cerraste todas las salidas.", "Cuenta las salidas del rey."},
        {"Una buena idea cada turno.", "Miro la mejor casilla.", "Mi pieza mejora al capturar.",
         "Un jaque cambia el plan.", "El plan fue consistente.", "Jugaste con mucha claridad.",
         "Encuentra la casilla ideal.", "La pieza quedo perfecta.", "Piensa donde debe estar."},
        {"Juguemos con dinamismo.", "Calculo el contraataque.", "Gano material con amenaza.",
         "Tu rey recibe el reto.", "El contraataque funciono.", "Te anticipaste muy bien.",
         "Busca un golpe de iniciativa.", "Tomaste la iniciativa.", "Revisa los jaques primero."},
        {"Mantengamos la posicion sana.", "Compruebo la estructura.", "Una captura mejora mis peones.",
         "La seguridad es prioridad.", "Mi posicion resistio.", "Tu juego fue muy solido.",
         "Busca una jugada segura.", "Precision y buena estructura.", "Comprueba que nada quede suelto."},
        {"Busquemos claridad.", "Simplifico mis opciones.", "Una captura para buen final.",
         "Un jaque muy sencillo.", "El final quedo resuelto.", "Convertiste bien tu ventaja.",
         "Haz simple esta posicion.", "La solucion fue elegante.", "Revisa los cambios de piezas."},
        {"Primero, revisar las salidas.", "Reconozco el terreno.", "Objetivo asegurado.",
         "Tu rey quedo expuesto.", "Mision de tablero completa.", "Buen plan de evacuacion.",
         "Encuentra una ruta segura.", "Ruta despejada, buen trabajo.", "Revisa la salida antes de ir."},
        {"Equipo listo para avanzar.", "Reviso las rutas de escape.", "Recurso recuperado.",
         "Cuidado con esa esquina.", "Operacion completada.", "Tu defensa estuvo preparada.",
         "Abre un camino entre piezas.", "Camino despejado.", "Aun hay una ruta mejor."},
        {"Cuidemos cada pieza.", "Busco una forma de ayudar.", "Un recurso para continuar.",
         "Ese rey necesita apoyo.", "El equipo llego unido.", "Protegiste bien a los tuyos.",
         "Salva la posicion con calma.", "Encontraste como ayudar.", "Mira que pieza necesita apoyo."},
        {"Formacion lista.", "Busco una ruptura firme.", "Un obstaculo menos.",
         "Presion sobre tu rey.", "La linea quedo asegurada.", "Tu linea aguanto bien.",
         "Rompe la defensa con calculo.", "Buena fuerza y buen calculo.", "La fuerza necesita una idea."},
        {"Un paso seguro cada vez.", "Cuido nuestros recursos.", "Esto puede servir despues.",
         "No dejes solo a tu rey.", "Llegamos al otro lado.", "Cuidaste bien tu camino.",
         "Haz que cada pieza cuente.", "Ese recurso era importante.", "Conserva lo que necesitas."},
        {"A ver que hay mas adelante.", "Estoy probando otra ruta.", "Me llevo este recurso.",
         "Oye, tu rey esta en peligro.", "Eso salio bastante bien.", "Buena esa, vamos otra vez.",
         "Encuentra una salida creativa.", "Vaya, esa estuvo buena!", "Otra ruta, tu puedes."},
        {"Un bloque, luego otro.", "Construyo la siguiente idea.", "Material para mi inventario.",
         "Tu refugio tiene un hueco.", "La construccion quedo firme.", "Tu construccion fue mejor.",
         "Construye una red de mate.", "Ultimo bloque en su lugar!", "Revisa la base del plan."},
        {"Hay un tablero por explorar.", "Busco una ruta nueva.", "Recurso listo para crear.",
         "Cuidado con ese tunel.", "Exploracion completada.", "Encontraste un buen camino.",
         "Explora las casillas clave.", "Encontraste el camino secreto!", "Prueba desde otra casilla."},
        {"Mi rey pide mas VRAM.", "Calculando en paralelo.", "Una pieza mas para mi GPU.",
         "Tu rey necesita refrigeracion.", "Ese mate tiene ray tracing!", "Me faltaron nucleos CUDA.",
         "Acelera esa combinacion.", "Buen rendimiento por jugada!", "Revisa los drivers del plan."},
        {"¿Solo 64 casillas?", "Necesito más cómputo.", "Movimiento optimizado.",
         "Tu rey necesita un parche.", "Ajedrez demasiado simple.", "Error de simulación.",
         "¿Y el árbol tecnológico?", "Grok ya lo sabía.", "Necesito más datos."},
    };
    const int bot = id >= 0 && id < botCount ? id : 0;
    const int e = static_cast<int>(event);
    return lines[bot][e >= 0 && e < 9 ? e : 0];
}
const char *rivalInteraction(int speaker, int opponent, Dialogue event) {
    if (speaker != 40) return nullptr;
    if (opponent == 39) {
        if (event == Dialogue::Greeting) return "Necesito más VRAM.";
        if (event == Dialogue::Thinking) return "Ese cálculo usa demasiados CUDA.";
    }
    if (opponent == 2) {
        if (event == Dialogue::Greeting) return "¿Quién programó a este robot?";
        if (event == Dialogue::Thinking) return "Necesito actualizar mi IA.";
    }
    return nullptr;
}
int legacyBotForLevel(int level) {
    return level <= 1 ? 0 : level <= 3 ? 1 : 2;
}
chess::Move openingMove(int id, const chess::Game &game) {
    if (id < 0 || id >= botCount || game.result != chess::Result::Playing || game.moves.size() > 1)
        return {};
    auto expected = chess::Position::initial();
    if (game.initialFen != expected.fen())
        return {};
    // Reject inconsistent in-memory history too, rather than applying an opening to a custom board.
    for (auto played : game.moves) {
        auto legal = expected.legal();
        if (std::find(legal.begin(), legal.end(), played) == legal.end())
            return {};
        expected = expected.after(played);
    }
    if (game.position.fen() != expected.fen())
        return {};
    const char *uci;
    if (game.moves.empty()) {
        static constexpr const char *white[] = {"e2e4", "d2d4", "g1f3", "c2c4", "e2e4"};
        uci = white[id % 5];
    } else {
        const auto first = game.moves.front().uci();
        if (first == "e2e4")
            uci = (id == 2 || id == 4) ? "c7c5" : "e7e5";
        else if (first == "d2d4")
            uci = id >= 3 ? "g8f6" : "d7d5";
        else
            uci = "d7d5";
    }
    const std::string text = uci;
    chess::Move candidate{chess::square(text.substr(0, 2)), chess::square(text.substr(2, 2)), 0};
    auto legal = game.position.legal();
    return std::find(legal.begin(), legal.end(), candidate) != legal.end() ? candidate
                                                                           : chess::Move{};
}
} // namespace ai
