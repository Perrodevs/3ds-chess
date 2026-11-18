# Legacy Update — implementación e integración

## A. Base conservada

La base era 0.9.2 Legacy Preview, con 40 rivales, motor clásico propio, seis niveles,
academia, campaña, ocho puzzles originales, retos, torneos, biblioteca, tienda,
perfiles, Developer, Carrera, Supervivencia, 16 trofeos y aperturas.
La suite existente pasó antes de ampliar módulos. No hay repositorio Git en esta
carpeta; no se crearon commits. Recuperación completa previa en
`build/legacy-update/recovery-092.zip`, SHA-256
`7fe592b1a40c96561983aaeb9b031a0ec6e0489ed4b37e9b12e012891ef7e044`.
No contiene guardados privados del emulador.

## B–C. Sistemas y arquitectura

Inicio → SELECT → Modos conserva las cuatro primeras opciones y añade **Editor y
puzzles creados** y **Variantes**. Son interfaces independientes que no reemplazan
la sesión competitiva. Las búsquedas clásicas, de variantes y de demostración de
puzzles se cancelan al salir, cambiar perfil o suspender; nunca se arrancan dos
búsquedas pesadas a la vez por estos recorridos.

Se reutilizan tablero, piezas, fuentes y el motor propio. No hay recursos nuevos
pesados, red, Stockfish ni dependencias de ejecución nuevas. La pila principal
sigue en 256 KiB. El worker de variantes conserva 256 KiB y el de demostración
64 KiB: profundidad máxima cinco y consumo estático por marco inspeccionado.
Esto no constituye una medición de RAM o rendimiento de consola.

## D. Perfiles y migración

Cada perfil tiene `composer.dat` y `variants.dat` independientes. El importador de
la estructura antigua reconoce esos archivos cuando existen. Los originales se
conservan. No cambia el formato competitivo v4 ni se inventan estadísticas de
partidas anteriores. Se mantienen los lectores de perfil v1/v2/v3/v4; los
programas antiguos necesitan restaurar la copia previa para volver a una versión
que no conoce v4.

Los sidecars tienen versión, checksum, límites, temporal, respaldo y seis fallos
simulados de escritura. Sus lectores validan estructuras y reconstruyen partidas.
La corrupción bloquea la escritura del módulo, conservando archivos y perfil
normal. No se promete resistencia absoluta ante cualquier corte de energía.

## E. Modos anteriores

Carrera, Supervivencia, torneos y ajedrez clásico conservan sus reglas y guardados.
Las estadísticas/recompensas nuevas de variantes son independientes y no otorgan
monedas. Tres medallas adicionales derivan de la primera victoria persistida en
cada variante: la sala reúne 19 reconocimientos, sin pagos repetidos ni fechas
inventadas. El panel Developer no interviene en el taller ni en variantes.

## F. Editor, puzzles y aperturas

El editor coloca/retira piezas con paleta táctil, mini-tablero inferior y cruceta
sobre el tablero superior. Configura turno, cuatro derechos de enroque y casilla
al paso; permite limpiar/restaurar, guardar, editar y jugar localmente una posición
válida. Acepta borradores incompletos para editar, pero no para jugar/exportar.

Colección máxima: **30 posiciones y 30 puzzles por perfil**. Nombres ASCII de
1–24 caracteres mediante el teclado existente. Sobrescribir y eliminar requiere
confirmación. Importación desde `import.fen` y exportación a `export.fen` dentro
del directorio del perfil activo, sin rutas arbitrarias ni servicios externos.
La validación comprueba piezas/reyes/peones, promociones posibles, jaque del bando
que acaba de jugar, derechos y antecedente legal de captura al paso. No pretende
resolver toda la retrolegalidad de cualquier FEN concebible.

Los puzzles se crean desde el editor grabando hasta cinco medias jugadas. Mate
en una/dos/tres requiere demostrar el objetivo frente a **todas** las defensas,
con prioridad del mate y reclamaciones de cincuenta movimientos. Una línea
escrita por sí sola no demuestra mate. El límite de 50000 nodos o cancelación
produce **Pendiente**, que se puede guardar pero no presentar como validado.

Captura significa una captura legal, incluida al paso; no promete ventaja neta.
Defensa significa salir legalmente de un jaque inicial; no promete salvar una
posición perdida. Los objetivos y límites están expuestos en la documentación.
La práctica acepta alternativas demostradas, pistas, reinicio y contadores sin
monedas. Se manejan promociones a las cuatro piezas.

Se guardan registros nombrados y contadores confirmados. El borrador sin guardar,
la grabación incompleta y el instante de práctica local son temporales; no se
promete reanudarlos tras cerrar. La sesión de variantes sí guarda cada movimiento
confirmado. Los diez grupos/veinte líneas de aperturas y sus favoritos se conservan.
Detalles: `LEGACY_COMPOSER.md`, `LEGACY_COMPOSER_UI.md` y `LEGACY_OPENINGS.md`.

## G. Variantes

- **Chess960:** 960 índices reproducibles, alfiles opuestos, rey entre torres,
  enroque con rey/torre estacionarios, cruces e intercambio. Para enrocar se
  selecciona el rey y su torre original, como explica la pantalla.
- **Three-check:** contador por color, una cuenta por jugada aunque dé jaque
  doble, victoria al tercer jaque; mate anterior sigue finalizando la partida.
- **King of the Hill:** gana la entrada legal del rey a d4/e4/d5/e5; nunca puede
  entrar en jaque.

Cada variante utiliza legalidad, resultados, evaluación, claves, revisión y
persistencia propias. IA negamax/alpha-beta con profundización iterativa, seis
presupuestos existentes y profundidad máxima seis; sin tabla adicional ni Elo
prometido. Respuestas canceladas/atrasadas se rechazan. La UI permite color, nivel,
semilla Chess960, reanudación, revisión, promociones, rendición confirmada y tablas
reclamables. 4096 medias jugadas es un límite administrativo visible; no inventa
tablas oficiales al alcanzarlo. Detalles: `LEGACY_VARIANTS.md`.

## H. Batería

Usa las funciones reales del PTMU de libctru instalado: nivel aproximado 0–5 y
estado de carga. Icono inferior discreto, sin porcentajes ni autonomía inventada.
Consulta cada 30 segundos; aviso de seis segundos al bajar a 0/1, con histéresis y
control de repetición. No pausa controles/relojes ni realiza guardados automáticos.
Si no hay datos, no muestra un nivel ficticio. Una fuente simulada verifica los
estados y la integración con un reloj de partida activo.
Detalles: `LEGACY_BATTERY.md`. Los umbrales físicos no están calibrados.

## I–J. Compatibilidad, correcciones y pruebas

Old 3DS XL sigue siendo el objetivo, ARMv6K/C++17/Citro2D/libctru/NDSP. No requiere
New 3DS. La suite clásica conserva perft inicial profundidad cuatro = 197281.
Las pruebas adicionales cubren los tres juegos, cancelación, revisión, guardados,
colecciones, FEN, mates falsos/alternativos, promociones, teclado cancelado,
confirmaciones, aislamiento entre perfiles y batería simulada.

La comparación Chess960 con python-chess comprobó **8279 posiciones de los 960
índices**: conjuntos legales y estado resultante coincidentes, incluido enroque.
Es evidencia de reglas sobre esos casos, no una prueba exhaustiva de todas las
partidas posibles. Sanitizadores y comandos finales están en
`LEGACY_TEST_RESULTS.json`; registros originales bajo `build/legacy-update/` y
`build/legacy-remainder/`. Los sanitizadores de host no prueban el hardware.

Se corrigieron límites táctiles de variantes y una demostración de mate que
ignoraba reclamaciones de cincuenta movimientos. La notación de variantes se
reconstruye al cambiar la partida y se reutiliza en dibujo, evitando repetirla
en cada frame. No se declara una aceleración numérica ni autonomía medida.

## K. Entregables

`dist/3ds-chess.3dsx`, `.smdh`, `3ds-chess-sd.zip`, `3ds-chess-source.zip`,
`SHA256SUMS` y `RELEASE.json`. La variante personal con piezas de Cosunosuke
permanece en `private/`; su fuente original se excluye del paquete de código.
Los recursos con revisión de derechos pendiente siguen limitados a pruebas
locales; no se publicó material ni se creó CIA.

## L–M. Límites y pruebas físicas pendientes

Faltan validación física, medición de RAM/fluidez en Old 3DS, suspensión real,
audio DSP, teclado y pantalla táctil de consola, lectura y transiciones de batería
real, SD/FAT bajo cortes físicos y sesiones largas. Los artefactos de captura/tinte
Vulkan observados anteriormente no se declaran corregidos.

El taller no analiza profundamente tácticas arbitrarias ni demuestra retrolegalidad
exhaustiva de FEN. Los borradores no guardados son temporales y los puzzles cuyo
mate agota presupuesto permanecen pendientes. Estos límites son explícitos, no
resultados simulados. La validación automatizada y cualquier recorrido parcial en
Azahar deben mantenerse diferenciados de pruebas físicas.

## N. Veredicto de software

**EXPANSIÓN IMPLEMENTADA — PENDIENTE DE VALIDACIÓN FÍSICA.**
La identificación de la candidata y las pruebas ejecutadas figuran en el manifiesto
canónico y en `LEGACY_TEST_RESULTS.json`. No se declara estabilidad física.
