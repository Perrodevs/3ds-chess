# Testing 3DS Chess

## Current checkout

`main` contains **1.1.0 RC Legacy**. The small GitHub Actions workflow runs
the rules and search tests from a clean host checkout. It checks the new
documentation links and media. Run the same C++ targets locally.

```sh
make -j2 build/test_core build/test_strength
./build/test_core
./build/test_strength
```

The full host suite needs the locally generated assets described in
[Building](BUILDING.md). Run `make test` after provisioning them. For console
targets, `make test-target` builds and verifies ARMv6K ELF attributes and the
linked CP15 barrier. It does not run the executable.

Use a dedicated Azahar user/SD directory and synthetic profiles for interactive
checks. Record emulator and game versions, navigation, saves and exact
limitations. Hardware checks require a physical Old Nintendo 3DS XL and are
tracked separately. See [Legacy results](LEGACY_COMPLETE.md) and the
[RC2 report](https://github.com/Perrodevs/3ds-chess/blob/fix/stability-quality-update/docs/STABILITY_140_RC2.md).

## Historical notes

# Navegación revisada — 2026-11-17

Ver [errores reproducidos, correcciones y límites](NAVIGATION_AUDIT_090.md).
La suite actual contiene 26 programas C++ host, incluido test_navigation.

# Verificacion de la actualizacion 0.9.0 — 2026-11-16

Comandos, resultados y limites vigentes en [UPDATE_090.md](UPDATE_090.md).
Nuevas pruebas: `make test-update`, `make sanitize-update` y suites de modulos
con ASan/UBSan. La ejecucion normal de `make test` incluye los sistemas nuevos.
`make test-target` inspecciona ELF/objetos ARMv6K y un probe de barrera CP15, sin ejecutar codigo objetivo.
La base anterior y su compilacion ARM se preservaron antes de modificar sistemas.
Los registros de esta ejecucion estan en `build/update-090/`.
No hubo pruebas fisicas ni una nueva comprobacion en Azahar.

# Verificación de0.8.2RC — 2026-11-16

El resultado vigente, comandos y límites están en [AUDIT_082.md](AUDIT_082.md).
Los apartados siguientes son registros históricos de versiones anteriores;
sus recuentos de5/13/31/39rivales describen esas versiones, no el catálogo actual.
En0.8.2hay40rivales,160expresiones,360frases y41temas.

# Verificación real — 2026-11-15

## Actualización 0.8: entrenamiento, revisión y audio

Pasó `make -j3 all test sanitize-learning`, registro `build/learning-final-verification.log`.
Incluye todas las suites anteriores y nuevas pruebas de revisión, entrenamiento,
audio/diálogos e integración de menús. Se comprueban replay inmutable, undo blancas/negras,
capturas al paso/promoción, SAN de alternativas reales, soluciones y alternativas de puzzles,
desbloqueos y recuperación de progreso, no otorgar avance por rendirse, campaña sin monedas,
revancha y preservación exacta del guardado normal al entrar/salir de entrenamiento.
La prueba ASAN/UBSAN integrada pasó. Las pruebas de módulos aislados con sanitizers también.

Se comprobaron 40 bucles PCM distintos, límites de amplitud, ausencia de desplazamiento
continuo y351diálogos ASCII acotados. No equivale a una escucha en consola real.

En Azahar2126.2 se entró a Entrenamiento, se abrió el puzzle de Steve,
se solicitó pista con X y se ejecutó Te8#: apareció RETO COMPLETADO y guardado de progreso.
Se volvió a la partida normal de42medias jugadas sin cambiarla ni sus45monedas.
La primera revisión bloqueó ARM por desbordar la pila principal predeterminada de32KiB;
el registro apuntó a Position::key dentro del análisis. Se definió `__stacksize__=256*1024`
en main, igual que el trabajador de búsqueda, y se comprobó el símbolo fuerte en ELF.
Tras recompilar se abrió correctamente la revisión42/42, su atacante (torre d8),
las capturas y la navegación41/42 con una alternativa real (Txe1) y aviso de mate.
La ejecución volvió a59–60FPS después de analizar.

Se activó Música en Opciones: el emulador informa SIN DSP, así que no hubo
comprobación auditiva. Se detuvo la emulación y se restauraron byte por byte los dos
archivos originales de perfil; el progreso artificial quedó archivado bajo build.
No se ha probado Old3DS física. Sigue pendiente el defecto previo de algunas
capturas inmediatas incompletas/tinte del texto nativo, descrito en0.4.


## Actualización 0.7: personajes invitados

Pasaron `make all test sanitize-polish` y `make sanitize-bots`, registros
`build/guests-final-verification.log` y `build/guests-sanitize-bots.log`. Se probaron
los39rivales en selección, recarga, expresiones, navegación circular y huecos
de la última página. Las comprobaciones incluyen nombres/niveles de los8nuevos
y ausencia de Elo de referencia inventado.

El atlas256×1024 tiene156celdas distintas y opacas. Se compararon las31filas
anteriores píxel por píxel: intactas. La nueva organización en bancos mantiene
cada dimensión dentro de1024. Se probaron los39IDs con ASAN/UBSAN y el renderer
compartido; no aparecieron errores de memoria.

En Azahar2126.2 se reinició el3DSX actualizado, se abrió el selector y se navegó
a Jill Valentine en página7/8, viendo también Leon, Claire y Chris. Después
se navegó a Alex en página8/8, con Joel, Ellie y Steve visibles. Los retratos
de ambos bancos de la textura se mostraron. No se inició una partida ni se
reemplazó el guardado del usuario. Continúa pendiente la comprobación en
Old3DS física y el problema de capturas inmediatas incompletas descrito en0.4.


## Actualización 0.6: los 18 personajes de las capturas

Pasó `make all test sanitize-polish`, registro local
`build/masters-final-verification.log`. Se probaron los31IDs en selección,
guardado/recarga, cuatro expresiones, navegación circular y límites de la última
página (una tarjeta). Las pruebas comprueban también los18nombres y Elos
exactos de las capturas y su nivel fuerte, preservando los IDs anteriores.

Se verificó el atlas128×1024:124celdas distintas y opacas de32×32. Las primeras
13filas son idénticas píxel por píxel al atlas de0.5. Los prompts y las26imágenes
fuente consultadas se conservaron.

Se reinició el3DSX actualizado en Azahar2126.2 y se navegó desde inicio hasta
el selector: Capablanca apareció en la página7/7, tarjeta31, con su retrato
y Elo de referencia2725. No se inició otra partida ni se reemplazó el guardado
del usuario. La prueba de GPU emulada cubre la última fila del atlas ampliado.
Continúa pendiente la validación física de Old3DS y la investigación de las
capturas inmediatas incompletas ya descritas en0.4.


## Actualización 0.5: trece rivales

Pasó `make all test sanitize-polish`: compilación ARM, suites anteriores,
selección y recarga de los 13 IDs, expresiones y límites de las tres páginas.
Registro: `build/expanded-final-verification.log`. Se comprobó además que las
52 celdas del atlas son distintas, opacas y tienen tamaño de 32×32.

En Azahar se reinició el 3DSX actualizado desde inicio, se abrió el selector y
se recorrieron sus tres páginas. Se observaron Martin con Elo de referencia
250 y su retrato correcto, y Oliver con Elo de referencia 550 en la última
página junto a Elani y Noel. También se observaron expresiones cambiando.
No se comenzó otra partida ni se reemplazó el guardado de usuario.
Esta prueba verifica el nuevo atlas 128×512 en la GPU emulada; no equivale a
una comprobación física de Old 3DS. Las previews host siguen identificadas como
tales.


## Actualización 0.4: inicio, guía, pistas y resultados

`make test sanitize-polish` pasó: inicio sin autoinicio, cuatro páginas de ayuda,
pista sin ejecutar la jugada, flecha y selección, continuidad del reloj, cancelación
al pausar, resultados de victoria/derrota para ambos colores, mate, rendición,
revisión del tablero, recarga de final y no duplicar monedas. También pasaron
las suites anteriores de reglas, guardado, bots, fuerza y relojes. Compilación ARM
sin errores. Registro local: `build/polish-final-verification.log`.

Se ejecutó el 3DSX en Azahar 2126.2 en esta Mac: pantalla de inicio sin iniciar
partida, guía visual página 1, continuar un mate ya guardado y observar la caída
de piezas doradas con dama central y DERROTA. Se creó una partida de prueba sin
reloj ante Hamchow, se pidió pista con X: flecha b1-c3; se jugó Cc3 manualmente,
la IA respondió d5 y se abrió pausa. Se detuvo la emulación y se restauró el
guardado previo de usuario, conservado en build/emulator-save-before-polish/.
En una prueba previa se jugaron e4 e5, Cf3 Cc6 y se observaron incrementos 5+3.

La nueva lógica de resultado por tiempo y pistas se volvió a verificar tras
proteger entradas en el instante exacto de terminar. Ambos colores y victoria
se verificaron en las pruebas compartidas, no todos mediante juego manual.
Las imágenes host-polish-* son previews de Mac, no capturas del emulador.
No se ha probado esta versión en consola física. En algunas capturas inmediatas
de Azahar aparecieron regiones incompletas; al volver a observar se veía la
escena completa. No se determinó si es captura/compositor o renderizado.


## Actualización0.3: relojes opcionales

ControlesSin límite / 3 min / 5 min / 10 min / 5+3, modelo y timeoutmaterial en test_clock.cpp.
El default es Sin límite; v1/v2 migran sin reloj preservando partida y perfil.
Guardado v3 con validación de incrementos ganados, relojes coherentes y TimeControl
PGN. Integración test_clock_ui.cpp: turno, incremento, pausa, promoción pendiente,
carga offline, suspensión/restauración, caída antes de mover, cancelación de IA,
falta de material para ganar por tiempo y fallo de escritura. Un ensayo con tiempo
real comprueba conservación de fracciones; las demás pruebas inyectan intervalos.

Se ejecutaron las suites y compilación ARM; el registro está en
`build/clocks-final-verification.log`. La prueba de integración con ASAN/UBSAN está
incluida ahí y los módulos independientes fueron revisados por separado.
Los layouts de relojes y menú se inspeccionaron en imágenes producidas por el
renderizador compartido de Mac, no en una consola física. Se intentó abrir Azahar para esta actualización, pero
la Mac seguía bloqueada y no pudo iniciarse la revisión visual.

## Actualización0.2: cinco bots y motor mejorado

Se ejecutó la compilación ARMv6K, suites de reglas, almacenamiento/migraciónv1,
interfaz y selección de los cinco bots, partida completa, comparación diferencial
y sanitizadores. Registro actual: `build/bots-final-verification.log`.

Las pruebas nuevas incluyen todas las identidades persistentes, cancelar/confirmar
selector, rival de sesión separado de la próxima partida, cuatro expresiones,
aperturas legales y PGN con nombre real del bot elegido. Se probó además que
la IA elija un mate disponible sobre reclamar tablas por 50 movimientos. Las20celdas del atlas
son distintas, opacas32×32 y fueron revisadas visualmente en su interfaz de Mac.

La suite táctica prueba mate, dama desprotegida, horquilla, promoción, límites y
cancelación; compara tabla activada/desactivada con historial de repetición y
horizonte de50movimientos. La comparación limitada contra el motor anterior se
explica con detalle en [ENGINE_UPGRADE.md](ENGINE_UPGRADE.md).

Se intentó la nueva revisión en Azahar, pero macOS estaba bloqueado y la
automatización no pudo abrirlo. Por ello **no se ha validado la nueva textura de
retratos ni este motor mejorado en emulador/hardware**. Las capturas host muestran
el código compartido de UI y el atlas real; no sustituyen la GPU ARM.

Lo siguiente conserva la evidencia de la primera versión para distinguir qué se
probó antes. Los92plies y129839nodos corresponden al motor anterior; la nueva
partida completa figura en el registro actual.

---

## Evidencia histórica0.1 RC

## Compilación

macOS Apple Silicon, devkitARM GCC 16.1.0, libctru, Citro2D y Citro3D instalados en
`/opt/devkitpro`. Se compiló y enlazó un ELF ARMv6K y se convirtió con 3dsxtool a
`dist/3ds-chess.3dsx`. También se compiló por separado la variante personal de
Cosunosuke, sin incluirla en los paquetes para redistribución.

El último `make all test sanitize differential` terminó con código 0. Las
bibliotecas del SDK no fueron recompiladas ni actualizadas globalmente.

## Pruebas nativas en Mac

- Perft inicial: profundidades 0–4 = 1, 20, 400, 8902, 197281.
- Kiwipete: 48, 2039, 97862 a profundidades 1–3.
- Posición de torres/peones al paso: profundidad 4 = 43238.
- Posición especial con promociones y enroques: profundidad 3 = 9467.
- Posición de promociones/jaques descubiertos: profundidad 3 = 62379.
- Rey expuesto por captura al paso, derechos de enroque y movimiento de torre,
  cuatro promociones, FEN inválido, mate del loco, notación SAN en dos idiomas.
- Triple repetición reclamable, reclamación con movimiento previsto, quíntuple
  automática, 50 reclamables, 75 automáticos y prioridad del mate.
- Ahogado, rey solo, menor único y dos caballos sin adjudicación incorrecta de tablas.
- Búsqueda: movimiento legal, límite de nodos, cancelación previa y mate en uno.
- Partida completa IA vs IA desde el inicio: **92 medios movimientos, jaque mate,
  129839 nodos acumulados**, con semilla reproducible. Se guardó, restauró y se
  comprobó que su resultado no volviera a otorgar monedas.
- Serialización, detección de corrupción/checksum, respaldo, reemplazo seguro,
  exportación PGN, compras repetidas, saldo insuficiente, equipamiento y logros
  idempotentes. Una rendición inmediata no recompensa.
- UI compartida: navegación, selección física e2–e4, pausa/reanudación, tienda,
  opciones, música persistida, estadísticas, confirmación para reemplazar partida,
  inicio con negras, cancelación del hilo al pausar y respuesta asíncrona.
- UI de promoción: selección real del caballo, resultado por material insuficiente
  y persistencia de la partida terminada. Fallo de escritura simulado en una ruta
  inexistente: la sesión no se confirma y la interfaz conserva el estado previo.
- Comparación independiente contra python-chess: **3000 posiciones**, 20 recorridos
  aleatorios de hasta 150 medios movimientos, semilla 20261115. Coinciden FEN,
  conjunto completo de movimientos legales y SAN de cada movimiento.
- AddressSanitizer y UndefinedBehaviorSanitizer: reglas/IA e interfaz ejecutadas
  sin errores reportados. No certifican el backend ARM ni el hardware DSP/GPU.

Los casos completos están en `tests/`. No se necesita Python para las pruebas C++
básicas; sí para la comparación independiente. `build/final-verification.log`
conserva el registro de ejecución local. El archivo público
`docs/verification-build.log` conserva una copia del registro de pruebas.

## Azahar 2126.2, Apple Silicon

Se ejecutó el `.3dsx` real con Vulkan desde la publicación oficial de Azahar.
Se observaron las pantallas superior/inferior, tablero, piezas y fuente bitmap.
La primera prueba detectó que invertir las filas durante la carga de la textura
ocultaba los sprites; se corrigió y se volvió a probar.

Se navegaron los menús mediante los controles emulados, se inició Jugador vs IA,
se seleccionó e2 con A y cruceta, se jugó e4 y la IA respondió h5 sin bloquear la
interfaz. La barra del emulador mostró aproximadamente 60 FPS/100 % de velocidad.
Se inspeccionó `sdmc/3ds/3ds-chess/profile.dat` y contenía `e2e4 h7h5`, con checksum
y los demás datos del perfil. Este resultado valida escritura en la SD emulada,
no durabilidad de una SD física.

La sesión visual terminó cuando macOS se bloqueó. No se obtuvo una captura
exportada del emulador ni se continuó la partida completa allí. Las imágenes del
README son del renderizador de pruebas de la Mac, claramente etiquetadas.

Los últimos ajustes de tono de texto, música procedural opcional y etiqueta ELO
inferior se compilaron y pasaron las pruebas nativas después de esta prueba visual;
no se volvió a validar su aspecto/audio en Azahar. Tampoco se probó visualmente la
variante personal de sprites. No se confunde compilar con validar estos detalles.

## Sin probar todavía

Ninguna consola física, medición de RAM dinámica máxima, consumo eléctrico,
autonomía, estabilidad a largo plazo, latencia táctil física, calidad del sonido,
suspensión con tapa/HOME ni interrupciones eléctricas sobre FAT real.
La lista ejecutable de comprobaciones está en `HARDWARE_CHECKLIST.md`.

Los casos habituales de material insuficiente están cubiertos. No se implementó
un solucionador general de posiciones muertas: ciertos bloqueos imposibles de
resolver por cualquier secuencia legal siguen figurando como partida en curso.
Esta es una limitación real respecto a una implementación absolutamente completa
de todas las posiciones terminales de las reglas FIDE.

## Actualización 0.8.1: Jensen Huang

`make -j3 all test sanitize-bots` pasó (build/jensen-verification.log).
Se probaron 40 selecciones y guardados, 41 bucles musicales distintos y360frases
acotadas, incluidas las nueve nuevas. El ID39 no modifica los IDs anteriores.
Se compararon los39retratos previos píxel por píxel: intactos. Las cuatro nuevas
expresiones son distintas y el atlas continúa en256×1024. Arte generado con
image_gen, con original y prompt completos guardados en assets/portraits/source.
No se volvió a ejecutar esta adición en emulador ni en consola física.
