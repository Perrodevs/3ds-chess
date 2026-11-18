# Biblioteca de partidas y estadísticas nuevas — 0.9.0

## Alcance y límites

El módulo nuevo `include/archive/archive.hpp` / `source/archive/archive.cpp` guarda
**hasta treinta partidas terminadas**, con metadatos suficientes para reproducir
sus jugadas legales y exportarlas como PGN. Es independiente de `profile.dat` y
no modifica el saldo, los resultados antiguos ni la ranura de partida actual.

La biblioteca es compacta: cada entrada conserva movimientos y FEN, en lugar de
mantener treinta objetos `Game` con todas sus claves de repetición en RAM. El
objeto completo se reconstruye solo para validar o abrir una partida.

No se declara una prueba física nueva: se ejecutaron pruebas nativas, sanitizadores
y compilación ARMv6K de este módulo. La integración gráfica corresponde a App.

## API para la interfaz

```cpp
archive::Store store(directory); // usa directory/archive.dat
archive::State data;
if (!store.load(data)) {
    if (store.corrupt) {
        // Mostrar error; no reinicializar ni sobrescribir los archivos existentes.
    } else if (!store.exists) {
        archive::initialize(data, oldProfile.paidId);
        store.save(data);
    }
}
```

`Store` expone `exists`, `corrupt` y `recovered`. La carga no cambia el argumento
si ningún archivo es válido. Si existe un principal o respaldo pero ambos fallan,
se marca `corrupt=true` y **save también rechaza sobrescribirlos**. Un principal
inválido con respaldo válido permite recuperarse sin conservar el archivo corrupto
como nuevo respaldo.

Después de confirmar el guardado de una sesión terminada:

```cpp
auto previous = data;
auto result = archive::record(data, session, playerName, "2026-11-16");
// date puede ser "" cuando no existe una fecha fiable.
if (result == archive::RecordResult::Full) {
    // Aviso explícito: biblioteca llena. Ninguna partida se sobrescribe.
}
if (result != archive::RecordResult::Invalid && !store.save(data)) {
    data = previous; // avisar de fallo SD; no confirmar el cambio en pantalla.
}
```

Resultados: `Added`, `AlreadyRecorded`, `Full`, `Invalid`. **Full puede actualizar
las estadísticas de la sesión nueva** aunque no pueda conservar su historial. Es
necesario persistir ese estado igualmente. La biblioteca no borra automáticamente
la partida más antigua ni cambia su capacidad para ocultar un fallo.

`erase(data, id)` elimina una entrada **solo en memoria**. La interfaz confirma
primero la acción y luego llama a `save`, restaurando el estado anterior si falla.
Eliminar una entrada no elimina estadísticas acumuladas ni permite cobrarlas de
nuevo. Tampoco borra un PGN ya exportado, que es una salida independiente del juego.

Las entradas se almacenan en orden creciente de ID. Se pueden recorrer al revés
para mostrar primero las más recientes. Los campos públicos son:

- `id`, `bot` (0–39), `human` (+1 blancas / -1 negras), `mode` y `level` (0–5).
- `clock`: preset y tiempos finales de ambos jugadores.
- `initialFen`, `finalFen`, `moves`, `result` y `reason`.
- `playerName` y `date` (ISO `YYYY-MM-DD`, o cadena vacía si se desconoce).

Métodos de una entrada:

```cpp
chess::Game replay;
entry.replay(ply, replay); // 0..entry.moves.size(), sin alterar la partida actual.
economy::Session final;
entry.session(final);     // reconstrucción validada del estado final completo.
auto text = entry.pgn();  // vacío si los datos no son válidos.
store.exportPgn(entry);   // directory/archive-ID.pgn
```

La reproducción parcial restaura el prefijo legal. En el último ply también se
restaura la adjudicación externa, como rendición, acuerdo de tablas o caída de
bandera. El PGN tiene el rival, colores y resultado correctos, `TimeControl`, y
`Date`. Una fecha desconocida se expresa como `????.??.??`; no se inventa una.
La validación de fechas contempla días por mes y años bisiestos.

## Estadísticas sin inventar el pasado

Al crear el archivo por primera vez, `initialize(state, oldProfile.paidId)` fija
un corte de migración. No convierte los totales antiguos del perfil en supuestas
partidas contra rivales concretos. Una partida antigua real aún puede archivarse,
pero un ID igual o anterior al corte **no añade estadísticas nuevas**.

`state.statistics` contiene:

- `total`: `wins`, `losses`, `draws`, y método `played()`.
- `byOpponent[40]`: los mismos contadores por ID persistente de rival.
- `byColor[2]`: índice 0 jugando con blancas, 1 con negras.
- `currentStreak` y `bestStreak`: rachas de victorias competitivas.
- `winsByLevel[6]`: victorias para los seis presupuestos del motor.

Solo `mode==0` (Jugador vs IA competitivo) suma contadores y modifica rachas.
Las tablas o derrotas competitivas cortan la racha; los modos local y libre no
la cambian. Las rendiciones competitivas sí son resultados estadísticos aunque
no reciban monedas por las reglas antipremio temprano del perfil. Este módulo
no concede monedas ni modifica los premios existentes.

`lastStatisticsId` evita volver a sumar una sesión ya procesada. Avanza también
al observar una sesión local/libre nueva, sin añadirla a los totales competitivos.
`lastArchivedId` es una marca independiente para evitar volver a insertar un ID
archivado, incluso si se eliminó. Al llenarse la biblioteca, esa última marca no
avanza: tras eliminar una entrada se puede volver a intentar archivar la última
sesión rechazada sin duplicar su estadística. Si ya se archivó otra sesión con ID
posterior, la marca impide recuperar IDs anteriores; no se promete un archivo
histórico de sesiones rechazadas que ya no estén disponibles.

Estas garantías asumen que los IDs de sesión siguen creciendo, como en el perfil
actual. Restaurar una copia vieja restaura sus estadísticas y marcas juntas. No
se pretende autenticar datos frente a edición intencionada ni fusionar varias
instancias del juego.

## Formato y validación

Archivo autoritativo: `archive.dat`. Respaldo: `archive.dat.bak`. Temporal no
confirmado: `archive.dat.tmp`.

Cabecera ASCII:

```text
3DSARCHIVE 1 CHECKSUM_FNV1A32
```

El checksum cubre todos los bytes posteriores a la primera nueva línea. El cuerpo
contiene inicialización/corte/marcas de ID, contadores agregados, 40 contadores por
rival, dos por color, rachas, seis victorias por nivel y las entradas compactas.
Cadenas mediante `std::quoted`, movimientos mediante UCI. Versión 1 tiene cuarenta
rivales persistentes: ampliar esa tabla requiere una migración explícita.

Límites defensivos: treinta entradas, 16384 medios movimientos por entrada, ID
máximo 999999999, archivo máximo de 4 MiB. Se valida el checksum, versión,
longitudes, rangos, orden e identidad de IDs, coherencia de sumas W/D/L por rival
y color, victorias por nivel, límites de rachas y corte de migración.

Cada entrada se reconstruye mediante el **mismo codec autoritativo del perfil**.
Así se comprueban FEN inicial, todos los movimientos legales, resultado/motivo,
repeticiones reclamadas, relojes agotados e incrementos. La posición reconstruida
debe coincidir exactamente con `finalFen`. Un checksum correcto con una jugada
ilegal o estadísticas incompatibles sigue siendo rechazado.

El guardado comprueba la semántica antes de abrir el temporal; luego escribe,
`fflush`, `fsync`, cierra, conserva el principal válido como respaldo y renombra
el temporal. Si el principal previo es corrupto y el respaldo es válido, ese
respaldo se conserva. No se cargan temporales como si fueran confirmados.

El protocolo reduce corrupción accidental pero no demuestra durabilidad absoluta
en FAT bajo corte eléctrico: no se ha hecho esa prueba con una SD física ni se
sincroniza el directorio. La recuperación desde respaldo puede perder la última
transacción completa. No se descartan ambos archivos corruptos automáticamente.

## Pruebas ejecutadas

`tests/test_archive.cpp` comprueba:

- Corte de migración, ausencia de estadísticas históricas inventadas y archivo de
  una sesión antigua real sin sumarla a contadores nuevos.
- Rival ID39, colores, victorias/derrotas/tablas, seis niveles, rachas y exclusión
  de modos local/libre.
- Repetir el mismo ID no duplica entradas ni resultados.
- Treinta entradas, rechazo explícito de la número31, ninguna sustitución,
  estadísticas de Full, reintento tras liberar espacio y marcas tras borrar.
- Reproducción desde ply0, prefijo intermedio y mate final; exportación SAN/PGN.
- Fechas válidas, desconocidas e inválidas, incluidos años bisiestos.
- Roundtrip, truncamiento, checksum y casos semánticamente inválidos con checksum
  recalculado: movimiento ilegal, FEN final falso, ID duplicado, color fuera de
  rango, sumas y rachas imposibles.
- Carga inexistente, recuperación de respaldo y **rechazo de sobrescritura cuando
  ambos archivos existentes están corruptos**.
- Fallos deterministas Write, Flush, Sync, Close, Backup y Commit, aislados en una
  carpeta temporal; las rutas normales usan `SaveFault::None`.
- Estado terminal por tiempo agotado y metadatos del reloj.

Se compiló `archive.cpp` para ARMv6K con devkitARM y `-fno-exceptions`. Las pruebas
nativas se ejecutaron con éxito, y también se ejecutó el binario de pruebas con
AddressSanitizer y UndefinedBehaviorSanitizer. No se modificó el perfil antiguo,
los assets, App ni el Makefile desde este submódulo.
