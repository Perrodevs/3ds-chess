# 3DS Chess

**1.1.0 RC Legacy — pendiente de validacion fisica.**

Tres perfiles, Developer con codigo `1234`, Carrera, Supervivencia, 19 trofeos,
10 aperturas/20 lineas, editor FEN, 30 posiciones y 30 puzzles personalizados por
perfil, Chess960/Three-check/King of the Hill con IA y bateria aproximada real.
Desde el menu principal, SELECT abre los modos Legacy.
[Informe completo y limites](docs/LEGACY_COMPLETE.md) · [Pruebas](docs/LEGACY_TEST_RESULTS.json).
Ajedrez homebrew nativo C++17 para Old Nintendo 3DS XL. Offline, graficos 2D pixel art,
dos pantallas, motor propio y 40 rivales. Se conserva una copia recuperable de 0.8.2 RC.
[Implementacion](docs/UPDATE_090.md), [cambios](docs/CHANGELOG_090.md),
[pruebas](docs/TESTING.md) y [auditoria anterior](docs/AUDIT_082.md).

## Funciones actuales

- Editor: colocar/retirar, turno/enroque/al paso, FEN local, coleccion y practica.
- Creador: mate1/2/3 probado contra todas las defensas o marcado pendiente;
  captura y salida de jaque con criterios definidos, pistas y contadores propios.
- Variantes con seis niveles, reanudacion, revision, estadisticas y medallas;
  Chess960 usa 960 indices reproducibles y enroque rey → torre original.
- Bateria mediante PTMU: barras 0–5/carga, aviso breve no bloqueante, sin porcentajes.
- Inicio deliberado, ayuda de4 páginas y menús táctiles inferiores.
- Contra IA, dos personas en una consola y práctica sin reloj/premios con deshacer.
- 40 rivales en 8 páginas:3 originales,28 homenajes/bots ajedrecísticos,
 8 invitados de videojuegos y Jensen Huang. [Catálogo](docs/BOTS.md).
- 160 expresiones de 32×32, 360 diálogos ficticios breves,41 bucles musicales originales
 (menú+40 rivales) y efectos. Música apagada y efectos encendidos por defecto.
- Reglas legales, jaque/mate/ahogado, enroques, al paso,4 promociones,
 reclamaciones triple/50 y automáticas quíntuple/75; casos habituales de material
 insuficiente. La detección general de posiciones muertas sigue incompleta.
- Pista X con flecha, sin mover automáticamente; el reloj sigue corriendo.
- Revisión aproximada por jugada, explicación del final, alternativa del motor,
 capturas y revancha manual con mismo rival/color/modo/reloj.
- 8 puzzles, 8 etapas de campaña y 4 retos; progreso separado de la partida normal.
- Academia: 10 lecciones con posiciones, movimientos reales, objetivos, pistas,
  explicaciones, repeticion y desbloqueos persistentes; sin monedas competitivas.
- Torneos offline de 4/8 participantes, cuadro, revancha por tablas y desempate
  administrativo explicito. IA contra IA juega partidas reales con presupuesto,
  una busqueda asincrona a la vez; progreso y trofeos simbolicos separados.
- Biblioteca de 30 partidas: consulta, reproduccion, resultado, reloj, PGN y
  eliminacion confirmada. Si se llena, rechaza nuevas entradas sin reemplazar antiguas.
- Estadisticas nuevas por rival/color/nivel, porcentaje y rachas desde esta
  actualizacion. El historial antiguo no se inventa; entrenamiento queda separado.
- Seis preferencias de IA configurables, con el mismo motor y seis niveles:
  equilibrado, agresivo, defensivo, posicional, tactico y finales.
- Reduccion de animaciones e informacion opcional de piezas en Opciones.
- Relojes opcionales: sin límite,3 min, 5 min, 10 min, 5+3. Pausa detiene ambos.
- Perfil, nombre ASCII hasta 24 caracteres, estadísticas,7 logros y13 artículos.
- Guardado principal con respaldo; entrenamiento con respaldo propio; exportación
 PGN al terminar. No hay importación PGN; cada perfil conserva una partida clasica
 y sesiones independientes de torneo/progresion/variantes.

## Controles y orientación

Cruceta mueve el cursor; A selecciona/acepta; B cancela/vuelve; START pausa;
X solicita pista; SELECT abre el historial durante la partida y las estadisticas
nuevas desde Estadisticas y logros. El tablero superior **no es táctil**.
Menús y botones de la pantalla inferior sí lo son.

Blancas son doradas y negras azules. Al jugar contra IA con negras el tablero,
coordenadas, flechas, cursor y animación giran para dejar tus piezas abajo.
Dos jugadores locales conserva perspectiva fija. En clasico la posicion inicial es
fija; Chess960 permite elegir un indice reproducible.
En revisión: izquierda/derecha recorre jugadas; arriba/inicio, abajo/final.

## Ejecución y artefactos locales

La consola debe tener ya un método para ejecutar homebrew. Este proyecto no
instala firmware ni modifica el sistema. Copia 3ds-chess.3dsx y .smdh a
`SD:/3ds/3ds-chess/` y abre desde Homebrew Launcher.
Los gráficos están integrados; no necesita PNG ni Python en la consola.
NDSP necesita el DSP de la propia consola; si falta, continúa sin audio.
No se incluye firmware DSP, claves ni un instalador CIA.

**Los paquetes completos son exclusivamente para pruebas locales del propietario.**
El atlas contiene recursos con derechos pendientes de revisión. El binario con
piezas originales tampoco está automáticamente autorizado para publicación.
Consulta [inventario de derechos](docs/ASSET_RIGHTS.md) y
[aviso de uso local](docs/LOCAL_TESTING_ONLY.md). No se publica nada al compilar.
Las piezas personales Cosunosuke se conservan solamente en private/, separadas.

## Compilar y verificar

Entorno usado: macOS Apple Silicon, devkitPro/devkitARM, libctru, Citro2D/Citro3D,
3ds-tools. Sin dependencias nuevas para ejecutar el juego.

```sh
make -j3 all test test-target
make sanitize sanitize-polish sanitize-clocks sanitize-learning
make sanitize-audit sanitize-storage sanitize-native sanitize-update
make differential
make audit-assets
make dist
```

`make dist` crea ZIP para SD y fuentes, RELEASE.json y SHA256SUMS con avisos
**LOCAL TESTING ONLY**. El empaquetador exige `--local-testing`, piezas originales
y versión coincidente en código/binario/SMDH. Rechaza incluir la variante personal.
Python/Pillow/python-chess se usan solo en herramientas de desarrollo; las
herramientas instaladas están en .venv y no se empaquetan.

## Motor, guardado y límites

Motor C++ propio: seis niveles 0–5, PVS/alpha-beta, tabla de transposiciones y
valoración de rey/peones/movilidad/finales. Stockfish no se integra ni enlaza;
Stockfish 8 fue una referencia externa de pruebas en Mac. Los Elo mostrados son
metadata de referencia, no fuerza calibrada ni una réplica real de cada persona.

`profile.dat` integra ajustes, perfil, inventario, resultado, reloj e historial;
`.bak` conserva el anterior. Se mantienen lectores v1/v2/v3 y entrenamiento
TRAINING1. [Detalles](docs/STORAGE.md). Las inyecciones de fallos son pruebas de
proceso: no certifican resistencia absoluta a cortes de alimentación de una SD.

No hay online, comunicación entre consolas, cuentas, doblaje, editor de posiciones
ni revisión profunda con precisión global. Se han ejecutado pruebas de host,
sanitizadores, compilación ARMv6K y pruebas de emulación con alcance documentado.
Batería, consumo, temperatura, audio real y suspensión física siguen sin medir.

## Código y recursos

Módulos: source/chess, ai, graphics, ui, storage, economy y training, con cabeceras
en include y pruebas en tests. Código propio MIT, avisos de dependencias en docs/licenses.
Los retratos y recursos de terceros conservan sus condiciones particulares:
[THIRD_PARTY](docs/THIRD_PARTY.md). La licencia del código no cubre esos derechos.
