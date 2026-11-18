# 3DS Chess 0.9.0 RC — informe de implementación

Actualización del 17 de noviembre: [auditoría de B, Volver, toques y pruebas](NAVIGATION_AUDIT_090.md). La verificación manual en Azahar sigue siendo parcial.

Fecha de trabajo: 16 de noviembre de 2026. Plataforma de compilación y pruebas:
Mac Apple Silicon. Destino: Old Nintendo 3DS XL, C++17, ARMv6K, libctru y Citro2D.
**No se realizó ninguna prueba física.** En el empaquetado inicial del 16, Azahar se omitió siguiendo la indicación
del propietario. La comprobación manual del 17 fue parcial; las correcciones de
navegación aún no se probaron directamente en el emulador. Compilar no demuestra estabilidad en consola.

## 1. Estado inicial y preservación

La fuente inicial se identificaba como **0.8.2 RC**. Antes de modificarla se ejecutó
`make -j3 all test`, con resultado correcto, incluidas reglas, perft inicial
197281 a profundidad cuatro, guardados, entrenamiento, interfaz, audio/recursos,
IA y cancelación. Registro: `build/update-090/baseline.log`.

Se creó y verificó el ZIP recuperable `build/update-090/baseline-082.zip`, incluyendo
código, pruebas, documentación, recursos y los artefactos locales disponibles.
Su SHA-256 está en `build/update-090/baseline-082.sha256`:

```text
f14909daeffc1fa64788d2892c8c5dd6babb46c44f4e1dbd9c082febcf77b4ad
```

Es una copia **local** que incluye archivos personales; no forma parte del ZIP de
código fuente entregable. Este directorio de trabajo no contiene repositorio Git;
no se inventaron commits ni se eliminó historial. Se trabajó sobre el proyecto
existente, sin reconstruirlo. La identificación de versión se conserva en un
único archivo, `include/app_version.hpp`, y solo se actualiza después de pasar
las pruebas funcionales de los sistemas comprometidos.

## 2. Funciones implementadas

### Academia interactiva

Acceso: menú principal → Entrenamiento y campaña → Academia interactiva.
Diez lecciones con introducción, FEN preparada, objetivo, tablero real, validación,
explicación de acierto/error, pista, repetición, desbloqueos y guardado independiente:

1. Movimiento y valor: capturar una dama con una torre; valores de las piezas.
2. Centro: d4 o e4 desde la posición inicial.
3. Desarrollo: Cc3 o Cf3.
4. Protección del rey: salir legalmente del jaque.
5. Enroque: cualquiera de los dos enroques legales de la posición.
6. Ataque doble: caballo contra dama y torre.
7. Clavada: alfil, torre rival y rey alineados.
8. Ataque descubierto: apartar alfil y abrir jaque de torre.
9. Táctica de caballo: mate ahogado.
10. Final de rey y dama: mate con apoyo del rey.

Los objetivos se comprueban por reglas y predicados, no por comparar únicamente
una cadena con la pista. Las pruebas recorren las jugadas legales y aceptan 23
respuestas entre las diez posiciones. No es un curso exhaustivo de todos los
finales ni un tutor generativo. Las explicaciones se muestran dentro de la pantalla
inferior, sin ventanas bloqueantes. No concede monedas competitivas ni modifica
la partida normal. [Detalles](UPDATE_090_ACADEMY_STYLES.md).

### Torneos offline

Cuatro u ocho participantes, eliminación directa, cuadro visual de hasta siete
encuentros, rival seleccionado para el primer cruce, progreso, reanudación,
campeón y trofeo simbólico. El humano ocupa la cabeza de serie número uno.
El cuadro muestra identidades estables y resalta quién avanza.

Los cruces IA contra IA se disputan mediante jugadas legales reales. Se usa un
solo trabajador asíncrono, nivel interno cero y un límite adicional cooperativo de
20 ms por búsqueda, con estilo del rival. El contrincante del humano usa su nivel
habitual. No se sortean vencedores ni se ejecutan búsquedas paralelas.

Primera tabla: revancha con colores invertidos. Segunda tabla: avanza la mejor
cabeza de serie, como desempate administrativo anunciado, no como victoria de
ajedrez ficticia. A las 240 medias jugadas se declaran tablas y se aplica la misma
política. Los torneos no usan reloj ni conceden monedas competitivas. Se cancela
la simulación al salir o suspender; se guarda cada avance. La partida normal se
restaura al abandonar el modo. [Detalles](UPDATE_090_TOURNAMENT.md).

### Biblioteca de partidas

Archivo local de hasta treinta partidas terminadas de los modos normales,
locales o de práctica; el entrenamiento temporal y los encuentros de torneo se
mantienen en sus propios sistemas. Cada entrada contiene ID de sesión, rival,
color, modo/nivel, resultado/motivo, control de tiempo, FEN inicial/final y jugadas.
La fecha es desconocida cuando no hay una fuente fiable; el codec admite fecha
ISO comprobada y el PGN utiliza `????.??.??` si se desconoce.

Se puede consultar con izquierda/derecha, reproducir mediante la revisión
existente, exportar PGN y eliminar mediante confirmación. El borrado conserva las
estadísticas y no reaparece al cargar el resultado otra vez. La política es
rechazar nuevas entradas al llenarse: **no se sobrescribe ninguna partida antigua**.
El aviso aparece al intentar archivar y al consultar la biblioteca llena. Para
archivar el resultado actual pendiente se puede liberar espacio y volver a guardar;
las partidas omitidas después de comenzar otra no se recuperan mágicamente.

No se guardan imágenes ni treinta objetos Game completos. Las posiciones se
reconstruyen a partir de movimientos legales. [Formato y límites](UPDATE_090_ARCHIVE.md).

### Estadísticas y estilos

En Estadísticas y logros, SELECT abre las nuevas estadísticas: W/D/L por los
cuarenta rivales, porcentaje entero de victorias, partidas por color, racha actual,
mejor racha y victorias por seis niveles. Academia y torneos aparecen como
progresión independiente. Las estadísticas nuevas cuentan resultados de modo
competitivo desde esta actualización, incluso finales breves sin premio; práctica,
local, academia, campaña, puzzles y torneos no se mezclan con ellas. Los totales
antiguos del perfil y sus reglas de premios se conservan separados.

La migración guarda el identificador ya liquidado como punto de corte. No inventa
repartos por rival/color de partidas históricas. Las marcas independientes de
estadísticas y archivo impiden contar dos veces y permiten contar un resultado
competitivo aunque la biblioteca esté llena.

Equilibrado, Agresivo, Defensivo, Posicional, Táctico y Finales son configuraciones
del mismo motor. Modifican modestamente movilidad, presión, centro, peones,
escudo del rey, cambios y actividad del rey en finales. Sesgo limitado a ±180
centipeones. Conservan legalidad, material base, seis niveles y presupuestos.
Equilibrado mantiene la evaluación anterior. Se asignan perfiles a los cuarenta
IDs y pueden sustituirse en Opciones → Accesibilidad / información. No se afirma
imitar auténticamente a ajedrecistas reales ni tener Elo medido.

### Calidad de vida y funciones conservadas

SELECT abre el historial sin alterar la partida; información opcional de pieza
seleccionada, reducir animaciones, explicaciones educativas en varias líneas,
confirmación de borrado y progreso visible. La rendición confirmada ahora puede
realizarse durante cualquier turno y corresponde al jugador que se rinde.
Último movimiento y confirmación de rendición ya existían y se reutilizaron.

Permanecen los cuarenta rivales, sus IDs, 160 expresiones, diálogos, 41 temas y
sonidos, práctica/deshacer, campaña de ocho etapas, ocho puzzles, cuatro retos,
relojes, siete logros, trece artículos, inventario, personalización, tablero girado
y dos jugadores locales con perspectiva fija. No hay red, servicios externos,
Stockfish distribuido ni nuevas dependencias de ejecución.

## 3. Arquitectura, archivos y guardados

- `source/training/academy.cpp` y su cabecera: catálogo, objetivos, ejecución y progreso.
- `source/tournament/tournament.cpp`: cuadro, resultados, simulación y persistencia.
- `source/archive/archive.cpp`: entradas compactas, estadísticas, codec, PGN y respaldo.
- `source/ai/styles.cpp`, Search y Worker: evaluación configurable; estilo inmutable por búsqueda.
- `source/ui/app.cpp`: integración, aislamiento temporal de modos, revisión y menús.
- `source/ui/preferences.cpp`: opciones separadas, lectura acotada y respaldo.
- Makefile, pruebas y herramientas de empaquetado: compilación/verificación reproducible.

El inventario comparado con el ZIP inicial está en `UPDATE_090_FILES.json`.
Los recursos visuales no se cambiaron para crear estos modos.

| Archivo | Formato y responsabilidad |
|---|---|
| profile.dat / .bak | Formato 3 existente; continúa leyendo 1 y 2 |
| training.dat / .bak | Puzzles, campaña y retos existentes, sin cambiar |
| academy.dat / .bak | ACADEMY1, diez bits de progreso consecutivo y checksum |
| tournament.dat / .bak | TOURNAMENT1, cuadro, partida actual, ID y contadores |
| archive.dat / .bak | 3DSARCHIVE 1, hasta treinta entradas y estadísticas nuevas |
| ui.dat / .bak | UI02; lee también UI01, reducción de movimiento/info/estilo |

Se utilizan temporales, validación, sincronización, respaldo válido y renombrado.
Las sumas detectan corrupción accidental; no son firmas contra manipulación.
La biblioteca rechaza sobrescribir datos si ambos archivos existentes son
inválidos. Corrupción de un sistema no elimina el perfil ni la partida normal.
Las pruebas inyectan fallos de escritura/flush/sync/cierre/respaldo/commit,
pero no prueban todos los posibles cortes físicos de energía.

Los guardados antiguos cargan en 0.9.0. Para retroceder al ejecutable 0.8.2,
restaurar su respaldo anterior: no conoce los archivos nuevos y su validador no
contempla una rendición fuera del turno del perdedor. No se promete que una
versión antigua pueda interpretar funciones nuevas.

## 4. Problemas encontrados y correcciones

Todos los siguientes corresponden a la integración nueva salvo donde se indica:

| Problema confirmado | Gravedad / condición | Corrección y evidencia |
|---|---|---|
| Lector de torneo rechazaba bloques válidos mayores de 1024 bytes | Alta / reproducible con historial largo | Validar fin de archivo completo; fixture real de 239 plies y recarga |
| Final por tiempo se guardaba en perfil pero faltaba archivo inmediato | Media / siempre en ese camino de finalización | Archivo tras guardado correcto; reloj inyectado y recarga sin duplicados |
| Tope de 240 dejaba el tablero humano en Playing después de resolver el cruce | Alta / siempre al alcanzar el tope | Resultado Draw visible; prueba 239→240 y revancha |
| Reutilización posible de IDs al recuperar perfil anterior a archivo independiente | Media / recuperación desfasada | nextId mayor que marcas archivadas; prueba con perfil ID1 y archivo ID9 |
| API resign atribuía derrota al turno actual | Riesgo en API; UI anterior ya lo bloqueaba | Color explícito y codec compatible con resultados antiguos; UI prueba ambos turnos |
| Revisión archivada podía usar orientación de la sesión normal | Media / colores diferentes | Fuente y color propios para revisión; normal inmutable |
| SDK instalado incorporaba una barrera con atributos ARMv7 al enlazar | Alta para el objetivo de compilacion / reproducible en ELF | Specs locales CP15, ELF/21 objetos ARMv6K y probe enlazado; no se observó fallo físico |
| Backup de biblioteca se validaba aunque se reemplazaría por principal válido | Coste confirmado / guardados con respaldo | Lectura condicional; benchmark pareado y regresiones de fallos |

Se corrigió además una ambigüedad de nombre del ayudante `training` de la nueva
prueba de UI antes de considerarla aprobada. Los fallos intermedios no se ocultan
como ejecuciones exitosas; los registros finales muestran las correcciones.

## 5. Rendimiento y memoria

No se aumentó la pila principal ni la de búsqueda: ambas siguen en 256 KiB.
La tabla de transposiciones mantiene tamaño; no hay recursos gráficos nuevos.
La simulación de torneos utiliza un trabajador a la vez y puede cancelarse.

Benchmark reproducible en esta Mac: treinta partidas legales de dieciséis medias
jugadas, doce muestras; archivo de 8981 bytes. Guardado medio pareado
**4.779 → 3.271 ms**, reducción aproximada del **31.6 %** al evitar validar un
respaldo innecesario. Carga media 1.522 ms, reproducción final 0.050 ms y PGN
0.067 ms en esa muestra. No son tiempos de ARM11 ni predicciones de rendimiento físico.

Corpus de estilos: 24 búsquedas sobre cuatro posiciones a profundidad máxima tres,
6000 nodos por búsqueda, cuatro elecciones diferentes respecto a Equilibrado y
76360 nodos totales; seis pruebas de mate y cancelación. Diferencias observables
no equivalen a fuerza ajedrecística mejorada ni imitación humana.

ABI ARM medido en objetos: App 4568 bytes frente a 2296 iniciales; Entry 184,
State de archivo 1144, Game 160 y Move 12. Son tamaños fijos, no el pico de RAM.
Treinta historiales extremos de 16384 plies sumarían 5.625 MiB solo en movimientos,
además de temporales y buffers. Ese extremo no se ha validado física ni temporalmente.
[Medición, comandos y limitaciones](UPDATE_090_PERFORMANCE.md).

## 6. Pruebas realizadas

Línea base sin cambios: `make -j3 all test` — correcta.
Integración: `make -j3 all test sanitize-update` — correcta antes de actualizar
la identificación de versión y otra vez sobre 0.9.0 RC: resultado correcto.

`make test` ejecuta ahora 26 programas C++ host, además de las comprobaciones Python
incluidas. Nuevas suites: test_academy, test_styles, test_tournament, test_archive,
test_update_ui, test_preferences y test_navigation. Incluyen soluciones alternativas/desbloqueos,
cuadros completos4/8 con IA real, empate/revancha/cancelación, persistencia,
truncamientos/corrupción, errores I/O, PGN/replay, todos los rivales/niveles,
liquidación idempotente, aislamiento de modos y migración.

Comandos de sanitizadores y comparación ejecutados:

```sh
make -j3 all test test-target
make -j2 sanitize-update
make -j3 sanitize-content
make -j2 sanitize sanitize-learning sanitize-polish sanitize-clocks sanitize-audit sanitize-storage sanitize-native differential
```

ASan y UBSan no detectaron errores en las ejecuciones aprobadas. La comparación
con python-chess pasó 3012 posiciones FEN/legal/SAN y 329 historias de
adjudicación/repetición/reclamaciones. Se conserva perft197281. El estrés existente
cubre cien cancelaciones, cincuenta sustituciones y treinta cancelaciones de pistas.
No se infiere ausencia universal de carreras o errores a partir de estas pruebas.

Registros reales en `build/update-090/`: baseline.log, final-verification.log,
new-tests.log, release-verification.log, release-sanitizers.log, content-sanitizers.log, regression-sanitizers.log,
update-ui-test.log, update-ui-sanitize.log y benchmark_archive_paired_*.log.
Las imágenes `build/preview-update-*.png` se generaron con el renderizador host
compartido y se inspeccionaron; no son capturas de consola ni emulador.

La compilación completa devkitARM genera ELF ARMv6K, .3dsx y .smdh.
Se detectó que `3dsx.specs` del SDK instalado incluye `sync-dmb.specs`: forzaba
extraer `_sync_dmb.o` de libgcc, etiquetado ARMv7, aunque su texto estaba descartado
en el mapa inicial. No se afirma que se ejecutara una instrucción incompatible
ni que causara un fallo físico. `tools/3dsx-armv6k.specs` conserva el arranque y
las opciones del SDK, cambiando solo a `sync-cp15dmb.specs`; no modifica el SDK
global. La barrera CP15 utiliza la misma operación MCR que la cabecera de
sincronización de libctru. `make test-target` verifica ELF y21 objetos ARMv6K,
compara los specs con el SDK instalado y enlaza una llamada explícita de prueba:
comprueba que resuelve a MCR CP15, sin ejecutar código objetivo. También se añadió
la dependencia de Makefile/specs al enlace para no reutilizar un ELF obsoleto. La versión
canónica se incorpora a interfaz, SMDH y manifiesto; el empaquetador rechaza
versiones obsoletas y recursos personales dentro del paquete principal.

## 7. Pendientes y límites

- Validación física completa de Old 3DS XL, servicios de audio, suspensión,
  latencia, SD, consumo y estabilidad. No se simularon estas mediciones.
- Historiales extremos y pico de RAM/latencia de biblioteca en hardware.
- Los defectos gráficos históricos de tinte/capturas siguen pendientes de
  verificación; no se adjudica su solución a esta actualización.
- Detección general de posiciones muertas continúa incompleta, como en la base;
  no se añadieron heurísticas que declaren tablas falsas.
- Fecha real se omite sin fuente fiable; la revisión sigue siendo aproximada.
- Sin importación PGN, red, nuevos personajes ni instalación CIA.
- Permisos de redistribución de recursos siguen pendientes. Todo se mantiene local.

[Lista de pruebas físicas para el propietario](PHYSICAL_TESTS_090.md).

## 8. Entregables y veredicto

En `dist/`: 3ds-chess.3dsx, 3ds-chess.smdh, 3ds-chess-sd.zip,
3ds-chess-source.zip, RELEASE.json y SHA256SUMS. Piezas originales en el principal.
Variante personal, cuando sus recursos están disponibles, en `private/` con su
propio SHA256SUMS, separada de los ZIP. No hay CIA ni publicación externa.

Los ZIP contienen este informe, cambios, pruebas físicas, inventario de derechos
y documentación de módulos. La copia recuperable 0.8.2 sigue en build/update-090.

**Veredicto: LISTO PARA PRUEBAS FÍSICAS.** La verificación final y las dos
compilaciones concluyeron correctamente. [Resultados estructurados](TEST_RESULTS_090.json).
Esto no significa estabilidad demostrada ni autorización para redistribuir los
recursos. Los hashes de los artefactos están en dist/SHA256SUMS y private/SHA256SUMS.
