# Torneos offline 0.9.0: módulo e integración

## Implementación

`include/tournament/tournament.hpp` y `source/tournament/tournament.cpp` implementan cuadros de cuatro u ocho participantes, eliminación directa, un humano y tres/siete rivales con identificadores originales. El rival seleccionado ocupa el primer enfrentamiento; los demás se eligen de forma determinista, sin duplicados. La selección de participantes no sustituye la disputa de las partidas.

`Session` conserva participantes, los siete nodos máximos del cuadro, enfrentamiento activo, revancha, partida actual y acumulados de torneos/trofeos. `start` conserva los acumulados y asigna un identificador monotónico. El trofeo es simbólico y no concede monedas competitivas. Los acumulados solo aumentan al resolver la final; volver a cargar un torneo terminado no los modifica.

`Controller::tick` ejecuta una búsqueda asíncrona a la vez para los enfrentamientos entre IA. Cada búsqueda usa profundidad/nodos del nivel interno cero y un límite adicional de 20 ms. La ordenación de participantes y los resultados de las partidas son conceptos separados: no se sortean vencedores. Las jugadas se validan mediante `Game::play`; se respetan las tablas automáticas y se reclaman las disponibles. Al abandonar la pantalla se debe llamar `Controller::stop`, y la aplicación debe cancelar su búsqueda de partida/pista antes de activar el controlador. El presupuesto temporal es cooperativo, no una garantía física de latencia de Old 3DS.

Las partidas humanas se reciben con `submitPlayerGame`, que reconstruye legalmente la continuación y rechaza resultados naturales inconsistentes. La aplicación utiliza su trabajador de IA habitual para el rival humano y conserva el progreso después de cada jugada. Los torneos no usan el archivo de partida competitiva.

## Tablas y desempates

Primera tabla: revancha desde la posición inicial con colores invertidos. Segunda tabla: avanza el participante con mejor cabeza de serie (menor índice del cuadro). Esto es un desempate administrativo explícito, no una victoria de ajedrez inventada. El humano ocupa la cabeza de serie cero. Cada partida tiene un máximo de 240 medias jugadas; alcanzar ese límite cuenta como tablas para aplicar esta misma política. No hay simulaciones ilimitadas ni partidas paralelas.

## Persistencia

`Store` recibe una ruta propia (por ejemplo, `tournament.dat`). Formato `TOURNAMENT1`, suma FNV-1a para detectar corrupción accidental, máximo de lectura 16 KiB y máximo 240 jugadas reconstruidas. Se comprueban participantes únicos, identificadores, estructura del cuadro, resultados/desempates coherentes, contadores y legalidad. El lector admite archivos de varios bloques de 1024 bytes y solo valida la terminación después de leer el archivo completo. La suma no es una firma criptográfica contra manipulación intencional.

Escritura mediante temporal, vaciado, sincronización, cierre, respaldo del archivo anterior válido y renombrado. Un archivo principal corrupto no reemplaza un respaldo válido. Si falla la carga principal se intenta `.bak`. Este archivo no modifica el perfil ni sus migraciones anteriores. La inyección de fallos comprueba límites lógicos de escritura; no simula todos los posibles cortes físicos de alimentación.

## Evidencia automatizada

`tests/test_tournament.cpp` verifica cuadros completos de cuatro y ocho participantes (partidas IA reales entre rondas), revancha, inversión de colores, desempate, trofeo, contadores conservados, rechazo de ganadores prematuros, reconstrucción de partidas, todos los prefijos truncados, corrupción, recuperación de respaldo, seis puntos de fallo de guardado y cancelación antes de cambiar de torneo.

Verificado en host con AddressSanitizer y UndefinedBehaviorSanitizer. El módulo compila por separado con devkitARM para ARMv6K, sin excepciones ni RTTI. La integración y la compilación completa corresponden a las pruebas generales de la actualización. No se hicieron pruebas físicas ni mediciones de tiempo/memoria en Old 3DS XL. La legibilidad del cuadro, continuidad de audio y latencia real quedan para la validación posterior del propietario.
