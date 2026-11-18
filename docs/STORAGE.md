# Guardado versión 3 (compatible con versiones 1 y 2)

`profile.dat` es la única fuente autoritativa de perfil, ajustes, estadísticas,
inventario, recompensas configurables y partida. Evita transacciones entre cuatro
archivos que podrían permitir cobrar dos veces el mismo resultado.

Cabecera ASCII: `3DSCHESS 3 CHECKSUM\n`, con checksum FNV-1a de 32 bits sobre
los bytes posteriores. Es detección accidental de corrupción, no autenticación.

Cuerpo, mediante enteros decimales y cadenas `std::quoted`, en este orden:

1. Nombre, monedas, completadas, victorias, derrotas, tablas, segundos, siguiente
   ID, último ID liquidado, máscara de desbloqueos, máscara de logros, mayor nivel.
2. Cinco índices equipados: tablero, piezas, paleta, marco, interfaz. Cero es estilo
   predeterminado para cualquier categoría, los demás índices se validan.
3. Notación española, efectos, música, nivel, color humano (+1/-1), modo (0–2),
   ID del bot seleccionado (0–4), control de tiempo seleccionado (0–4).
4. Recompensas: victoria, tablas, derrota, bonus por nivel, logro.
5. Sesión activa, ID, modo, nivel, humano, ID del bot de esa partida, resultado,
   motivo entre comillas.
6. Control de tiempo de la partida, milisegundos blancos y milisegundos negros.
7. FEN inicial entre comillas. Cantidad de medios movimientos. Lista de UCI.

Los IDs de bot son estables: 0 Pollito, 1 Hamchow, 2 Byte, 3 Magnus (homenaje),
4 Hikaru Nakamura (homenaje). No se reutilizan ni reordenan. Se persisten por
separado la selección para la próxima partida y el rival de la sesión actual:
cambiar de rival en el menú no cambia una partida que ya comenzó. El nivel
interno se conserva además del personaje y determina su presupuesto de búsqueda
y las recompensas, sin afirmar un ELO medido.

## Migración de guardados antiguos

La carga acepta las cabeceras 1, 2 y 3; toda escritura nueva usa versión 3. En la
versión 1 no existían los dos campos de bot. Se asigna Pollito a niveles 0–1,
Hamchow a niveles 2–3 y Byte a niveles 4–5, tanto en ajustes como en sesión. Esto
asocia una identidad al presupuesto ya existente, **sin cambiar el nivel anterior**,
ni reiniciar la partida, ni perder nombre, saldo, inventario, resultados, historial,
ID de sesión o ID del resultado liquidado. La siguiente escritura confirma el
formato nuevo mediante la misma transacción y conserva un respaldo legible de la
versión anterior. IDs de bot inválidos en versiones 2/3 se rechazan, igual que una
versión futura desconocida; no se sustituyen silenciosamente en archivos dañados.

En las versiones 1 y 2 no había relojes. Se migra tanto el ajuste como la sesión
a **Sin límite** (control 0, milisegundos 0/0), sin imponer un plazo a una partida
existente. La selección de tiempo de una próxima partida se guarda por separado
del control y los dos saldos de la sesión actual. Las versiones anteriores no
reconocen v3: para volver a una versión anterior se necesita una copia de sus datos.

Controles: 0 sin límite, 1 tres minutos, 2 cinco minutos, 3 diez minutos,
4 cinco minutos más tres segundos por jugada. Los saldos están en milisegundos,
no son fechas del reloj del sistema; guardar/cerrar no consume tiempo mientras
el juego no está en ejecución. No se serializa ningún instante monotónico del proceso.

La carga valida el control, saldos no negativos, cota de 24 horas y la cota más
estricta de tiempo inicial + incrementos ganados por los movimientos reales de
cada color. Sin límite exige exactamente 0/0. No se acepta un reloj agotado con
partida aún jugando, ni dos relojes agotados, ni victoria por tiempo con saldo
positivo o con el color equivocado a cero.

Un resultado `Tiempo agotado` / `Tiempo agotado: tablas` se reconstruye después
de reproducir las jugadas: debe estar agotado solo el reloj del jugador al turno,
y se vuelve a decidir victoria o tablas mediante `chessclock::adjudicateTimeout`.
Esto conserva la excepción de material del rival y evita restaurar un resultado
incoherente. No se adjudica como tiempo un mate o ahogado ya producido.

Los PGN nuevos identifican al bot seleccionado con su nombre, en lugar del nombre
genérico del motor. Un rival local continúa apareciendo como Jugador 2. La etiqueta
PGN `TimeControl` describe el control inicial, no el saldo restante: `-` sin
límite, `180`/`300`/`600` para minutos fijos, y `300+3` para cinco minutos con
incremento de tres segundos por jugada. Se inserta en la sección de etiquetas
sin cambiar la notación ni el resultado exportados por el motor.

Resultado: 0 jugando, 1 victoria blanca, 2 victoria negra, 3 tablas. La carga valida
longitud, checksum, versión, números, inventario, categorías y derechos FEN;
reproduce cada movimiento legal desde FEN y reconstruye claves de repetición.
Para resultados no automáticos valida rendición del jugador de turno, acuerdo
mutuo o una reclamación actual/por movimiento previsto. FEN por sí solo no basta
para reconstruir triple repetición; por eso se guarda el historial de movimientos.

La interfaz aplica cada cambio en memoria, valida y guarda el conjunto completo;
si falla, restaura el estado previo y muestra el error. No expone una recompensa
nueva que no se haya persistido junto con su identificador de liquidación.

Secuencia de escritura:

1. Validar la serialización nueva antes de tocar un archivo válido.
2. Escribir `profile.dat.tmp`, `fflush`, `fsync`, cerrar y comprobar errores.
3. Si el principal previo es válido, moverlo a `.bak`; si está corrupto, conservar
   el respaldo válido existente en lugar de sustituirlo con datos corruptos.
4. Renombrar `.tmp` a `profile.dat`.

La carga prefiere el principal válido y recurre al respaldo si hace falta. Un
`.tmp` no se considera confirmado. Si ambos fallan, inicia un perfil nuevo sin
bloquear el juego; se perderá ese progreso irrecuperable. El perfil nuevo solo
se escribe cuando exista una acción que guardar.

Se verificaron migraciones 1/2→3, controles y saldos, incrementos,
resultados por tiempo y rechazo de datos de reloj incoherentes; además, los cuarenta rivales, IDs inválidos, nombres en PGN,
archivo corrupto, checksum alterado, reanudación, terminales,
recompensas idempotentes y reemplazo de un principal corrupto conservando respaldo.
En Azahar se verificó escritura real mediante el backend SDMC de libctru.

No se ha probado cortar alimentación de una SD física durante un renombrado.
FAT, controladora, caché y falta de fsync del directorio limitan la durabilidad.
El respaldo puede representar una jugada anterior. Restaurar una copia antigua
restaura también el saldo antiguo; no combina saldos de historiales diferentes.
No se intenta defender contra edición intencionada ni múltiples instancias.

Tras guardar en v3, las versiones0.1/0.2 anteriores no reconocen el formato nuevo.
Para volver a una versión anterior conserva y usa una copia v1/v2 separada; no
abres el mismo perfil actualizado con un ejecutable viejo.
