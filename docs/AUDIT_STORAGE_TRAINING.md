# Auditoría 0.8.2 RC — persistencia, economía y entrenamiento

Fecha: 2026-11-16. Evidencia de host macOS y compilador ARMv6K; ninguna prueba
física. La línea base del conjunto principal pasó antes de modificar código
(`build/audit-082/baseline.log`). Este módulo no cambia versiones de archivos.

## Inventario y alcance

Se inspeccionaron completos `storage/store`, `economy/profile` y
`training/campaign`, sus cabeceras, pruebas y las llamadas de App relacionadas.
Existe exportación PGN, no importación desde interfaz. `profile.dat` integra
perfil, inventario, ajustes, sesión, reloj e historial. Sus respaldos y temporales
no constituyen ranuras independientes. Entrenamiento conserva máscaras separadas
para ocho puzzles, ocho capítulos y cuatro retos en `TRAINING1`.

Se verificaron mediante pruebas: los cuarenta IDs de rivales persistidos,
migración v1/v2 a v3 conservando ausencia de reloj, liquidación idempotente,
inventario de trece productos, compra/equipamiento, siete banderas de logros,
seis niveles y sus pagos, exclusión de modos sin premios, doce ejercicios con
réplicas/alternativas y progreso secuencial de campaña/retos. Los nueve diálogos
por personaje y los retratos se verifican en las suites de otros módulos.

## Problemas confirmados y regresiones

| Hallazgo | Gravedad | Probabilidad/condición | Corrección y prueba |
|---|---|---|---|
| `storage::decode`: suma de estadísticas unsigned con desbordamiento podía aceptar wins=UINT64_MAX, losses=1 y completed=0 | Media | Baja en archivos normales: requiere datos incoherentes con checksum válido | Acotar cada contador a completed antes de sumar; sonda `overflow` falla antes y pasa después |
| `economy::moveAchievements`: leía board[m.from] sin comprobar límites y concedía promoción por una petición ilegal | Alta para llamada inválida; impacto de premio incorrecto medio | Baja en UI normal, pero API sin precondición explícita y valores -1/64 reproducibles | Validar sesión, resultado, turno, índices y legalidad de candidatas a logro; sonda `achievement` rechazaba e2e4 con promoción antes del cambio |
| `Store::exportPgn`: devolvía éxito con datos aún en el buffer aunque fallara el cierre | Media | Condicionada a error de escritura diferido, por ejemplo espacio agotado | Flush y cierre explícitos con comprobación; hijo con RLIMIT_FSIZE=32 y SIGXFSZ ignorada reproduce falso éxito previo |

Las sondas previas están en `build/audit-082/before-overflow.log`,
`before-achievement.log` y `before-pgnflush.log`; todas terminaron por aserción
antes de aplicar las correcciones. El test posterior es
`tests/test_storage_audit.cpp`.

La validación legal de logros solo genera la lista cuando la petición parece
ser un enroque o una promoción: no añade otra generación legal a las jugadas
ordinarias. Las compras, reglas de pago y las siete banderas conservan su semántica.
App también debe validar índices antes de leer from/to; esa corrección pertenece
a la integración principal y se comunicó al responsable de App.

## Refuerzos sin declarar un fallo físico probado

- `ProgressStore::save` usa ahora `fsync` para su temporal principal antes del
  cierre/renombrado, en consonancia con el perfil. Antes existían fflush y cierre.
  No se afirma haber observado pérdida física ni haber probado cortes de SD.
- Un valor de `training::Kind` desconocido retorna count=0 y no puede iniciar
  ejercicios ni marcar progreso. Es defensa de API, no un camino normal de UI.
- Se añadió `storage::SaveFault`, argumento opcional con valor None predeterminado,
  para inyectar fallos deterministas en pruebas. No modifica el formato ni los
  llamadores existentes ni necesita dependencias externas.

## Fallos de escritura y corrupción

El nuevo test ejercita para ambos guardados las interrupciones lógicas en:

1. Escritura parcial del temporal.
2. Flush.
3. Sincronización.
4. Cierre.
5. Preparación del respaldo.
6. Commit/renombrado final.

En cada caso exige que save devuelva false, carga el último estado confirmado
(primario o respaldo), compara el perfil completo y prueba una escritura posterior
correcta. Un temporal de una transacción fallida nunca se toma como confirmado.
Se recorren todos los prefijos truncados y alteraciones de un bit por byte de un
perfil de prueba, comprobando que un decode fallido deja el destino intacto.

Estas inyecciones son errores lógicos controlados en el proceso, no equivalen a
retirar alimentación de una tarjeta física. En exportación sí se provoca además
un fallo real del sistema operativo mediante límite de tamaño de archivo en un
proceso hijo; los límites y señales del proceso principal no cambian.

## Memoria objetivo y límites

Mediciones estáticas con devkitARM, `-O2 -fstack-usage`:

| Función | Marco estático ARM |
|---|---:|
| storage::decode | 4288 B |
| Store::save | 928 B |
| Store::exportPgn | 552 B |
| Run::start | 3776 B |
| Run::submit | 3168 B |
| proveMate | 3240 B por nivel |
| moveAchievements | 3128 B |

Son marcos de función del compilador, no pico dinámico medido en consola. Las
llamadas auxiliares también consumen pila. La prueba de mates alternativos está
acotada a tres medias jugadas del catálogo; no se amplió su profundidad.

La carga de perfil limita la entrada a 1,500,000 bytes y 16384 medias jugadas; el
progreso lee como máximo 129 bytes y rechaza más de 128. El historial y las cadenas
se asignan en heap y crecen con las jugadas. La serialización y la validación
pueden mantener copias de texto/estado simultáneas; aquí no se presenta un máximo
de heap de hardware ni se aumentan cotas existentes.

## Comandos y resultados

- Las sondas de regresión antes de corregir fallaron en los tres casos indicados.
- `make -j2 build/test_storage build/test_training`: compiló; ambos ejecutables
  pasaron (`storage-tests.log`, `training-tests.log`).
- Host clang++ con `-fsanitize=address,undefined -fno-omit-frame-pointer` para
  `tests/test_storage_audit.cpp`: pasó (`storage-audit-sanitize.log`).
- La misma instrumentación para `tests/test_training.cpp`: pasó
  (`training-sanitize.log`).
- `store.cpp`, `campaign.cpp` y `profile.cpp` compilaron para ARMv6K con
  `-Wall -Wextra`, sin avisos; `.su` y objetos en `build/audit-082`.

## Pendientes y riesgos conocidos

- No se garantiza resistencia absoluta ante cualquier corte de alimentación.
- El directorio no se sincroniza explícitamente y FAT/controladora/cachés de SD
  pueden tener reglas de durabilidad diferentes de los archivos de host.
- No se soportan escritores concurrentes del mismo perfil; la aplicación mantiene
  una única instancia de guardado.
- El checksum detecta corrupción accidental; no autentica ni impide edición
  deliberada de archivos por el propietario.
- Restablecer un respaldo anterior restaura también su saldo anterior; no se
  combinan recompensas de ramas diferentes.
- No hay importación PGN ni varias ranuras. No se implementaron funciones nuevas
  de ese tipo durante esta auditoría.
