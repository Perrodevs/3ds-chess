# Carrera y supervivencia: reglas y persistencia

## Separación del juego normal

`progression::State` contiene una sesión independiente `economy::Session` en modo
interno 2, sin reloj. La aplicación guarda este estado en el directorio del perfil
seleccionado, por ejemplo `progression.dat`. No se cambia `profile.dat` desde este
módulo. Carrera y supervivencia no incrementan las estadísticas competitivas,
logros ni recompensas normales por victoria del juego clásico.

Hay una sola partida de progresión activa. `start()` rechaza reemplazar una
partida sin terminar. Una supervivencia con racha activa también impide empezar
Carrera, incluso entre dos victorias; primero se debe continuar o abandonar el
intento explícitamente. La aplicación debe pedir confirmación antes de abandonar.

Cada partida comienza en la posición inicial clásica. El color admite blancas o
negras. Los IDs de partida de progresión tienen secuencia independiente de los
IDs competitivos. El rival se elige de forma determinista entre los 40 rivales
existentes que tienen el nivel necesario, rotando según el ID de partida; no se
crean motores ni presupuestos nuevos.

## Carrera

Cinco divisiones, por puntos internos (no Elo ni clasificación FIDE):

| División | Puntos mínimos | Nivel interno del rival |
| --- | ---: | ---: |
| Bronce | 0 | 0 |
| Plata | 9 | 1 |
| Oro | 21 | 2 |
| Diamante | 36 | 3 |
| Maestro | 54 | 5 |

Victoria: +3. Tablas: +1. Derrota: −1, con mínimo cero. El rango actual deriva de
los puntos; la mejor división alcanzada se conserva aunque el jugador descienda.
La máscara `promotions` registra los ascensos conseguidos por primera vez. Una
segunda subida a la misma división no vuelve a conceder recompensas.

| Primer ascenso | Monedas acumulativas nuevas | Producto cosmético existente |
| --- | ---: | --- |
| Plata | 150 | Forest (ID 2) |
| Oro | 300 | Royal (ID 3) |
| Diamante | 450 | Neon Blue (ID 1) |
| Maestro | 600 | Marco dorado (ID 10) |

Se guardan victorias, derrotas, tablas, puntos, mejor división y conjunto de
ascensos. El historial de ascensos en esta fase es el conjunto persistente de
rangos alcanzados, no una cronología con fechas. No se inventan fechas fiables.

## Supervivencia

El primer rival usa nivel 0. Cada dos victorias consecutivas sube un nivel, hasta
el nivel 5: `min(5, racha / 2)`. Una victoria mantiene activo el intento y permite
iniciar la siguiente partida. Una derrota o tablas termina el intento y deja la
racha actual en cero, conservando su récord. Guardar y salir no equivale a perder:
la posición confirmada y el intento permanecen para reanudar.

Abandonar durante la partida es una rendición del jugador, cuenta como derrota y
termina el intento. Abandonar entre partidas, después de una victoria confirmada,
termina la racha sin inventar otro enfrentamiento, derrota o pago. Empezar después
de terminar un intento incrementa el número de intentos y vuelve al nivel 0.

Se conceden solo una vez en toda la vida del perfil: primera racha de 3, 50
monedas; de 5, 100; de 10, 200. Volver a lograr esos récords no repite el premio.
El récord puede seguir creciendo sin aumentar los presupuestos de búsqueda.

## Integridad de resultados

`submit()` exige que la lista de movimientos sea una continuación exacta del
prefijo guardado, desde la posición inicial clásica. Reutiliza el decodificador
de guardados y las reglas existentes para reconstruir todas las jugadas,
posición, historial de repetición y resultado. Rechaza FEN de preparación,
movimientos ilegales, respuestas atrasadas de otra partida con distinto prefijo,
mutaciones del tablero/historial y resultados naturales inventados.

Solo se acepta rendición explícita del jugador humano: el motor no se rinde
por sí mismo, por lo que inyectar una supuesta rendición de IA no sirve para
conceder una victoria. Las tablas acordadas o reclamadas pasan por las
validaciones existentes del juego. No se añade un límite artificial de medias
jugadas: el formato conserva el límite existente de 16384 movimientos.

El resultado terminal se liquida una vez mediante `settledId`. Volver a enviar el
mismo resultado confirmado es una operación exitosa sin cambios; no vuelve a
sumar puntos, resultados o recompensas. La partida terminal se conserva para
revisión. Cada nuevo encuentro recibe otro ID.

## Recompensas entre archivos

El módulo calcula **derechos acumulados**, no paga directamente al perfil:
`totalCoins` equivale exactamente a la suma de primeros ascensos y hitos logrados,
y `entitledItems` al conjunto de productos correspondientes. Como los premios
son finitos, la suma máxima es 1850 monedas (1500 de carrera y 350 de supervivencia).

La aplicación confirma primero el archivo de progresión y después aplica la
diferencia entre `totalCoins` y su marcador de monedas Legacy ya entregadas en
el archivo de perfil, guardando saldo y marcador juntos. Los productos se aplican
con OR, de forma idempotente. Si el pago del perfil falla después de guardar
progresión, el mismo derecho queda pendiente para un reintento, sin repetir el
resultado de la partida. El módulo no promete una transacción atómica entre dos
archivos; esa integración requiere sus propias pruebas de fallo.

## Formato y límites

Archivo `PROGRESSION1`, checksum FNV-1a, metadatos y sesión embebida en el formato
validado de perfil existente. Límite del archivo: 1,600,000 bytes; sesión embebida:
1,500,000. Contadores de enfrentamientos: 10,000,000 y IDs menores que mil millones.
No se permiten desbordamientos ni recompensas incoherentes con sus máscaras.

El guardado valida antes de escribir, usa temporal, vaciado/sincronización/cierre,
relectura, respaldo del archivo principal válido y publicación por `rename`.
Carga principal válido o respaldo válido; si ambos fallan, marca `corrupt` sin
modificar el State del llamante. La aplicación debe desactivar el modo y conservar
los archivos cuando `corrupt` sea verdadero, nunca reemplazarlos automáticamente
por progreso vacío.

Un respaldo puede contener la jugada o resultado anterior. Restaurarlo recupera
ese punto confirmado, no garantiza conservar todos los eventos posteriores.
No se ejecutaron cortes eléctricos ni mediciones físicas de la SD.

## Pruebas del backend

`tests/test_progression.cpp` comprueba ascensos reales mediante secuencias de mate
legales desde el inicio, ambos colores, descensos y reascensos, premios únicos,
rachas de 3/5/10, aumento de nivel, tablas, abandono en partida y entre partidas,
exclusión de modos activos, prefijos inválidos, estados falsificados, cada
truncamiento y alteración de un byte, seis fallos de I/O y aislamiento de Stores.
Estas partidas de regresión usan jugadas escritas para comprobar reglas y
progresión; no se presentan como una medición de fuerza ni partidas disputadas
por la IA. La aplicación debe verificar adicionalmente IA asíncrona, controles y
restauración de sesión normal.

Comando independiente:

```
clang++ -std=c++17 -O2 -g -Wall -Wextra -pthread -Iinclude \
 source/chess/clock.cpp source/chess/chess.cpp source/ai/search.cpp \
 source/ai/styles.cpp source/ai/bots.cpp source/economy/profile.cpp \
 source/storage/store.cpp source/progression/progression.cpp tests/test_progression.cpp \
 -o build/test_progression
./build/test_progression
```

El backend no ejecuta búsquedas nuevas ni mantiene hilos propios. No se midió
rendimiento de una partida completa ni memoria máxima en Old 3DS XL. No se
modificó el tamaño global de pila, tabla de transposiciones o límites de búsqueda.
