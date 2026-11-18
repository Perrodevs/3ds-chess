# Rival secreto: progreso y secuencias por perfil

`include/space/secret.hpp` y `source/space/secret.cpp` implementan un sidecar
pequeño, separado de monedas, logros y partida normal. Los identificadores son
estables: Byte 2, Jensen Huang 39 y Elon Musk 40. No se desplaza ningún rival.
El catálogo, retratos, idiomas y animaciones se integran fuera de este módulo.

## Desbloqueo y pruebas históricas

Se requieren victorias humanas contra Byte y Jensen en modo competitivo 0,
partiendo de la posición clásica. El resultado debe ser terminal y coincidir
con la reconstrucción legal del codec oficial del perfil, incluidos historial,
razón, reloj y posición final. Se excluyen prácticas, local y abandonos/acuerdos
antes de ocho medias jugadas, según el criterio de liquidación existente.
Un mate corto sí cuenta. Solo se procesa la sesión más reciente liquidada:
`session.id == profile.paidId`, con id mayor que `lastProcessedId`.

Las banderas de ambas victorias no retroceden. Cuando están ambas, `unlocked`
queda permanente. El sidecar no modifica la sesión, el saldo ni ningún logro.

Para datos anteriores se valida el archivo de biblioteca completo mediante su
codec, pero **solo se reconocen entradas conservadas**, reconstruidas y elegibles.
Las estadísticas agregadas pueden incluir abandonos antiguos y carecen de las
jugadas necesarias para distinguirlos. No se deducen victorias de esos contadores,
ni de totales del perfil. Si una victoria ya no tiene una entrada verificable,
se deberá volver a conseguir. Tampoco se inventan resultados históricos contra
un personaje que no existía. La biblioteca debe estar confirmada hasta un id no
mayor que el marcador pagado del perfil.

`initialize(state, paidId, optionalHistory)` se llama únicamente si el sidecar
no existe, antes de iniciar partidas nuevas. Fija `historyCutoffId` como horizonte
inicial y `lastProcessedId` para que un resultado antiguo no genere otra escena.
`recognizeHistory` puede reconocer después entradas recuperadas válidas; no
altera la alternancia de cinemáticas ni el horizonte inicial.

## Una sola animación y alternancia

`record` devuelve un evento solo para un id nuevo válido. Para Elon desbloqueado:

- Derrota humana 1: `Roadster`.
- Derrota humana 2: `Rocket`.
- Derrota humana 3: `Roadster`, y continúa alternando.
- Victoria humana: `FailedRocket`, sin avanzar el contador de victorias de Elon.
- Tablas y partidas excluidas: sin escena especial ni cambio de alternancia.

`elonWins`, `lastElonId` y `lastScene` se guardan junto al id procesado. Una carga
o llamada repetida devuelve un evento vacío y no aumenta el contador. La escena
persistida permite identificar la selección confirmada; no es una orden de volver
a reproducirla al abrir la revisión. Los otros rivales conservan su sistema normal.

La notificación de desbloqueo tiene una bandera independiente:
`unlockAnnounced`. La integración debe guardar con éxito `announceUnlock` **antes**
de comenzar su reproducción, tanto para desbloqueos nuevos como históricos.
Saltar o reabrir el resultado no vuelve a anunciar ni concede premios. Si ese
guardado falla, no se reproduce y queda pendiente para un punto seguro posterior.

## Política de guardado e integración

Ruta: `space.dat` dentro del directorio activo del perfil. Formato textual
`SPACE1`, checksum FNV-1a de integridad accidental, límite de 1024 bytes y números
acotados a 999999999. No usa un nuevo formato de `profile.dat`, no almacena fechas
inventadas y no concede monedas.

La UI conserva una copia anterior, procesa tras confirmar la liquidación normal,
guarda el sidecar y solo entonces reproduce el evento. Si falla, revierte la
mutación en memoria y conserva el resultado para reintentar antes de otra partida.
No hay una transacción física universal entre archivos FAT: una escritura segura
del perfil precede a este sidecar. No se promete resistencia absoluta a cortes.

Se escribe un temporal, se vacía y sincroniza, se valida, se conserva un respaldo
válido y se reemplaza el archivo principal. Se recupera del `.bak` si el principal
falla. Si ambos archivos están dañados, la carga no modifica el estado recibido y
el Store bloquea sobrescrituras; incluso una instancia que no llamó a `load` no
reemplaza archivos existentes sin una copia válida. Los temporales incompletos no
se promueven durante una carga. Los fallos simulados de escritura, flush, sync,
cierre, respaldo y commit no equivalen a pruebas de corte físico de energía.

## Comprobación

`tests/test_space_secret.cpp` cubre ambos colores, mates cortos, Byte/Jensen,
partidas excluidas, perfiles independientes, ids inválidos, historial/posición
manipulados, pagos aún no confirmados, resultados duplicados y atrasados,
historia verificable frente a agregados/entradas borradas, notificación persistida,
Roadster/cohete y victoria humana, corrupción/truncado, recuperación y seis fallos.

La suite host y AddressSanitizer + UndefinedBehaviorSanitizer pasaron. Registros:
`build/space-language/secret-host.log` y `secret-sanitizers.log`. El módulo compiló
para ARMv6K con la libctru instalada, sin avisos. La integración final conserva
sus registros separados.
No se realizaron pruebas físicas ni se atribuye rendimiento de Mac a Old 3DS XL.
