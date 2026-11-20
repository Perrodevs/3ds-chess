# 3DS Chess 1.4.0 RC2

La rama local `fix/stability-quality-update` parte de `860a6b9` (1.4.0 RC1).
El repositorio principal y el directorio de Antigravity se conservaron. La
entrega es una candidata para pruebas físicas y sus paquetes son de uso local.

## Perfiles y diagnóstico

En Azahar 2126.2 la primera compilación ARM mostró FAIL en la prueba de SD y SKIP
en las pruebas de perfiles. El log registró `CHECK_SD create_probe_directory
result=88`. En el newlib enlazado para ARM, `88` es ENOSYS. El desensamblado
mostró que la ruta de `mkdtemp` en `_gettemp` fija ese error directamente. El
juego bloqueaba correctamente el arranque tras un fallo de SD, pero esta vez el
sondeo fallaba por su propia llamada al sistema.

Los sondeos usan `mkdir` con nombres delimitados por sesión, hasta 16 intentos
por colisión y limpieza restringida a sus propios archivos. En una SD emulada
aislada, los tres checks de SD, creación aislada y lectura de perfiles pasaron
tras el cambio. La primera ejecución que reprodujo el fallo escribió en el log
habitual de Azahar, antes de que se inicializaran perfiles. Las pruebas de creación
y juego posteriores usaron un directorio de datos separado.

En RC1, «Crear perfil nuevo» guardaba un nombre automático sin activar el
teclado. RC2 abre el applet de nombre, valida el texto y ofrece una segunda
acción con el nombre sugerido. Cancelar conserva la ranura. El progreso no se
comparte entre ranuras. Los errores registran código y etapa, con el texto del
jugador omitido. La prueba de host bloquea la ruta de la segunda ranura, comprueba
el código y reintenta tras quitar el bloqueo sintético.

El diagnóstico ya ofrecía pruebas rápidas y extendidas, consola inicial y
acceso desde Developer. El journal ya asignaba un identificador por sesión,
rotaba entre dos registros de hasta 64 KiB y retenía eventos graves en memoria.
Se añadió el identificador a la vista de build, un evento para fallos de
rotación/apertura/escritura del propio journal y el caso de un guardado legado
con respaldo y sin archivo primario. Los sondeos usan directorios temporales y
la inspección de guardados es de lectura.

La causa detectada aplica al binario ARM examinado. Todavía falta confirmar si
también explica el incidente concreto reportado en la consola física.

## Diálogos y trofeos

El render elegía el texto superior otra vez en cada cuadro. El jaque de posición
y el estado instantáneo del hilo podían sustituir rápidamente la reacción.
La partida ahora envía eventos a un panel inferior con retrato y expresión.
La frase visible dura 3.4 segundos de juego y una reacción relevante puede
esperar en una cola de capacidad uno. Un resultado reemplaza los eventos
pendientes de la partida. La pausa conserva el tiempo de la frase. Existe una
preferencia persistente para reducir capturas, jaques y comentarios de búsqueda.
`UI03` añade esa opción. El lector sigue aceptando `UI01` y `UI02`. Los formatos
de perfil, partida y progreso se conservan. Un ejecutable anterior puede ignorar
una preferencia `UI03`.

La Sala de trofeos conserva sus 20 trofeos. Su nueva tabla consulta los archivos
de cada perfil existente, ordena por trofeos ganados y desempata por ranura.
Muestra división de Carrera, torneos ganados, mejor racha de Supervivencia y
trofeos destacados. El lector omite ranuras vacías o con guardado principal y
respaldo ilegibles y señala las fuentes parciales.

## Rendimiento y alcance

El dibujado del tablero calculaba todas las jugadas legales cada cuadro con
pieza seleccionada y buscaba destinos entre ellas para cada casilla. Se guarda
una máscara de destinos hasta que cambian la posición o la selección. El motor
sigue siendo la fuente de legalidad.

Microbenchmark en Mac Apple Silicon, clang++ `-O2`, cinco tandas de 90,000 cuadros
sintéticos con un cambio de posición o selección cada 60 cuadros. El algoritmo
anterior tardó 81.75–83.21 ms por tanda. El actual tardó 0.88–0.94 ms. La
medición cubre el cálculo de resaltado. El tiempo de cuadros completos y el
rendimiento de GPU, RAM y batería en consola quedan pendientes.
La compilación de inventario ARM midió 6,456 → 6,832 bytes para `App` (+376 bytes
por estado de diálogos, tabla y máscara). `Search` se mantuvo en 17,248 bytes,
`Worker` en 256 bytes y `MoveList` en 3,076. Son tamaños de tipos, sin sus
asignaciones dinámicas. Las reservas de pila se conservan.

Se revisaron cancelación y límites del motor, la carga y liberación de NDSP,
la ruta de guardado y los formatos de respaldo. Los seis niveles y los límites de
búsqueda se conservan. Los casos de carga de atlas y fallos de audio están en
las pruebas con interfaces nativas simuladas. La validación auditiva sigue pendiente.

Para reproducir la medición:

```sh
mkdir -p build
clang++ -std=c++17 -O2 -Iinclude source/chess/chess.cpp tools/benchmark_legal_destinations.cpp -o build/benchmark_legal_destinations
./build/benchmark_legal_destinations
```

## Verificación

La referencia `860a6b9` pasó `make -j10 test` con los recursos generados locales.
En Azahar se observó el teclado de perfiles, la creación y la activación tras
reiniciar, el saludo inferior, el primer intercambio e4/e5, la tabla de trofeos
y la vuelta al menú. NDSP mostró WARN en ese entorno. Las demás comprobaciones
de consola figuran en [la lista de pruebas físicas](PHYSICAL_TESTS_140_RC2.md).

Las regresiones comprueban navegación física simulada y táctil, las tres ranuras,
reapertura e independencia de datos, fallos de escritura y recuperación,
visibilidad de diálogos ES/EN, enroque y captura al paso para el resaltado,
vista de trofeos y desbloqueo de Elon con mates válidos tras vencer a Byte y
Jensen. Una victoria forzada con Developer queda fuera de esa progresión.

| Verificación | Resultado |
| --- | --- |
| `make -j8 test test-target differential` | 48 ejecutables de prueba y verificaciones Python pasaron. Perft inicial = 197,281 a profundidad 4. Diferencial = 3,012 posiciones y 329 historiales. ELF y 39 objetos ARMv6K, barrera CP15 y sin requisito Thumb-2. |
| `make -j3 sanitize-diagnostics sanitize-learning sanitize-storage sanitize-progression sanitize-native` | Seis ejecutables con AddressSanitizer y UndefinedBehaviorSanitizer, sin hallazgos. Incluye límites y fallos simulados de Citro2D/NDSP, no servicios físicos de la consola. |
| Pruebas dirigidas finales de aprendizaje, clasificación y preferencias | Pasaron con pensamiento de IA, ambos resultados, final de puzzle, una a tres ranuras y lectura del formato UI02. |
| Azahar 2126.2 | El bloqueo SD se reprodujo antes del cambio y dejó de reproducirse con el sondeo corregido. La sesión de emulador usó el código funcional de RC2 antes de actualizar su etiqueta de versión. |
| Old Nintendo 3DS XL | Pendiente. Incluye suspensión, audio, respuesta de controles y guardados de la SD física. |

La preparación inicial de la referencia necesitó los recursos generados
ignorados por Git. El primer reintento usó una ruta de Python con espacios
sin escaparlos. Con los recursos locales y la ruta relativa de Python, la
referencia pasó. Durante el desarrollo también se corrigieron datos de prueba
de navegación, FEN e idioma antes de volver a ejecutar los escenarios.

Los ZIP para SD ES y EN comparten el mismo árbol de guardado. El paquete de
fuentes y los binarios conservan la restricción de uso local indicada en
[el inventario de derechos](ASSET_RIGHTS.md).
