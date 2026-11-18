# Rendimiento de biblioteca — medición 0.9.0

## Qué se midió realmente

Microbenchmark reproducible en `tools/benchmark_update.cpp`, ejecutado el
2026-11-16 en macOS 27.0, Apple Silicon arm64, Apple Clang 21 (`-O2`). Se usa un
estado legal de apertura española de **16 medios movimientos**, incluidos ambos
enroques, seguido de un acuerdo de tablas. Se crean treinta registros de prueba
con esa misma partida, distintos IDs, rivales/colores/niveles y nombre `Benchmark`.
No son treinta partidas históricas del usuario ni un torneo de fuerza del motor.

Cada operación tiene un calentamiento y doce muestras mediante
`std::chrono::steady_clock`. Se informan mínimo, media y máximo de tiempo
**transcurrido**: las operaciones con archivos incluyen I/O y fsync, no solo CPU.
Los resultados usan código/datos y cachés de archivos calientes, en un SSD de Mac.
No se extrapolan a una tarjeta SD ni a CPU ARM11.

El benchmark crea una carpeta temporal exclusiva bajo `/tmp`, la elimina al
terminar y nunca carga ni modifica `profile.dat` o el guardado real del usuario.

## Resultados después de la optimización

| Operación, treinta partidas salvo indicación | Mínimo ms | Media ms | Máximo ms |
|---|---:|---:|---:|
| Construir y validar los 30 registros | 1.457 | 1.486 | 1.517 |
| Serializar/encode | 0.045 | 0.046 | 0.048 |
| Decode y validación semántica | 1.500 | 1.507 | 1.514 |
| Cargar desde archivo y validar | 1.513 | 1.522 | 1.547 |
| Guardar, conservar respaldo y confirmar | 3.199 | 3.271 | 3.504 |
| Reproducir última partida, ply 0 | 0.051 | 0.051 | 0.051 |
| Reproducir última partida, ply 8 | 0.071 | 0.073 | 0.092 |
| Reproducir última partida, ply 16/resultado | 0.049 | 0.050 | 0.050 |
| Construir PGN de la última partida | 0.066 | 0.067 | 0.072 |
| Exportar ese PGN con fsync | 0.162 | 0.180 | 0.198 |

El archivo `archive.dat` de esta prueba ocupa **8981 bytes**. Exportar PGN no está
incluido en la medida `pgn_latest`, sino en su fila independiente.

El proceso del microbenchmark alcanzó un RSS máximo registrado por getrusage de
**2686976 bytes** en esta ejecución. Es el proceso de benchmark sin renderizador,
audio ni búsqueda en ejecución; **no es la memoria máxima del juego**. El valor
incluye código, librerías y otros recursos del proceso, no solo el archivo.

## Cambio autorizado y comparación antes/después

`Store::save` validaba tanto el principal como el respaldo antes de guardar. Si
el principal es válido, será el nuevo respaldo y no se utiliza el respaldo viejo
para tomar ninguna decisión de recuperación. Se cambió únicamente esa lectura:

```cpp
const bool mainValid = readFile(path, old);
const bool backupValid = !mainValid && readFile(backup, old);
```

Se conserva la validación de respaldo cuando el principal está corrupto o falta.
Se conservan también validación del estado nuevo, checksum, fsync, conservación
atómica práctica y rechazo de sobrescritura cuando ambos existentes son corruptos.

Dos binarios con el mismo benchmark se ejecutaron consecutivamente, primero con
la implementación anterior conservada en `build/update-090/archive-before-shortcircuit.cpp`,
y después con la nueva. Doce muestras por operación, misma partida y archivo.

| Guardado de 30 partidas | Mínimo ms | Media ms | Máximo ms |
|---|---:|---:|---:|
| Antes, validando ambos archivos siempre | 4.684 | 4.779 | 4.894 |
| Después, respaldo solo cuando hace falta | 3.199 | 3.271 | 3.504 |

La diferencia media observada fue **1.508 ms, aproximadamente 31.6% menos** para
esa operación y esa ejecución en Mac. La carga permaneció en 1.522 ms medios en
ambos binarios; decode estuvo aproximadamente en 1.5 ms. Eso es coherente con
eliminar una reproducción/validación redundante de treinta partidas.

No se presenta ese porcentaje como una garantía en otra computadora, en consola
o para historiales más largos. No se hizo un estudio estadístico de carga del
sistema ni se midió consumo energético.

Registros locales completos:

- `build/update-090/benchmark_archive_paired_before.log`
- `build/update-090/benchmark_archive_paired_after.log`
- La primera muestra exploratoria sin modificar está en
  `build/update-090/benchmark_archive_before.log` y dio4.491 ms medios de guardado.
  La variación con el par posterior ilustra por qué no se deben prometer tiempos
  exactos usando una sola ejecución.

Las regresiones de archivo se ejecutaron después del cambio, incluidas todas las
inyecciones de fallo Write/Flush/Sync/Close/Backup/Commit. Se añadió el caso de
principal válido y respaldo ilegible: el guardado funciona, y el respaldo termina
siendo la copia válida anterior del principal. También pasaron ASan+UBSan y la
compilación ARMv6K del módulo cambiado.

## Tamaños de estructuras y almacenamiento

Los tamaños ARM se obtuvieron **del compilador devkitARM 16.1.0**, sin ejecutar
hardware. `ARCHIVE_LAYOUT_ONLY` emite símbolos cuyo tamaño es `sizeof(T)`; sus
valores se leyeron con `arm-none-eabi-nm -S`. Son datos de ABI estática reales,
no una estimación de memoria dinámica de la consola.

| Tipo | Host arm64/libc++, bytes | Target ARMv6K/libstdc++, bytes |
|---|---:|---:|
| `archive::Entry` | 200 | 184 |
| `archive::State` | 1152 | 1144 |
| `archive::Statistics` | 1096 | 1096 |
| `economy::Session` | 248 | 216 |
| `chess::Game` | 192 | 160 |
| `chess::Move` | 12 | 12 |
| `std::string` | 24 | 24 |
| `ui::App`, snapshot 0.9.0 | 4944 | 4568 |

Estas estructuras contienen punteros/vectores/cadenas: `sizeof` no incluye los
buffers dinámicos a los que apuntan. En la prueba de 30×16 plies se observó:

- Capacidad del vector de entradas:32 elementos. Sus objetos ocupaban6400 bytes
  en el host; con el mismo factor de capacidad serían5888 bytes de objetos ARM.
- Capacidad agregada de movimientos: **5760 bytes**, tanto host como target porque
  cada `Move` mide 12 bytes en ambos.
- Metadatos:4800 caracteres lógicos entre FEN, nombre, motivo y fecha. Parte puede
  estar en el almacenamiento corto de los objetos string ya contado por sizeof;
  no se suma otra vez como si todos fueran asignaciones independientes de heap.
- Una partida `Game` completa de esa muestra guardaba17 claves de posición.
  La capacidad del vector de strings más sus buffers alcanzaba2672 bytes en el
  host. Mantener treinta historiales completos requeriría **80160 bytes solo para
  esas posiciones**, además de movimientos, posiciones actuales y demás campos.
  El archivo compacto no conserva esas claves y las reconstruye al reproducir.

Esto justifica el formato compacto sin afirmar un total exacto de RAM: faltan
sobrecarga del asignador, capacidad de strings, objetos temporales y dependencias.

### Caso límite de historiales largos

El límite defensivo permite16384 medios movimientos por entrada. Treinta vectores
con exactamente esa capacidad ocuparían, solo en movimientos:

```text
16384 ×30 ×12 =5898240 bytes =5.625 MiB ≈5.9 MB decimales
```

La capacidad real del vector puede ser mayor que su longitud según su historia
de asignaciones. El codec limita el archivo a 4 MiB, pero **ese límite de bytes
serializados no limita la RAM a 4 MiB**. Los movimientos ASCII son más compactos
que `Move`; durante carga/validación también existen buffers de archivo, streams,
Estado temporal y una partida reconstruida con claves de repetición.

No se ejecutó ese extremo con una partida legal de 16384 plies ni se midió en una
Old 3DS. Su coste CPU, pico de heap, latencia de guardado y comportamiento con SD
física siguen pendientes. No se afirma que siempre quepa o responda fluidamente
por el solo hecho de que la muestra corta funcione.

## Stack principal

Se comparó `source/main.cpp` actual con el guardado en
`build/update-090/baseline-082.zip`. Ambos declaran exactamente:

```cpp
extern "C" { u32 __stacksize__ = 256 * 1024; }
```

**La reserva del stack principal sigue siendo 256 KiB.** No se aumentó para esta
biblioteca. Eso no significa que el uso real del stack sea idéntico: `ui::App`,
que se construye en main, pasó de 2296 bytes ARM en 0.8.2 a 4568 bytes en el snapshot
0.9.0 medido (+2272 bytes, también por otros módulos nuevos). En host pasó de 2544
a 4944 bytes. Los vectores de partidas y movimientos siguen en heap.

La compilación ARM de `archive.cpp` con `-fstack-usage` informó como mayor frame
propio 2016 bytes para decode y 1360 bytes para `Store::save`; otros ejemplos fueron
856 bytes para reconstruir sesión y 448 bytes para replay. Son frames por función,
no el máximo de su cadena de llamadas: **no sustituyen medir el stack de la
aplicación completa en consola**. La búsqueda y los codecs llamados añaden sus
propios frames.

## Reproducir

Desde la raíz del proyecto, sin modificar el Makefile:

```sh
mkdir -p build/update-090
clang++ -std=c++17 -O2 -g -Wall -Wextra -pthread \
  -Iinclude -Iassets/generated -Iassets/portraits \
  source/chess/chess.cpp source/chess/clock.cpp source/ai/bots.cpp \
  source/economy/profile.cpp source/storage/store.cpp \
  source/archive/archive.cpp tools/benchmark_update.cpp \
  -o build/update-090/benchmark_update
./build/update-090/benchmark_update 12
```

El código usa 12 repeticiones si no se pasa argumento y acepta 3–100. Para obtener
el layout ARM sin enlazar ni ejecutar la aplicación:

```sh
/opt/devkitpro/devkitARM/bin/arm-none-eabi-g++ \
  -D__3DS__ -DARCHIVE_LAYOUT_ONLY -Iinclude -Iassets/generated \
  -Iassets/portraits -I/opt/devkitpro/libctru/include \
  -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft \
  -std=gnu++17 -O2 -fno-exceptions -fno-rtti \
  -c tools/benchmark_update.cpp -o build/update-090/archive-layout-arm.o
/opt/devkitpro/devkitARM/bin/arm-none-eabi-nm -S --size-sort \
  build/update-090/archive-layout-arm.o
```

No se hicieron optimizaciones de caché de partidas validadas ni se eliminó la
validación semántica de entradas para acelerar la interfaz. Si una prueba física
revela pausas, el siguiente paso sería medirlas allí antes de diseñar cachés o
validación incremental que puedan complicar la recuperación.
