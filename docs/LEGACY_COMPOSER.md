# Editor y puzzles personalizados — backend

El editor trabaja con `chess::Position`; una posición incompleta puede mantenerse
en memoria. `validate` devuelve una explicación breve y solo una posición válida
puede convertirse en `Game` o almacenarse. El motor clásico no se modifica.

La validación comprueba un rey de cada color, límites de piezas y promociones
(incluidos alfiles del mismo color), peones fuera de las filas extremas, turno,
contadores, coherencia de enroque y ausencia de jaque al bando que acaba de mover.
Para captura al paso exige medio movimiento cero y reconstruye un doble paso legal
anterior. La importación FEN exige seis campos y rechaza dígitos vacíos consecutivos.
Esto valida invariantes de la posición; no pretende demostrar toda su historia
retroanalítica desde la posición inicial. Exportación/importación de archivos FEN
locales usa un límite de 161 bytes y no consulta servicios externos.

## Colección independiente

`composer.dat` guarda hasta 30 posiciones y 30 puzzles por perfil. Cada entrada
incluye ID estable, nombre ASCII de 1–24 caracteres y FEN. Los puzzles añaden
objetivo, solución de hasta cinco medias jugadas, estado de validación e intentos/
completados. IDs compartidos de colección son únicos y monótonos. No se sobrescribe
al alcanzar capacidad: la creación se rechaza. Un ID explícito solo permite editar
una entrada existente; la interfaz debe pedir confirmación antes de sobrescribir
posiciones o eliminar. Los métodos `put...` modifican la colección en memoria;
el llamador confirma con `save` y restaura su copia si falla.

Formato COMPOSER1, checksum FNV-1a, máximo 128 KiB. Las escrituras pasan por archivo
temporal, verificación, respaldo válido y reemplazo. Un archivo principal corrupto
puede recuperar `.bak`. Ambos inválidos se señalan y no se sustituyen con colección
vacía automáticamente. Se prueban fallos de escribir, vaciar, sincronizar, cerrar,
respaldar y confirmar; no son una simulación de todos los cortes eléctricos FAT.
El módulo no toca perfil, monedas ni partida competitiva.

## Objetivos y prueba de soluciones

- Mate en una/dos/tres: mate forzado en como máximo 1/3/5 medias jugadas. La línea
  grabada debe terminar en mate. Cada movimiento atacante de esa línea debe poder
  demostrar el mate contra **todas** las respuestas legales del defensor.
- Captura táctica: una jugada legal que capture una pieza enemiga (incluye captura
  al paso). No se afirma que gane material neto o sea la mejor jugada.
- Defensa: desde una posición en jaque, una jugada legal que deje de estar en jaque.
  No se afirma que salve una partida perdida o sea la única defensa.

La prueba de mate es AND/OR con profundidad máxima cinco y presupuesto de 50000
nodos por defecto, cancelable con `atomic<bool>`. Presupuesto agotado o cancelación
produce Pendiente, nunca una validación afirmativa. No hay estimación de Elo ni
análisis profundo. La interfaz debe ejecutar las pruebas en un trabajo cancelable
para mantener los controles disponibles.

`Run::start` vuelve a validar el puzzle (incluso si el archivo decía Validado).
Acepta soluciones atacantes alternativas si puede demostrar el mismo objetivo.
La prueba contempla las reclamaciones defensivas de cincuenta movimientos,
incluso mediante jugada prevista, y da prioridad al mate ya consumado. Un puzzle
empieza con historia vacia; sus cinco medias jugadas no alcanzan triple repeticion.
El defensor puede elegir cualquier respuesta legal; la pista propone una jugada
compatible con la prueba. Un movimiento incorrecto o una prueba pendiente no
modifica el tablero. El jugador puede practicar ambos colores; no hay recompensas
competitivas. La interfaz conserva intentos/completados y reinicia con `start`.
`Store::putPuzzle` comprueba estructura y legalidad de la línea, sin repetir la
prueba costosa que la interfaz acaba de realizar; el estado Pendiente puede guardarse.
No permite jugar un puzzle pendiente sin una posterior validación satisfactoria.

## Verificación disponible

`tests/test_composer.cpp`: FEN válidos/inválidos, EP y alfiles, mate y alternativas,
rechazo de una línea que da mate pero permite otra defensa, mate3, límite/cancelación,
captura/defensa, colección, capacidad, IDs, todas las truncaciones y seis fallos de
persistencia. Evidencia en `build/legacy-update/composer-backend.log`,
`composer-sanitizers.log` y `composer-arm.log`/`composer-arm.o`.
No se ejecutaron pruebas físicas ni se midió latencia en Old 3DS.

El compilador ARM con `-fstack-usage` informa 3224 bytes por marco recursivo de la
prueba y 3320 bytes para su llamador de validación, en esta compilación `-O2`.
La profundidad posterior a la primera jugada es como máximo cinco marcos (unos
16 KiB solo para esa recursión), más los llamadores y generación de movimientos.
Es evidencia estática del compilador, no una medición de pila viva en consola.
El trabajo de interfaz reserva 64 KiB y no aumenta la pila principal.
