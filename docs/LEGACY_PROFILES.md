# Tres perfiles: almacenamiento y migración

## Contrato

`profiles::Manager` gestiona exactamente tres ranuras, con identificadores 0, 1 y
2, nombre ASCII imprimible de 1–24 caracteres (no solo espacios) y avatar 0–39.
La aplicación debe inicializar el índice y ejecutar `migrate()` antes de ofrecer
creación/selección. La inicialización no crea un jugador automáticamente.

Cada jugador tiene un directorio exclusivo devuelto por `path(slot)`:

```
<raiz>/profiles/slot_01/generation_1/
<raiz>/profiles/slot_02/generation_1/
<raiz>/profiles/slot_03/generation_1/
```

Todos los Stores del jugador seleccionado deben apuntar a ese directorio:
perfil, entrenamiento, academia, torneo, biblioteca/estadísticas y opciones.
Los modos futuros deben guardar también bajo esa misma ruta. El catálogo de
retratos y otros recursos estáticos no se duplica.

El índice `profiles/index.dat` contiene `PROFILES1`, marca de migración, ocupación,
generación, avatar, nombre y suma FNV-1a de 32 bits. Está limitado a 1024 bytes.
La suma detecta corrupción accidental; no es una firma criptográfica ni una
medida contra modificación intencional. Identificadores y rutas se construyen
internamente: los nombres del jugador nunca forman parte de una ruta.

`rename()` publica el nombre en el índice, que es la fuente de verdad. Al cargar
la ranura, la aplicación aplica ese nombre a `State.profile.name` y lo guarda en
la siguiente operación normal. Así no requiere una transacción inseparable
entre dos archivos de formatos diferentes. Las partidas archivadas conservan el
nombre que tenían cuando se jugaron. `setAvatar()` modifica solo el índice.

Si un perfil ocupado no puede cargarse desde `profile.dat` ni su respaldo, la
interfaz debe rechazar su selección, informar el problema y conservar sus datos.
No debe tratarlo como un jugador nuevo. El índice no valida todas las partidas
para simplemente listar tres nombres; cada Store mantiene su validador propio.

## Publicación, fallos y eliminación

Cada modificación del índice escribe un temporal, vacía el buffer, sincroniza el
archivo, cierra y comprueba los bytes escritos. Valida además que el índice se
pueda decodificar. Copia el índice válido anterior a un respaldo mediante otro
temporal y publica el nuevo por `rename`. Un fallo reportado antes de publicar
conserva la selección y el contenido anterior del índice principal. El respaldo
se utiliza únicamente si el principal no es válido; si ambos son inválidos se
rechaza la inicialización, sin sobrescribirlos con un índice vacío.

Eliminar desde la interfaz requiere confirmación en la aplicación. El backend
marca la ranura vacía en el índice y **conserva los archivos de la generación
anterior** para recuperación manual. La próxima creación reserva una generación
nueva sin heredar monedas, compras o archivos secundarios del jugador eliminado.
Los directorios incompletos de una operación fallida tampoco se reutilizan.

Esto permite recuperar datos y probar la eliminación sin borrado físico, pero
los archivos retenidos consumen espacio. No hay purga automática. Eliminar una
ranura libera el espacio lógico en el selector, no promete liberar espacio SD.
El número de generaciones está limitado a mil millones; la búsqueda de un nombre
libre intenta como máximo 1024 directorios por operación y falla de forma clara.

Restaurar un respaldo del índice puede recuperar su estado anterior, por ejemplo
un nombre previo o una ranura recientemente eliminada. La propiedad `recovered`
permite informar esta recuperación. Los datos recientes siguen en sus
directorios, aunque un índice anterior no los anuncie. No se promete resistencia
absoluta a cortes de alimentación: no se ha validado durabilidad de metadatos FAT
ni sincronización de directorios en hardware.

## Migración de 0.8.x / 0.9.0

La migración busca `profile.dat` o `profile.dat.bak` en la raíz antigua. El Store
existente debe poder validar uno. Importa al primer slot vacío. Si no hay espacio,
o si ninguno de los archivos principales es válido, falla sin borrar el origen.

Copia los seis grupos conocidos `profile.dat`, `training.dat`, `academy.dat`,
`tournament.dat`, `archive.dat` y `ui.dat`, más sus `.bak`, en una generación aún
no publicada. Cada archivo se limita a 4 MiB, se compara byte a byte tras copiar,
y el perfil copiado debe decodificar exactamente al mismo State que el origen.
Solo después publica la ranura y la marca de migración en el índice.

Los archivos opcionales corruptos se preservan literalmente: la migración no
finge repararlos ni validar su contenido. Sus módulos pueden usar el respaldo y
sus mensajes existentes. Los temporales antiguos no se consideran guardados
confirmados. Las exportaciones PGN anteriores permanecen en la raíz original;
no constituyen progreso y no se redistribuyen entre los perfiles nuevos.

Los originales **nunca se eliminan**. Si una escritura se interrumpe antes de
publicar, el reintento crea otra generación. Una migración confirmada no vuelve a
importarse al reiniciar ni después de eliminar el perfil importado. La ausencia
de datos antiguos también se registra como migración resuelta.

La copia mantiene como máximo un archivo completo de 4 MiB en memoria; la
verificación de escritura usa un bloque fijo de 4096 bytes. Se reutilizan los
límites y validadores existentes de cada formato; no se aumentó la pila global.
No se midió aquí el máximo de RAM del ejecutable completo en una Old 3DS XL.

## Pruebas del backend

`tests/test_profiles.cpp` crea datos sintéticos bajo `/tmp`. Cubre límite de tres,
nombres y avatares, aislamiento de monedas/inventario/estadísticas, generaciones,
retención de eliminación, importación al primer hueco, copia de archivos,
reintentos, idempotencia, respaldo del perfil antiguo, corrupción del índice en
cada truncamiento y cambio de un byte y seis puntos de fallo de escritura para
creación, renombrado, avatar, eliminación e importación.

Comando independiente de host (también se puede integrar en Makefile):

```
clang++ -std=c++17 -O2 -g -Wall -Wextra -pthread -Iinclude \
 source/chess/clock.cpp source/chess/chess.cpp source/ai/search.cpp \
 source/ai/styles.cpp source/ai/bots.cpp source/economy/profile.cpp \
 source/storage/store.cpp source/storage/profiles.cpp tests/test_profiles.cpp \
 -o build/test_profiles
./build/test_profiles
```

Los fallos son inyección de retornos de I/O controlados, no simulación de consumo,
interrupción eléctrica real o suspensión física. La aplicación requiere pruebas
adicionales de selección/cambio de perfil y cancelación de trabajos de IA.

## Extensiones de 0.9.2 Preview

La migración reconoce también `progression.dat` y `openings.dat` con sus
respaldos si existen en una raíz anterior. Sus Stores usan siempre la ruta del
perfil seleccionado. El perfil formato 4 añade un marcador acumulado para los
premios Legacy; el lector conserva los formatos 1/2/3. Para regresar a un
ejecutable antiguo se necesita el respaldo previo de guardados.
