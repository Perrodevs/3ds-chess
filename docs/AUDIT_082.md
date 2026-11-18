# Auditoría integral — 3DS Chess 0.8.2 RC

Fecha: **2026-11-16**. Proyecto existente, sin reconstrucción ni pérdida de funciones.
**Veredicto: LISTO PARA PRUEBAS FÍSICAS**, exclusivamente como candidata local.
No significa estable en hardware real. No se efectuó ninguna prueba física.
Azahar no se ejecutó en esta auditoría: la Mac estaba bloqueada y el propietario
indicó continuar sin él. No se tocaron sus preferencias ni el guardado del emulador.

## A. Estado inicial

Se inventariaron source/include/tests/tools/docs, Makefile, recursos, SDK, guardados
y entregables antes de modificar. `make -j3 all test` terminó con código0:
`build/audit-082/baseline.log`. Las pruebas anteriores no detectaban los casos
adversos ahora añadidos. SDK devkitARM/libctru/Citro2D/Citro3D y herramientas de
host estaban instalados. No se instaló ninguna dependencia nueva.

No existe .git ni repositorio Git padre. No se crearon commits ni se inicializó
un repositorio artificial. Se conservaron hashes iniciales, copia fuente y
entregables0.8.1 en build/audit-082. La copia fuente coincide exactamente con los
hashes iniciales de los archivos auditados; se conserva un diff local changes.patch.

### Inventario funcional verificado

| Sistema | Estado real | Evidencia / límite |
|---|---|---|
| Arquitectura C++17 / ARMv6K | Implementada, compila | Módulos separados; no Stockfish integrado, sin requisitos New3DS |
| Reglas y notación | Implementadas y probadas | Perft/diferencial; posiciones muertas generales incompletas |
| IA / seis niveles | Implementada y probada | PVS, alpha-beta, TT contextual, evaluación; sin Elo medido |
| Hilos / cancelación | Implementados y reforzados | Estrés ASAN/UBSAN/TSAN; llamadas Worker desde hilo UI |
| UI / giro negras / revisión | Implementados y regresiones pasan | Renderer host; ejecución visual nueva en Azahar pendiente |
| Renderer / NDSP | Ramas objetivo auditadas | SDK instrumentado; no sustituye GPU/DSP físicos |
| Catálogo |40IDs estables,160expresiones,360frases,41temas | Selección/persistencia/atlas/PCM comprobados |
| Relojes |5controles, fracciones, incrementos y resultados | Tiempo simulado inyectado; suspensión física pendiente |
| Perfil / economía / tienda |7logros,13productos,6pagos | Validación, idempotencia y persistencia comprobadas |
| Guardado | v1/v2/v3 +TRAINING1 preservados | Corrupción/truncado/fallos lógicos y recuperación |
| Entrenamiento |8puzzles,8etapas,4retos | Soluciones/alternativas/desbloqueos; partida normal aislada |
| Exportación PGN | Implementada, cierre comprobado | No existe importación PGN en interfaz |
| Campaña narrativa extensa / precisión global / online | Ausentes | No se añadieron ni se presentan como implementados |
| Documentación/licencias | Actualizadas / revisión de derechos parcial | Paquetes locales; no autorización pública de todo el arte |

## B. Problemas confirmados

Gravedad se refiere al efecto técnico; probabilidad al disparador, no a una
frecuencia medida. Las rutas normales ya validaban muchas entradas corruptas.

| ID | Gravedad / probabilidad | Archivo y función | Fallo comprobado |
|---|---|---|---|
| R1 | Alta / baja, Move corrupto | source/chess/chess.cpp, Move::uci | Lectura fuera de límites al usar promotion99; ASAN antes del cambio |
| R2 | Alta / baja, historial corrupto | mismo, Game::notation/pgn | after recibía from=-1/to=80 tras SAN fallido; ASAN |
| R3 | Media / baja, FEN imposible | mismo, Position::fromFen | Aceptaba exceso de peones/material promovido |
| R4 | Media / baja, argumento inválido | mismo, perft | Profundidad negativa sin condición de término |
| S1 | Media / baja, archivo incoherente con checksum válido | source/storage/store.cpp, decode | Suma unsigned de estadísticas podía desbordar y validar datos incoherentes |
| S2 | Alta para API inválida / baja en UI | source/economy/profile.cpp, moveAchievements | Índices sin validar y logro por petición de promoción ilegal |
| S3 | Media / error de escritura diferido | source/storage/store.cpp, exportPgn | Anunciaba éxito antes de comprobar flush/cierre |
| G1 | Alta / baja, fallo de framebuffer | source/graphics/renderer.cpp, init | Target nulo usado tras anunciar inicialización correcta |
| G2 | Alta / salida con GPU en vuelo | mismo, shutdown | Recursos liberados antes de esperar cola GPU; orden comprobado, no fallo físico reproducido |
| A1 | Media / reserva musical fallida | source/graphics/audio.cpp, init | Anunciaba audio disponible con buffers incompletos |
| U1 | Media / navegación normal | source/ui/app.cpp, rows |4opciones seleccionables en portada con3dibujadas |
| U2 | Media / toque en hueco o margen | mismo, update | Área táctil activaba opciones fuera del botón visible |
| U3 | Media / A+táctil simultáneos | mismo, update | Doble activación atravesaba dos vistas; pausa+A reanudaba inmediatamente |
| U4 | Media / reclamar, salir y entrenar | mismo, go/beginLesson/leaveTraining | Intención de reclamar tablas interceptaba la solución de un puzzle |
| U5 | Media / baja, seguir una pista de subpromoción | source/ui/app.cpp, activate | Se perdía la pieza recomendada al abrir promoción; confirmar dama podía causar ahogado |
| D1 | Baja / leer documentación | README y guías | Recuentos39, perspectiva fija y ausencia de undo ya no describían el código |

Las seis sondas UI fallan en la copia inicial y pasan con el arreglo:
`build/audit-082/ui-before-after.log`. Los errores de formato y guardado tienen
sondas anteriores y regresiones posteriores en los informes de módulos.

### Endurecimiento, separado de fallos normales reproducidos

- Worker coteja posición, FEN inicial e historial al consumir respuestas; valida
  también que la jugada siga siendo legal. Se conserva el join antes de reutilizar
  estado. No se afirma haber encontrado una carrera normal: TSAN no reportó ninguna
  en el estrés ejecutado. La API se usa desde el hilo de interfaz.
- App valida casillas antes de leer from/to y no continúa abriendo revisión si una
  transición de reloj cambió la vista. No se atribuye a esto el bloqueo histórico.
- Audio/Renderer inicializan de forma idempotente y no son copiables como propietarios.
- NDSP finaliza antes de liberar muestras; no se usa solo vaciar cola como garantía
  de fin de consumo físico. Tests verifican el orden en el límite del SDK.
- Entrenamiento hace fsync del temporal y rechaza categorías desconocidas.
- La promoción conserva la pieza de la pista como opción inicial, sin impedir cambiarla.
- Fuente monotónica inyectable para probar fracciones sin sleep real; producción
  conserva steady_clock y los mismos cinco controles de tiempo.
- Etiquetas inferiores del tablero quedan dentro de240píxeles.

## C. Correcciones implementadas

Se corrigieron todos los fallos de la tabla anterior, con pruebas de regresión.
No se retiró ningún personaje, modo, tienda, pista, reloj ni entrenamiento.
Los informes detallados con funciones, precondiciones y pruebas son:

- [Reglas, IA y pila](AUDIT_RULES_ENGINE.md).
- [Persistencia, economía y entrenamiento](AUDIT_STORAGE_TRAINING.md).
- [Gráficos, audio y recursos](AUDIT_RENDER_AUDIO.md).
- [Registro de cambios](CHANGELOG_082.md).

La versión procede de include/app_version.hpp y aparece en portada y SMDH.
El empaquetador comprueba que binario/SMDH contienen esa versión antes de crear ZIP.

## D. Optimización y memoria

### Mediciones

- Position::legal filtra en su mismo búfer y conserva orden: marco estático ARM
 3424→360B,3064B menos, sin heap adicional. No cambia los seis presupuestos.
- Microbenchmark Mac, cinco muestras con cinco perft(inicio,4) por muestra:
 mediana43.286→39.622ms para986405nodos por muestra. Aproximadamente8.5% menos en
 ese conjunto; carga concurrente posible, sin extrapolar a ARM, batería o fuerza.
- Buffer de efectos12000→7000PCM16:10000bytes menos de reserva, sonidos intactos.
- TT real según ABI ARM:16384×24=393216B=384KiB; las notas anteriores de512KiB
 eran incorrectas. No se cambió su capacidad.

### Bloqueo anterior de revisión

-fstack-usage confirma analyze17840B, raíz8408B, negamax4376B por nivel y key3104B
antes de auxiliares. Superaban32KiB con poca recursión. El replay es iterativo;
no apareció un acceso fuera de límites en el historial legal de42medias jugadas.
Los31marcos máximos de negamax suman135656B, antes de raíz/auxiliares.
Se conserva la pila de256KiB previa, justificada estáticamente; no se aumentó.
La regresión reconstruye0..42 y analiza41/42 con promoción y enroque mate.
No se midió una marca máxima física de pila.

### Memoria objetivo calculada

| Componente | Bytes / alcance |
|---|---:|
| Atlas constantes fuente/piezas +retratos en imagen |1310720 |
| Copias GPU/lineales de esos atlas |1310720 |
| Audio lineal completo |366800 |
| TT de elementos |393216 |
| C2D vértices/índices según configuración |573440 |
| Command buffer C3D predeterminado |262144 |
| Framebuffers color/depth estimados sin overhead |1036800 |
| Pila principal / cada hilo de búsqueda |262144 cada una |

Los ámbitos de memoria no se equiparan con RSS de host ni se consideran un pico
físico total. Se excluyen overhead, bibliotecas, shader/colas y heap variable.
El archivo target-layout.txt mide tipos mediante símbolos del compilador ARM:
Position84B, MoveList3076B, Game160B, Search17240B, Worker240B, Hints376B,
State384B, App2296B. Los vectores/cadenas y sus datos dinámicos no están incluidos
en sizeof; el historial sigue consumiendo memoria según longitud y snapshots.

`leaks --atExit` sobre test_audit_ui en Mac reportó0fugas/0bytes en ese escenario
(log leaks-ui.log). No es una certificación universal ni una medición3DS.
El SDK instrumentado comprueba que las reservas de prueba vuelven a cero.
No se midieron batería, consumo, temperatura ni memoria máxima en hardware.

## E. Pruebas ejecutadas

Los registros completos permanecen en build/audit-082; no se inventan pruebas físicas.

| Comando / escenario | Resultado |
|---|---|
| make -j3 all test, antes de cambios | EXIT0, línea base |
| make -B -j3 all test | EXIT0, recompilación completa host/ARM y todas las suites |
| make sanitize sanitize-polish sanitize-clocks sanitize-learning sanitize-audit sanitize-storage sanitize-native | EXIT0, ASAN+UBSAN |
| make differential | EXIT0,3012posiciones y329historiales |
| Worker stress compilado con -fsanitize=thread | EXIT0, sin informes TSAN en escenarios ejecutados |
| /usr/bin/leaks --atExit -- ./build/test_audit_ui | EXIT0,0fugas reportadas |
| tools/audit_assets.py |40IDs,160expresiones únicas/opacas y atlas coherente |
| tests/test_packaging.py |2casos adversos pasan: alcance privado y binarios obsoletos |
| SDK instrumentado de ramas __3DS__ |10fallos de reserva/servicio, rollback, reintento, orden GPU/NDSP |
| Sondas antes/después de UI |6fallos iniciales reproducidos y6pases posteriores |
| Comparación TT activada/desactivada |18posiciones completas, puntuaciones iguales en la muestra |
| Emulación nueva en Azahar |No realizada: Mac bloqueada; propietario indicó continuar sin Azahar |

Perft inicial profundidad4 conserva197281. Se mantienen Kiwipete y referencias
previas, y se añaden cinco casos específicos de enroque, EP, promoción, EP que
expone el rey y evasión de jaque. El diferencial distingue resultados automáticos
de reclamaciones y no confunde can_claim de python-chess con condición actual.

Estrés:100cancelaciones,50sustituciones de búsqueda,30cancelaciones de pista y
rechazo de posición/historial/resultado distintos. Los relojes usan tiempo inyectado;
20pasos de1250microsegundos cobran exactamente25ms, sin esperas reales para su lógica.

La persistencia prueba corrupción/truncados, salida inmutable al rechazar datos,
v1/v2/v3, seis fallos de cada guardado, exportación con límite real del SO en un
hijo, pagos/seis niveles, siete logros, trece artículos y repetición de liquidación.
Las doce soluciones de entrenamiento y sus alternativas pasan; campaña no paga
monedas ni sustituye el guardado competitivo. Los tests no simulan una SD física.

Después de la última corrección de subpromoción se repitieron all/test y sanitize-audit,
incluida la recomendación de torre que evita ahogado. No quedaron pruebas automatizadas fallidas. La única comprobación solicitada
que no se ejecutó fue la nueva interacción con Azahar, por la limitación indicada.

## F. Compatibilidad

- Old3DS/ARMv6K, C++17 y SDK existentes; sin servicios online/dependencias nuevas.
- Exactamente40rivales, IDs0–39 intactos,160expresiones y atlas sin regenerar arte.
- Seis niveles, cinco controles,7logros,13artículos y catálogo de entrenamiento intactos.
- Formatos de perfilv1/v2/v3 y TRAINING1 preservados; no se requiere migración nueva.
- FEN imposibles antes aceptados se rechazan; no son partidas legales que preservar.
- No se cambia la perspectiva local fija; negras contra IA conserva giro completo.
- Variante de piezas personales separada en private/; fuentes privadas excluidas.
- Paquetes marcados de uso local: no se publica ni redistribuye arte sin autorización.

## G. Pendientes y límites deliberados

1. Posiciones muertas generales: detección parcial, sin heurística de tablas falsas.
 Caso demostrable kR6/8/1K6/8/8/8/8/8 b - - 0 1 tiene una captura forzada a KvsK.
 Adelantar adjudicación necesita migración de historiales/resultados antiguos;
 se conserva la semántica previa y se documenta, no se declara resuelto.
2. Tinte/capturas nativas: las llamadas de color cumplen el contrato SDK en pruebas.
 Existe un reporte de [Azahar2471](https://github.com/azahar-emu/azahar/issues/2471)
 sobre C2D_PlainImageTint fallando en Vulkan2126 y funcionando en OpenGL/software.
 Es una pista primaria, no prueba causal en este juego. A/B de backend y origen de
 capturas negras parciales quedan pendientes; no se aplicó un parche especulativo.
3. Audio: bucles de4s y transiciones pueden resultar repetitivos o dar clic al
 cortar en medio. Solo muestras/contrato verificados; evaluación auditiva pendiente.
4. Historial muy largo: decode conserva su límite16384medias jugadas. No se elevó
 arbitrariamente; límites extremos y coste de replay merecen evaluación específica.
5. Personalidades: diseño de ocho perfiles en RIVAL_PROFILE_DESIGN.md, no activados.
 No se cambiaron pesos sin corpus comparativo ni se simulan estilos humanos reales.
6. Recursos con derechos pendientes: bloquean redistribución pública, no pruebas
 locales del propietario. ASSET_RIGHTS.md distingue código, fotografías, avatares,
 fan art e integración personal. No se obtuvieron permisos nuevos.
7. No existe CIA, importación PGN, varias ranuras, online ni precisión global;
 no se presentan como funciones implementadas.

## H. Pruebas físicas posteriores del propietario

- Arranque HomebrewLauncher en Old3DSXL; versión, pantallas, legibilidad y colores.
- Controles físicos/táctiles, márgenes, pausa, giro negras, flechas y animaciones.
- Partidas de todos los rangos, reglas especiales, resultados y revancha.
- Relojes/promoción/pistas; cierre de tapa, HOME, suspensión y reanudación reales.
- Revisión de partida larga, práctica/deshacer y guardado tras salir de entrenamiento.
- Compra/equipamiento/logros y recuperación ante fallos usando datos de prueba.
- Audio con DSP disponible/ausente, volumen, bucles y transiciones escuchados.
- Medir memoria/pila máxima, tiempos de IA, fluidez y batería con método documentado.

No se realizó ni simuló ninguna de estas pruebas físicas. No retirar alimentación
de los datos reales para ensayar corrupción. Checklist ampliado en HARDWARE_CHECKLIST.md.

## I. Entregables locales

Raíz: /Users/pollo/Documents/ChatGPT/3DS Games/3ds-chess

- dist/3ds-chess.3dsx y dist/3ds-chess.smdh: piezas originales, catálogo completo.
- private/3ds-chess-cosunosuke.3dsx y .smdh: variante personal, no redistribuible.
- dist/3ds-chess-sd.zip y dist/3ds-chess-source.zip: **LOCAL TESTING ONLY**.
- dist/RELEASE.json y dist/SHA256SUMS: versión, alcance y hashes.
- private/SHA256SUMS: hashes de la variante personal.
- Este informe, CHANGELOG_082.md y los tres informes de módulos.
- build/audit-082: línea base, hashes, fallos previos, logs finales, métricas y diff.

Los ZIP comprueban CRC, exclusiones, cabeceras y versión canónica. Las piezas
personales no se incluyen en sus fuentes ni en el ZIP principal. Ningún artefacto
se subió a un servicio externo. El código original MIT no convierte todos los
retratos en recursos libres para publicación.

## J. Veredicto

**LISTO PARA PRUEBAS FÍSICAS**, como0.8.2RC local y con los límites anteriores.
Los errores corregibles confirmados se resolvieron con regresiones; las pruebas
host/sanitizadores/ARM pasan. No se declara completamente estable ni validado
físicamente. La comprobación visual nueva en Azahar quedó omitida por decisión
expresa del propietario tras el bloqueo de la Mac.
