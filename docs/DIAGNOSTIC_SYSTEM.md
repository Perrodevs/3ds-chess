# 3DS Chess Diagnostic System

Versión 1.4.0 RC1. Base 1.3.0 RC1, commit 277d936, rama
`feature/diagnostic-system`. La versión se compiló para ARMv6K. La creación
de perfiles en la Old 3DS XL necesita verificación física.

## Hallazgo sobre perfiles

La creación de una ranura vacía usaba un nombre predeterminado. El teclado
solo intervenía al renombrar un perfil ya activado. El selector combinaba la
lectura del índice y la migración de una partida antigua. Cualquier fallo de
migración impedía usar incluso un índice sano. Reprodujimos ese caso con un
`profile.dat` antiguo inválido. Ahora el selector permite crear una ranura
separada y conserva el archivo antiguo para recuperar sus datos. Un índice
dañado sin respaldo válido sigue bloqueando el selector.

La escritura normal valida los datos, escribe un temporal, hace flush y sync,
lo cierra, lo vuelve a abrir y compara el contenido antes de publicar. Si hay
archivos existentes y ninguno es legible, la escritura se bloquea. Las etapas de
creación tienen códigos distintos para validación, reserva, guardado,
verificación, publicación y activación. El nombre predeterminado y las reglas
de nombre se mantienen. El fallo exacto reportado en la consola física sigue
sin confirmarse.

## Arranque y Developer

La pantalla inferior ejecuta diez pasos y muestra PASS, FAIL, WARN o SKIP.
A continúa cuando pasa la prueba SD, o la reintenta si falla. START sale de
la consola inicial. Un fallo del índice o de un perfil ya guardado también se
muestra en el diagnóstico. La aplicación conserva sus propios controles de
escritura para esos archivos.

- Gráficos: resultado de inicializar Citro2D, targets y cargar texturas.
- Ajedrez: FEN inicial, ida y vuelta de codificación y 20 jugadas legales.
- IA: evaluación de ventaja material y configuración de los seis presupuestos.
- Audio: inicialización NDSP y asignación de buffers. Se permite jugar en silencio.
- Idiomas: catálogos no vacíos y claves ordenadas y únicas, idioma global y
  lectura de preferencias existentes por perfil.
- Rivales: recuento de la compilación, identidad por índice, referencias y límites
  del atlas, niveles y nueve eventos de diálogo por rival.
- SD: directorio temporal exclusivo, escritura, sync, lectura y comparación,
  cierre y limpieza. En ARM registra el último resultado FAT si la consulta
  responde. Ese resultado puede ser previo al error observado.
- Perfiles: creación, publicación y recarga de una ranura de prueba bajo otro
  directorio, limpieza y validación de perfiles reales en lectura.
- IA ampliada: perft 2 de referencia (400), creación de hilo, respuesta legal y
  cancelación de un segundo trabajo. B cancela. El buscador tiene 15 ms de
  presupuesto temporal y el supervisor considera 500 ms como límite diagnóstico.

Activa Developer desde Opciones y presiona SELECT en una partida para abrir su
menú. Elige **Diagnósticos**. Se puede repetir el arranque, ejecutar las pruebas
ampliadas, consultar errores de esta sesión, ver los resultados de almacenamiento,
perfiles y recursos, y ver la versión y la ruta. Ajusta DEBUG, INFO, WARN,
ERROR o FATAL para esta sesión. El ajuste de pantalla inicial completa o resumen
se guarda en la SD y siempre se conservan las comprobaciones.

## Registros y códigos

Ruta `sdmc:/3ds/3ds-chess/logs/`. `session.log` y `session.previous.log` rotan
en torno a 64 KiB por archivo. Un marcador `active` permite informar
`SESSION_INCOMPLETE` cuando la sesión anterior pudo quedar interrumpida. Puede
deberse a un cierre abrupto o a un fallo de escritura. El búfer incluye la versión,
idioma, arquitectura, sesión y tiempos relativos del proceso. El reloj del
dispositivo se registra cuando contiene una fecha plausible, sin validar su
hora. El registro omite nombres, texto ingresado, rutas personales y la clave
de Developer.

| Código | Uso |
|---|---|
| `PROFILE_SLOT_INVALID`, `PROFILE_NAME_INVALID`, `PROFILE_RESERVE_FAILED` | Validación de ranura/nombre o reserva en disco |
| `PROFILE_DATA_WRITE_FAILED`, `PROFILE_VERIFY_FAILED` | Guardado y reapertura del perfil inicial |
| `PROFILE_INDEX_WRITE_FAILED`, `PROFILE_INDEX_BACKUP_FAILED`, `PROFILE_INDEX_COMMIT_FAILED` | Temporal, respaldo y publicación del índice |
| `PROFILE_INDEX_CORRUPT`, `PROFILE_MIGRATION_DEFERRED` | Índice ilegible o importación anterior pendiente |
| `SAVE_WRITE_FAILED`, `SAVE_LOAD_FAILED`, `SAVE_BACKUP_USED` | Operación y detalle del guardado normal |
| `CHECK_SD`, `SD_FS_DETAIL`, `CHECK_PROFILE_CREATE` | Prueba SD, detalle FAT y creación aislada |
| `AI_THREAD_START_FAILED`, `HINT_START_FAILED`, `CHECK_AI_THREAD` | Trabajo asíncrono de IA y pistas |
| `GRAPHICS_INIT_FAILED`, `AUDIO_NDSP_FAILED`, `AUDIO_BUFFER_FAILED` | Inicialización y recursos |
| `APP_OPERATION_FAILED`, `ENTER_SCREEN` | Aviso genérico con área de la aplicación y transiciones |
| `COMPOSER_SAVE_FAILED`, `VARIANT_SAVE_FAILED`, `TRAINING_BACKUP_USED`, `ACADEMY_BACKUP_USED` | Operaciones específicas de modos |
| `CLOCK_FLAG`, `MATCH_RESULT` | Reloj, resultado y entrada a la escena |

Las transiciones incluyen tienda e inventario, trofeos, retos, campaña, academia,
Carrera, Supervivencia, torneos, biblioteca, idiomas, variantes y editor. Las
notificaciones de error comunes identifican el área. Algunos subsistemas solo
tienen cobertura por transición o aviso y no cuentan con instrumentación de cada
operación interna.

El registro se vacía por lotes al terminar diagnósticos, ante fallos de creación,
al suspender, al salir y periódicamente en menús. Si la escritura falla, los
eventos recientes se conservan en memoria. Los tamaños ARM medidos son 1016 bytes
para el diario y 640 bytes para la consola, incluidos sus miembros directos.
Los buffers guardan 32 eventos pendientes y 16 avisos o errores recientes. Estos
tamaños excluyen las asignaciones del motor, los estados de perfil y la E/S.
No se amplió la pila configurada. Las operaciones de SD son síncronas y un
dispositivo que quede esperando dentro de una llamada del sistema puede detener
temporalmente la interfaz. Las pruebas están separadas por pasos y la búsqueda
ampliada se ejecuta en un hilo.

libctru expone `threadOnException`, pero el manejador se ejecuta en contexto de
excepción. Escribir mediante stdio/FS en ese punto podría volver a fallar o bloquear
el hilo. El diario se limita a eventos capturados normalmente y al marcador de
sesión. Cortes de energía, cuelgues y fallos de la tarjeta pueden dejar el
último lote incompleto. La prueba de SD comprueba una transacción pequeña.

## Qué probar en la consola

1. Abre la candidata desde Homebrew Launcher y observa el resultado de SD,
   creación aislada, lectura de perfiles y audio.
2. En una ranura vacía, crea y activa un perfil. Cierra y vuelve a abrir para
   comprobar que aparece y que se puede activar de nuevo.
3. Ejecuta pruebas completas desde Developer y cancela una segunda ejecución.
   Comprueba el resumen inicial, el cambio de idioma y los errores recientes.
4. Si falla una etapa, anota el código de pantalla. Sal normalmente cuando sea
   posible y comparte `3ds/3ds-chess/logs/session.log` y, si corresponde,
   `session.previous.log`.

Referencias: [adaptador de archivos de libctru](https://github.com/devkitPro/libctru/blob/master/libctru/source/archive_dev.c)
y [API de excepciones de libctru](https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/thread.h).

## Resultados de verificación

- Base: `make -j4 test`, 46 programas de host y comprobaciones Python.
- Candidata: `make -j8 test`, 48 programas de host y comprobaciones Python.
  Incluye regresiones de perfiles, recuperación, rotación, log truncado,
  sesión incompleta, fallo de apertura, límite de escritura simulado por
  `RLIMIT_FSIZE`, cancelación, catálogo, render en inglés y prueba de SD bloqueada.
  Perft 4 del motor conservó 197281 nodos.
- `make sanitize-diagnostics`: las dos pruebas nuevas pasaron con ASan y UBSan.
- `make -j4 test-target`: ELF y 39 objetos ARMv6K, sin requisito Thumb-2.
- En Mac, las comprobaciones rápidas con estado sintético tomaron 13–15 ms
  incluyendo la espera del harness. La SD, los gráficos y el audio físicos
  todavía deben probarse en la consola.

Fragmento real generado por la prueba en Mac. Primero se simula un fallo de
migración y luego se usa el selector normal.

```text
+0s [WARN] PROFILE_MIGRATION_DEFERRED originals_preserved result=0
+0s [INFO] PROFILE_CREATE_REQUEST slot_zero_based result=0
+0s [INFO] PROFILE_CREATED selector_published result=0
+0s [INFO] PROFILE_SWITCH_REQUEST slot_zero_based result=0
+0s [INFO] PROFILE_LANGUAGE ES result=0
+0s [INFO] PROFILE_ACTIVATED slot_zero_based result=0
```
