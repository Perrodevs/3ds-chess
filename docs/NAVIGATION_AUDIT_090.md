# Auditoría de navegación y de sus pruebas — 2026-11-17

Esta revisión se hizo sobre 0.9.0 RC a petición del propietario: buscar errores de
recorrido, especialmente B/Volver/Cancelar, y comprobar si las pruebas los ocultaban.
Las correcciones conservan la identificación 0.9.0 RC y los formatos de guardado.

## Errores reproducidos antes de corregir

`build/navigation-audit-090/before.log` contiene nueve escenarios fallidos de la
aplicación real compilada para host, con entradas de botones/toques:

| Escenario | Antes | Ahora |
|---|---|---|
| B desde menú principal | Permanecía en el mismo menú | Vuelve a la portada |
| B desde Accesibilidad/información | Saltaba al menú principal | Vuelve a Opciones |
| B desde Torneos | Saltaba al menú principal | Vuelve a Entrenamiento |
| B desde Biblioteca | Saltaba al menú principal | Vuelve a Entrenamiento |
| B al completar una lección/reto | Saltaba al menú principal | Vuelve al catálogo |
| Toque x=0 junto al botón Pausa | Abría Pausa | Se ignora |
| Toque entre Pausa y Tablas | Abría Pausa/acción vecina | Se ignora |
| Toque por debajo de los botones de partida | Activaba un botón | Se ignora |
| Toque x=0 junto al botón Volver de estadísticas | Salía de la pantalla | Se ignora |

La ampliación de la prueba encontró otro fallo, registrado en
`expanded-before.log`: tocar el espacio entre botones de Ayuda volvía al menú.
Se corrigió. También se ajustaron los límites de los cuatro botones de revisión
para que sus huecos y el espacio superior no cambien de jugada ni cierren la pantalla.
Las comprobaciones comparan el renderizado de la revisión antes/después de un
toque inválido; no confunden la inmutabilidad de la partida con la del visor.

## Qué fallaba en las pruebas anteriores

- `tests/test_update_ui.cpp`: el ayudante de entrada a Entrenamiento asignaba
  directamente `app.view = Home`. Eso no probaba cómo se había vuelto a Home.
  Ahora, cuando corresponde, entra desde la portada pulsando A y exige estar en
  el menú esperado. La salida de Torneos con B tenía una pulsación sin comprobar
  su destino; se añadió la aserción de Entrenamiento.
- `tests/test_audit_ui.cpp`: una prueba esperaba expresamente Home después de B
  en TrainingEnd. Esa expectativa perpetuaba el salto al menú principal.
  Se cambió a catálogo, con un caso nuevo que reproduce el fallo anterior.
- `tests/test_learning_ui.cpp`: después de B se forzaba Home, sin comprobar las
  pantallas intermedias. Ahora comprueba catálogo y Entrenamiento y continúa desde allí.
- Faltaban pruebas para toques en huecos/márgenes de controles durante una partida,
  en Ayuda y en revisión. Las pruebas de límites de menús no cubrían esos controles.

Los cambios de expectativa no se usaron para ocultar un fallo nuevo: se guardaron
las reproducciones fallidas y se añadieron recorridos con el comportamiento de
retorno esperado. Los preparativos directos de FEN/partidas en pruebas unitarias
siguen siendo útiles; no se presentan como navegación completa. Las asignaciones
de vistas utilizadas únicamente para generar imágenes tampoco prueban recorridos.

## Correcciones

`App::back()` centraliza los destinos de B y de los controles Volver/Cancelar.
Recupera la fila de origen: volver de Accesibilidad mantiene seleccionada su
entrada en Opciones, por ejemplo. También respeta revisión→origen, ayuda→origen,
selección de rival→configuración, confirmaciones→pantalla que las abrió y
resultado de entrenamiento/torneo→catálogo/cuadro. Si el reloj termina durante
una transición, no se cambia la selección de la pantalla de resultado por error.

Durante una partida B **cancela selección/reclamación pendiente/pista**, tal como
indica la ayuda. No sale de la partida. START abre Pausa. En la animación de
resultado, la primera pulsación de B la salta; una pulsación posterior vuelve.
Estos comportamientos deliberados se conservan y se prueban.

Se bloqueó explícitamente Deshacer dentro de torneos: internamente usan una
sesión temporal de modo práctica, pero el deshacer libre no corresponde al torneo.
Antes el validador rechazaba la continuación acortada como si hubiera un problema
de guardado. La práctica libre mantiene su deshacer.

## Cobertura nueva

`tests/test_navigation.cpp` entra desde `App::init()` y usa A/B/START/SELECT,
cruceta y toques. No asigna vistas para alcanzar la pantalla objetivo. Comprueba:

- Portada, menú, Opciones/Accesibilidad, Tienda, Estadísticas, Entrenamiento,
  Biblioteca, Torneos, catálogo y entrenamiento terminado.
- Ida/vuelta y conservación de la fila de origen al pulsar B o Volver.
- Revisión desde herramientas y retorno a partida sin cambiar el tablero.
- Cancelar salida, rendición y eliminación, conservando partida/registro.
- Pista para resolver realmente un puzzle y retorno al catálogo sin monedas nuevas.
- Toques fuera de botones, debajo y entre botones; revisión visual inmutable.
- B en partida como cancelación, no como abandono.

La suite completa ahora contiene 26 programas C++ host. El nuevo caso se incluye
en `make test`. Comandos ejecutados y aprobados:

```sh
make -j3 test test-target all sanitize-navigation
make -j3 test-learning sanitize-update
```

Registros: `build/navigation-audit-090/verification.log` e `integration.log`.
ASan/UBSan no detectaron errores en esos casos. El ELF y los 21 objetos propios
mantienen ARMv6K; la prueba enlazada de barrera CP15 continúa pasando. El perft
inicial de profundidad cuatro conserva 197281. Se compilan ambas variantes
locales y se actualizan ZIP y hashes. Esto no prueba ausencia de otros fallos.

## Límite de la prueba por Computadora

En esta sesión el inventario de herramientas no expone el control de Computadora,
por lo que **no se pulsó B en Azahar para comprobar estas correcciones**. No se
sustituyó ese acceso por eventos de teclado/ratón desde terminal. La sesión manual
anterior del 17 de noviembre solo confirmó arranque de 0.9.0, menús y apertura del
catálogo de academia; su registro sigue en
`build/emulator-090-20261117/RESULTADO_PARCIAL.md`.

Las pruebas actuales usan carpetas sintéticas bajo /tmp, no los guardados reales
de Azahar. Quedan pendientes la prueba manual de estos recorridos en el emulador,
el problema histórico de tinte con Vulkan y toda validación física. La copia de
la entrega previa está en `build/navigation-audit-090/before-release/`.

## Comprobación manual posterior

Computadora volvió a estar disponible el 17 de noviembre. Se comprobaron B,
Volver y Cancelar en los recorridos detallados en
`docs/MANUAL_AZAHAR_20261117.md`, incluida una lección realmente completada.
Ese registro amplía la cobertura parcial anterior; no valida todos los modos
ni sustituye las pruebas físicas. Se restauraron los guardados originales.
