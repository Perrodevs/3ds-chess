# Explorador de aperturas: catálogo y práctica offline

El módulo `openings` contiene diez familias de apertura y exactamente dos líneas
por familia, compartidas entre todos los perfiles. Son secuencias educativas
originales de doce medias jugadas desde la posición inicial clásica, no partidas
históricas atribuidas a personas ni estadísticas de popularidad.

| Familia | Línea principal | Segunda línea |
| --- | --- | --- |
| Italiana | Desarrollo clásico | Gambito Evans |
| Española | Defensa cerrada | Defensa Berlín |
| Siciliana | Centro abierto | Esquema cerrado |
| Francesa | Caballo a c3 | Avance |
| Caro-Kann | Desarrollo del alfil | Avance |
| Gambito de dama | Rehusado | Aceptado |
| Londres | Centro con d5 | Contra fianchetto |
| India de rey | Centro clásico | Fianchetto blanco |
| Inglesa | Respuesta e5 | Estructura simétrica |
| Escandinava | Dama a a5 | Caballo a f6 |

Cada `Line` aporta nombre, idea estratégica breve, objetivo y secuencia UCI.
`branchCount(index)` devuelve dos para las diez entradas válidas y cero para un
índice inválido. `line()` devuelve una entrada segura «Apertura no disponible» si
los índices no son válidos. No hay carga de bases de datos externas ni conexión
de red.

## Exploración y práctica

`positionAt(index,branch,ply,out)` reconstruye el prefijo legal solicitado con el
motor existente. Permite avanzar o retroceder hasta la posición inicial sin
modificar la partida normal. Si los índices o el número de jugadas no son válidos,
devuelve falso y conserva el objeto de salida.

`Run::start()` valida la secuencia completa antes de abrir el ejercicio.
`Run::submit()` distingue:

- `Illegal`: movimiento ilegal, ejercicio sin iniciar o ya terminado.
- `Wrong`: jugada legal distinta a la de la línea estudiada.
- `Correct`: siguiente movimiento correcto.
- `Complete`: último movimiento correcto de la línea.

Una jugada incorrecta o ilegal no cambia tablero, turno ni posición dentro de la
línea. `hint()` devuelve el siguiente movimiento y `ply()` el número confirmado.
El jugador practica ambos colores, sin esperar a la IA y sin nuevas búsquedas.
Puede reiniciar la misma línea. Una jugada legal distinta se considera una
alternativa fuera del ejercicio, no un error universal de ajedrez; la interfaz
debe explicar esa distinción.

El módulo no entrega monedas, logros competitivos ni altera las reglas. La
aplicación reutiliza el tablero superior y presenta nombre, idea y objetivo en la
pantalla inferior. Este backend no controla menús, teclas físicas ni la interfaz
para escoger una línea; esa integración se prueba por separado.

## Progreso de cada perfil

`Store("<ruta-del-perfil>/openings.dat")` conserva:

- `favorites`: máscara de diez aperturas favoritas.
- `studied`: máscara de diez aperturas practicadas.
- `lastOpening`: último índice consultado, 0–9.

La marca de estudiada es por familia, al terminar una de sus líneas, no una
estadística individual de las veinte ramas. Los datos estáticos no se copian a
cada perfil. La aplicación modifica y guarda el progreso cuando se confirma una
acción; un fallo de guardado debe revertir el progreso en memoria o mantener una
indicación explícita de pendiente.

El formato `OPENINGS1` tiene checksum FNV-1a y un máximo de 256 bytes. Se validan
bits fuera del catálogo, índice negativo o fuera de rango, fin de archivo y
checksum. La suma comprueba corrupción accidental, no autenticidad frente a
edición intencional.

Las escrituras usan temporal, vaciado, sincronización, cierre, verificación,
respaldo previo válido y publicación por `rename`. `load()` intenta principal y
respaldo sin cambiar progreso ante fallo. `exists`, `corrupt` y `recovered`
permiten a la interfaz distinguir ausencia, corrupción y recuperación. Si ambos
archivos son inválidos, la aplicación debe conservarlos y desactivar los cambios
para no reemplazarlos automáticamente con favoritos vacíos. El temporal no se
interpreta como un guardado confirmado.

El formato no existía en 0.9.0, por lo que no inventa favoritos ni progreso antiguo.
No se promete durabilidad absoluta ante cortes eléctricos ni validación física.

## Verificación

`tests/test_openings.cpp` comprueba las veinte líneas, cada prefijo del tablero,
ambas ramas, errores legales/ilegales sin avance, movimientos de ambos colores,
pistas, finalización y reinicio. También cubre índices y prefijos fuera de rango,
persistencia, aislamiento entre archivos de perfiles, cada truncamiento y cambio
de un byte del archivo válido, tamaño excesivo, respaldo corrupto y seis puntos
controlados de fallo de escritura.

Comando independiente:

```
clang++ -std=c++17 -O2 -g -Wall -Wextra -pthread -Iinclude \
 source/chess/chess.cpp source/openings/openings.cpp tests/test_openings.cpp \
 -o build/test_openings
./build/test_openings
```

El catálogo usa texto constante y líneas cortas. Solo se reserva el vector de
hasta dieciséis movimientos al abrir/reconstruir una línea; la selección de cada
movimiento usa la lista legal existente. No se añaden hilos, texturas, presupuestos
de búsqueda ni cambios de pila. No se midió el máximo de memoria del juego
completo ni se probó en una Old 3DS XL física.
