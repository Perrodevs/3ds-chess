# Motor offline: mejora de búsqueda y evaluación

Actualización de 2026-11-15. Implementación original C++; no se integra código de Stockfish.

## Cambios reales

- Evaluación interpolada entre medio juego y final: material, actividad central, movilidad, presión sobre el rey, refugio de peones, columnas abiertas para torres, pareja de alfiles, peones doblados/aislados/pasados y seguridad ante ataques de peón.
- Búsqueda de ventana principal (PVS), ordenación por movimiento de tabla, capturas, dos movimientos de corte por profundidad y un historial de movimientos tranquilos. Se reutiliza primero el mejor movimiento de la iteración anterior.
- Tabla de transposiciones de 16,384 entradas, 384 KiB en heap (Entry24B según ABI ARM). Solo se reutiliza una puntuación cuando coinciden posición, contador de 50 movimientos e historial reversible; de otra forma se reutiliza únicamente el movimiento para ordenar la búsqueda. No se ignora la dependencia de las tablas respecto del historial.
- Claves Zobrist de 64 bits, incluida captura al paso solo si es legal. El historial FEN se convierte una vez al iniciar la búsqueda; no hay FEN, SAN ni cadenas nuevas por nodo. La pila de claves reserva capacidad antes de la búsqueda.
- Reclamaciones de triple repetición/50 movimientos son una opción de valor cero en la búsqueda, mientras que quíntuple/75 movimientos son terminales. Mate y ahogado se comprueban primero.
- Presupuesto de nodos y cancelación comprobados en cada nodo; reloj cada 64 nodos. Se conserva la última iteración completa y un movimiento legal de respaldo si no se completa ninguna. Límite de 31 medios movimientos de recursión.

## Fuerza cualitativa

Los índices antiguos se conservan para compatibilidad con partidas guardadas. Los valores ELO son únicamente objetivos/metadata; la interfaz debe mostrar rangos cualitativos.

| Índice | Rango | Profundidad máxima | Tiempo máximo objetivo | Nodos máximos | Ruido máximo (cp) |
|---|---|---:|---:|---:|---:|
| 0 | Bajo | 1 | 120 | 1200 | 220 |
| 1 | Bajo / Pollito | 2 | 220 | 3500 | 120 |
| 2 | Normal | 4 | 600 | 10000 | 45 |
| 3 | Normal / Hamchow | 5 | 1000 | 30000 | 15 |
| 4 | Fuerte | 7 | 1800 | 65000 | 0 |
| 5 | Fuerte / Byte y homenajes | 9 | 2500 | 110000 | 0 |

Los niveles fuertes no añaden errores aleatorios. El ruido de los bajos permite elegir entre decisiones de evaluación cercana; no modifica movimientos legales. Tiempo/profundidad realmente alcanzados dependen de hardware y posición. No se promete fuerza humana 2500.

## Verificación ejecutada

- `tests/test_strength.cpp`: mate en uno, captura de dama desprotegida, horquilla real, promoción a dama, presupuestos deterministas, límite de un nodo, cancelación previa y durante búsqueda, terminales, igualdad de puntuación con tabla activada/desactivada ante historial de repetición y horizonte de 50 movimientos.
- Pruebas de reglas originales (`tests/test_core.cpp`) pasan con el nuevo motor.
- AddressSanitizer y UndefinedBehaviorSanitizer ejecutaron la suite de fuerza sin errores reportados.
- `source/ai/search.cpp` se compiló realmente con devkitARM GCC 16.1.0 para ARMv6K, `-O2 -Wall -Wextra`, sin advertencias en la verificación final.
- El informe `-fstack-usage` de ARM muestra 4,376 bytes por llamada de negamax, 8,408 bytes para runWithBudget y 3,104 bytes para calcular una clave con posible generación al paso. Con 31 niveles, Search (~17 KiB), llamadas auxiliares y el hilo de 256 KiB, el cálculo estático permanece dentro del presupuesto. Es una estimación estática, no una medición de marca máxima en consola.

## Comparación reproducible con la versión anterior

Antes de sustituir el motor se extrajeron su cabecera y fuente del ZIP RC guardado en `build/before-bots/3ds-chess-source.zip`. Se renombró su namespace a `baseline` únicamente en un arnés temporal, y todos sus perfiles se fijaron al mismo presupuesto que el nuevo: profundidad máxima 6, 25,000 nodos, 3,000 ms, ruido cero. Ninguna búsqueda de esta muestra llegó al límite de tiempo (máximos observados en Mac: 54 ms nuevo, 51 ms anterior).

Se usaron cuatro posiciones escritas para los tests (mate, captura de dama, horquilla y promoción), y las posiciones 1–16 después del inicio del conjunto [Stockfish 8 benchmark](https://github.com/official-stockfish/Stockfish/blob/sf_8/src/benchmark.cpp). El Stockfish 8 de ese tag se compiló para macOS solo como oráculo externo de pruebas (1 hilo, hash 16 MiB, profundidad máxima 12 / 200,000 nodos por análisis, hash limpio entre análisis). No se enlaza, copia ni distribuye con el juego.

Para cada posición se comparó la jugada elegida con el mejor movimiento del oráculo. Además se analizó cada jugada candidata restringiendo la raíz a ella; las puntuaciones siguientes son del punto de vista del jugador de turno. Los valores de mate se representan con 30,000. No son mediciones ELO, ni una prueba de que el oráculo haya resuelto todas las posiciones.

| Posición | Nuevo | Anterior | Referencia | Valor ref. nuevo | Valor ref. anterior |
|---|---|---|---|---:|---:|
| mate-one | f7g7 | f7g7 | f7g7 | 29999 | 29999 |
| free-queen | d1d5 | d1d5 | d1d5 | 4602 | 4602 |
| royal-fork | c3d5 | c3d5 | c3d5 | 0 | 0 |
| promotion | a7a8q | e1f1 | a7a8q | 5106 | 5106 |
| SF8-bench-1 | e2a6 | e2a6 | e2a6 | -87 | -87 |
| SF8-bench-2 | b4f4 | b4f4 | b4f4 | 76 | 76 |
| SF8-bench-3 | f5d3 | f5d3 | f5d3 | 27 | 27 |
| SF8-bench-4 | d4c6 | d5e7 | d5f6 | 102 | -44 |
| SF8-bench-5 | d3d4 | a2a4 | c3b1 | -84 | -110 |
| SF8-bench-6 | b4b2 | b4b2 | b4b2 | -40 | -40 |
| SF8-bench-7 | f4d6 | f4e5 | f4g3 | 189 | 134 |
| SF8-bench-8 | g1h1 | a2a4 | b3c2 | 18 | 33 |
| SF8-bench-9 | e3g3 | e3g3 | f4f6 | 27 | 27 |
| SF8-bench-10 | e5g4 | e6d5 | e5d7 | -123 | -386 |
| SF8-bench-11 | d1d3 | b3d5 | a2a4 | 52 | -24 |
| SF8-bench-12 | a4c2 | a3d6 | c6c5 | 93 | 66 |
| SF8-bench-13 | c1b1 | b2b4 | a4b5 | -34 | -224 |
| SF8-bench-14 | e8e7 | f7f5 | e8d7 | 89 | 91 |
| SF8-bench-15 | c5d5 | c5d5 | c5d5 | 127 | 127 |
| SF8-bench-16 | e4f6 | e4d6 | e4f6 | 16 | -13 |

**Resultado limitado a esta muestra:** coincidencia exacta con la referencia 10/20 frente a 8/20; mejora mayor de 30 cp en 5 posiciones, diferencia dentro de ±30 cp en 15 y ninguna pérdida mayor de 30 cp. Profundidad completa media: 4.05 frente a 2.85. Tiempo medio Mac: 33.15 frente a 38.00 ms. Estos números muestran una mejora en este conjunto y presupuesto; no sustituyen un torneo amplio ni demuestran una clasificación humana.

Los datos crudos y el arnés de esta sesión están en `build/strength-comparison.json`, `build/strength-comparison.log`, `build/check_strength.cpp` y `build/compare_strength.py`. El directorio build no forma parte del paquete distribuido. Se conservan aquí los resultados y la metodología para que los límites de la evidencia sean explícitos.

## Lo que no se comprobó

No se ejecutó este motor mejorado en hardware Old 3DS físico, ni se midieron allí tiempo, memoria dinámica máxima, batería o fuerza. El port completo de Stockfish 8 a 3DS sigue sin estar validado: compilar su objeto thread.cpp o su ejecutable para Mac no demuestra que funcione en la consola. El motor original evita hacer depender los personajes de un port todavía no terminado.
