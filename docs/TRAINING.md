# Entrenamiento, campaña y retos

El entrenamiento usa posiciones y progreso independientes de la partida normal. No cambia monedas ni estadísticas de partidas competitivas. La aplicación integra el catálogo mediante `training/training.hpp`.

## Catálogos

- Ocho puzzles: cinco mates en una y tres mates en dos. Incluyen un puzzle con azules. Todos están abiertos desde el inicio.
- Ocho capítulos de campaña: partidas ante Steve, Alex, Ellie, Joel, Claire, Jill, Leon y Chris, en ese orden. Ganar abre el capítulo siguiente; se puede repetir cualquier capítulo completado.
- Cuatro retos: coronación con mate, captura al paso, enroque corto y mate en dos jugando con azules. Se desbloquean en orden.

Las posiciones son propias de enseñanza de reglas y patrones clásicos; no son posiciones importadas de una base de datos. Los capítulos indican únicamente el rival: la aplicación crea y administra esas partidas.

## API e interacción

`Kind` distingue `Puzzle`, `Campaign` y `Challenge`. `count(kind)`, `exercise(kind, index)` y `chapter(index)` exponen datos inmutables. Los ejercicios contienen nombre, descripción, FEN inicial, línea sugerida en UCI, rival y objetivo.

`Run::start(kind, index)` devuelve `false` ante un identificador inválido o si la línea no es legal. Solo inicia puzzles y retos. La partida está disponible como referencia constante con `game()`. `submit(move)` devuelve `Illegal`, `Incorrect`, `Advanced` o `Completed`; en `Advanced`, la respuesta del rival ya se ha aplicado. Un intento incorrecto no modifica el tablero. `hint()` muestra la siguiente jugada sugerida sin ejecutarla. Reiniciar el mismo ejercicio vuelve a la posición inicial.

Se aceptan mates legales alternativos. Para una primera jugada distinta en mate en dos, el módulo verifica que todas las respuestas legales del rival permitan el mate dentro del límite, y adapta la continuación. Esta comprobación está acotada a tres medias jugadas del catálogo; no usa un análisis profundo ni estima precisión o Elo. El reto de coronación también admite una torre si corona dando mate.

## Progreso y archivos

`Progress` contiene tres máscaras independientes y ofrece `completed`, `unlocked` y `markComplete`. Esta última devuelve `true` solamente cuando se registra una nueva finalización permitida; repetirla no duplica progreso. La aplicación debe llamarla al resolver el ejercicio o ganar el capítulo. El módulo no toca `economy::State`.

`ProgressStore(fullFilePath)` recibe la ruta completa, no un directorio. El directorio padre debe existir. `load()` carga el archivo principal o recupera `.bak`; `recovered` indica recuperación. Si ambos son inválidos, el progreso queda vacío y devuelve `false`. `save()` escribe y verifica `.tmp`, conserva un respaldo validado y renombra el archivo temporal sobre el principal. El formato versionado `TRAINING1` usa un checksum para detectar corrupción accidental y comprueba rangos y desbloqueo secuencial. No protege contra modificaciones deliberadas del archivo.

El guardado no reemplaza un respaldo válido con un archivo principal corrupto. Esta operación tiene las limitaciones de durabilidad del sistema de archivos; no promete recuperación ante todo corte de alimentación durante escritura física de la tarjeta SD.

## Verificación

`tests/test_training.cpp` recorre las doce soluciones con el motor de reglas del juego, comprueba réplicas, rechazo de intentos inválidos sin mutación, reinicio, mate alternativo en dos, coronación alternativa con torre, desbloqueo e idempotencia de los ocho capítulos, persistencia, corrupción y recuperación del respaldo. Se verificaron las posiciones y líneas también con python-chess, y se compiló el módulo para ARMv6K. El ejecutable nativo pasó AddressSanitizer y UndefinedBehaviorSanitizer.
