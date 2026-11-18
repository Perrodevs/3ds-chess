# Relojes opcionales

El valor predeterminado es **Sin límite**. No hay cuenta regresiva hasta que el
jugador elige expresamente otro control antes de comenzar una partida.

| ID estable | Control | Tiempo por jugador | Incremento por jugada |
|---|---|---:|---:|
| 0 | Sin límite | Sin reloj | Ninguno |
| 1 | 3 min | 03:00 | Ninguno |
| 2 | 5 min | 05:00 | Ninguno |
| 3 | 10 min | 10:00 | Ninguno |
| 4 | 5 min + 3 s | 05:00 | 3 segundos |

La lógica independiente de gráficos está en `chess/clock.hpp` y `clock.cpp`.
Cada sesión guarda el control y los milisegundos restantes de blancas/doradas
(índice0) y negras/azules (índice1). Las partidas de versiones anteriores migran
sin límite; el tablero, historial y otras reglas no se alteran por elegir reloj.

## Semántica

- `State::reset` inicializa ambos tiempos desde un control válido. Si recibe un
  ID inválido se recupera como Sin límite. El cargador rechaza datos inválidos,
  en lugar de usar esta recuperación para ocultar corrupción.
- `advance(side,elapsedMs)` consume exclusivamente el reloj del jugador indicado;
  tiempo negativo no añade saldo. Al llegar exactamente a cero devuelve caída
  de bandera y satura en cero, incluso ante un delta de64bits extremadamente grande.
- `moved(side)` añade el incremento solo después de una jugada legal completada
  antes de agotar tiempo. Una pieza seleccionada, destino ilegal o promoción
  todavía pendiente no cuentan como jugada terminada. El incremento no revive
  un reloj que ya llegó a cero.
- El reloj es independiente del número de cuadros y usa tiempo transcurrido,
  no una resta fija por frame. La interfaz debe cobrar el intervalo antes de
  aceptar una jugada o el resultado del bot. El tiempo de cálculo de la IA
  corresponde a su propio turno, igual que el tiempo del jugador humano.
- El formato visual redondea segundos hacia arriba: un milisegundo restante se
  muestra00:01 y solo cero se muestra00:00. No se anuncia visualmente una caída
  antes de que ocurra. Los valores se calculan internamente en milisegundos.

La validación impone valores entre cero y24horas por lado; ese margen supera
ampliamente la acumulación de incrementos de una partida legal en estos controles.
Un control Sin límite requiere ambos campos en cero. El cargador añade coherencia
con el resultado, para que una partida en curso no contenga un reloj agotado.

## Pérdida por tiempo y excepción de tablas

La regla de referencia es el [artículo6.9 de las Leyes FIDE](https://handbook.fide.com/chapter/e012023):
se pierde por tiempo salvo que el rival no pueda dar mate mediante ninguna
secuencia legal. No basta preguntar si el rival puede **forzar** mate. Por
eso rey y dos caballos puede ganar por tiempo, aunque no fuerce mate contra rey.

`canPossiblyMate(position,color)` identifica imposibilidad material de un lado,
no simplemente falta de material global de ambos:

- Un rey solo nunca puede ganar por tiempo.
- Un único caballo no puede dar mate si el rival solo tiene rey o rey y damas.
  Peones, torres u otras piezas rivales pueden servir de bloqueos en un mate
  cooperativo, así que no se declaran automáticamente tablas en esos casos.
- Cuando el lado solo tiene alfiles, no puede dar mate si todos los alfiles de
  ambos lados usan el mismo color de casilla y no hay peones ni caballos en el
  tablero. Dos alfiles del mismo color por promoción no equivalen a la pareja
  habitual de alfiles de colores opuestos.
- Peones, torre, dama, alfil+caballo, dos caballos o alfiles de ambos colores no
  permiten probar imposibilidad mediante este análisis material conservador.

La [documentación de python-chess sobre material insuficiente](https://python-chess.readthedocs.io/en/latest/core.html#chess.Board.has_insufficient_material)
sirvió como referencia independiente para verificar estos casos de reglas; no
se integra ni copia su implementación GPL en el ejecutable C++.

**Límite conocido:** esto no resuelve todas las posiciones muertas geométricas,
peones bloqueados ni secuencias forzadas. Devuelve “puede ser posible” en casos
no demostrados imposibles por material. Es la misma limitación general ya
advertida para posiciones muertas del motor; no se presenta como un solucionador
exhaustivo FIDE de cualquier posición arbitraria.

`adjudicateTimeout` conserva cualquier resultado existente y primero comprueba
mate, ahogado y tablas automáticas ya alcanzados. Solo puede caer el reloj del
jugador que tiene el turno. Un mate legal ya completado no se reemplaza por
un resultado de tiempo posterior. Motivos de reloj persistidos:
`Tiempo agotado` y `Tiempo agotado: tablas`.

## Pruebas ejecutadas

`tests/test_clock.cpp` cubre controles, valor predeterminado, cobro de un solo
lado, incremento exacto, cero sin resurrección, límites de entero, formato visual,
material de un lado, dos caballos, excepciones de caballo/alfiles, resultados por
tiempo, idempotencia y prioridad de mate/ahogado existentes. Se ejecutó en Mac
con clang y con AddressSanitizer/UndefinedBehaviorSanitizer. `clock.cpp` también
se compiló para ARMv6K con devkitARM. Estas pruebas no equivalen a medir tiempo
ni suspensión en una consola física.

## Integración en el juego

Nueva partida → Reloj recorre los cinco controles con A/táctil o izquierda/derecha.
El valor de un perfil nuevo es Sin límite y las partidas v1/v2 también migran sin
límite. La preferencia elegida se recuerda para la siguiente partida. Cambiarla
no altera el reloj de una partida ya comenzada.

La pantalla inferior muestra ambos relojes; un borde resalta al jugador de turno
y el texto se vuelve rojo con 10 segundos o menos. El historial conserva las tres
jugadas completas más recientes cuando hay relojes y cuatro cuando no hay límite,
reservando espacio para capturas.

La aplicación usa steady_clock, conserva fracciones de milisegundo entre cuadros
y sincroniza antes de aceptar movimientos o reclamaciones de la IA. El tiempo
de promoción pertenece al mismo jugador hasta que confirma la pieza. START,
menús de pausa, guardar al menú y suspensión detienen los relojes; cargar no cobra
tiempo offline. Los hooks APT de restauración reinician la referencia temporal.
La escritura SD de la propia aplicación no consume tiempo de ninguno.

La búsqueda de la IA se limita también con su tiempo restante y un margen de
reserva; una respuesta que llega después de caer bandera no puede aplicarse.
Al agotar tiempo se cancela y une el hilo, adjudica, liquida y guarda el resultado.
Si falla la SD, la partida sigue terminada en memoria y no se expone el premio no
guardado; una escritura posterior puede reintentarlo sin duplicarlo.

Pruebas de integración: `tests/test_clock_ui.cpp`, con transcurso inyectado
determinista y una comprobación de tiempo real para evitar perder fracciones por
frame. Cubren opciones/default, incremento, turno, promoción, pausa, carga offline,
suspensión/restauración, límite exacto, IA atrasada, tablas por material y fallo SD.
Se generaron vistas de ambas pantallas con el renderizador de Mac. La consola
física y la suspensión real del hardware siguen pendientes.

La prueba de fracciones usa ahora una fuente monotónica inyectada:20pasos de1250microsegundos cobran exactamente25ms, sin esperas físicas.
