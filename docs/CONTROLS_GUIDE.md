# Inicio y ayuda para jugar

El arranque muestra una pantalla de titulo deliberada. Abrir el programa **no
inicia una partida ni pone a correr un reloj**: se entra primero al menu y luego
se elige Nueva partida o Continuar. Las cinco miniaturas de rivales en la portada
son decorativas; los rivales se eligen dentro de Nueva partida → Rival.

## Primero: como mover

1. El tablero esta en la pantalla superior. No puedes tocarlo con el lapiz.
2. Mueve el marco con la cruceta hasta una pieza del jugador de turno.
3. Pulsa A para seleccionarla: aparecen sus destinos legales.
4. Lleva el marco al destino con la cruceta y pulsa A otra vez.
5. B cancela la seleccion. Una pieza rival no se selecciona como propia.

Dorado significa blancas; azul significa negras. Blancas empieza abajo y mueve
primero. Contra IA, elegir negras gira tablero, coordenadas y controles para dejar tus piezas abajo. En dos jugadores locales la vista permanece fija.

## Pistas, pausa y ayudas

X o el boton PISTA solicita una sugerencia. Se marca una pieza y una flecha hacia
el destino. La pista no ejecuta la jugada: puedes seguirla o jugar otra cosa.
Si hay reloj, pensar o pedir una pista pertenece al tiempo del jugador; la pausa
START detiene los relojes. SELECT muestra informacion adicional del motor.
Los botones y menus de la pantalla inferior si pueden tocarse.

## Como termina una partida

- Jaque: tu rey esta amenazado y tu movimiento debe resolverlo.
- Jaque mate: tu rey no puede escapar; pierdes. Si le das mate al rival, ganas.
- Ahogado: el jugador de turno no tiene jugadas legales, pero no esta en jaque;
  la partida es tablas, no una victoria del atacante.
- Tambien hay otros resultados de tablas, rendicion y perdida por tiempo. Existen
  excepciones de material que impiden ganar por tiempo; el juego las comprueba.
- La escena final identifica Victoria, Derrota o Tablas y mantiene accesibles las
  acciones posteriores; no hay que deducir el resultado mirando solo el tablero.

## Modos y progreso

Jugador vs IA permite escoger rival y color, y da recompensas al terminar una
partida valida. Dos jugadores es para turnarse en la misma consola. Practica es una
partida contra IA sin reloj ni premios y permite deshacer desde herramientas. Las monedas no son dinero real y sirven para la
tienda de personalizacion. Guardar y menu conserva la partida para Continuar.
El reloj inicial es Sin limite; los controles con tiempo son opcionales.

## Integracion de estas pantallas

`ui/onboarding.hpp` expone:

- `drawTitleTop(renderer,ticks)`: pantalla superior400×240, con titulo, piezas,
  rivales y movimiento discreto. Inicia la escena superior.
- `drawHelpTop(renderer,page)`: ilustracion correspondiente a una de cuatro
  paginas; inicia la escena superior.
- `drawHelpBottom(renderer,page)`: titulo y explicacion dentro de y42..187,
  conservando el encabezado de perfil de quien llama. No inicia/limpia la escena.
- `helpPageCount=4`.

Las paginas son mover piezas, pistas/pausa, ganar/perder y modos/progreso. Izquierda
y derecha navegan; B vuelve. La franja de botones inferiores esta entre y198 y219:
Anterior x12..94, Volver x113..207, Siguiente x226..308. La raiz del juego gestiona
los eventos y el retorno al menu/pantalla desde donde se abrio la ayuda.

Todos los elementos se dibujan con los sprites y fuente del proyecto, coordenadas
enteras y ampliaciones nearest-neighbor. No hay imagenes externas, descargas ni
animaciones 3D nuevas. Las comprobaciones de render de este modulo en Mac no
sustituyen una verificacion visual en hardware fisico.
