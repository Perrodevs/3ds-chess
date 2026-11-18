# Pantalla final y caída de piezas

`ui::EndScene` es una escena puramente visual: no modifica el tablero, los relojes,
las estadísticas ni las monedas. La interfaz debe iniciarla después de adjudicar
y guardar el resultado real de la partida.

La animación dura **180 cuadros**, aproximadamente tres segundos a60FPS. Caen
quince sprites del bando derrotado: ocho peones, dos caballos, dos alfiles, dos
torres y una dama. Empiezan de forma escalonada cada ocho cuadros, aceleran con
una trayectoria entera sencilla, tienen un rebote discreto y quedan en el suelo.
La última dama aterriza al cuadro160. Después aparece una dama grande del mismo
bando en el centro, junto al texto **VICTORIA** o **DERROTA**.

El color se determina con el resultado, no con la orientación del tablero:

- Si gana el humano dorado, caen piezas azules; si pierde, doradas.
- Si gana el humano azul, caen piezas doradas; si pierde, azules.
- En local se muestra VICTORIA y el bando ganador explícito, porque ambos lados
  son humanos y no existe un único resultado de victoria/derrota del usuario.
- En tablas no hay bando derrotado ni caída; aparecen dos damas y TABLAS.

La textura de la dama y las demás piezas es el atlas ya existente. Se respeta la
skin equipada y el escalado entero nearest-neighbor; no hay gráficos3D ni recursos
nuevos con licencia independiente. No hay física ni asignaciones por cuadro:
quince posiciones se calculan determinísticamente a partir del contador saturado.

## API para la interfaz

- `begin(session)`: copia únicamente resultado, bando humano y modo.
- `update()`: avanza un cuadro hasta180 y después permanece estable.
- `skip()`: salta al resultado completo sin alterar ningún dato de partida.
- `finished()`, `defeatedSide()` y `title()`: permiten botones y pruebas.
- `drawTop(renderer,skin)`: inicia y dibuja exclusivamente la pantalla superior.
  La interfaz conserva la pantalla inferior para resumen y botones.

`tests/test_end_scene.cpp` verifica los cuatro casos de victoria/derrota con
colores humanos opuestos, modo local, tablas, estado en curso, fin acotado y
salto. También produce snapshotsSVG de cuadros0/80/160/180 en el renderizador de
Mac. La compilación ARM de la escena no equivale a una prueba física de animación.
