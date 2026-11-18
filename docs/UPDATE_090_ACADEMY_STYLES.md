# Academia y estilos de IA — implementación 0.9.0

## Academia

`include/training/academy.hpp` y `source/training/academy.cpp` contienen diez lecciones interactivas: movimiento/valor, centro, desarrollo, protección del rey, enroque, ataque doble, clavada, ataque descubierto, caballo y final de rey/dama. Cada una incluye introducción, objetivo, posición FEN, pista y explicación de acierto/error. Son ejercicios guiados de una jugada, no cursos exhaustivos de cada tema.

`Run::submit` genera movimientos legales antes de comprobar el objetivo sobre la posición resultante. No compara simplemente la respuesta con la pista. Acepta, por ejemplo, d4/e4 para el centro, Cc3/Cf3 para desarrollo, ambos enroques, diferentes escapes legales del rey y diferentes movimientos que descubren el jaque de la torre. Las respuestas incorrectas no alteran el tablero. Repetir llama `start` y reinicia el ejercicio. La capa de interfaz marca el progreso solo tras `Completed`.

El progreso usa un archivo independiente `ACADEMY1`, máscara de diez bits, checksum, temporal y respaldo validado. Los bits deben formar un prefijo completado. El guardado no modifica los formatos anteriores de perfil, partida normal ni entrenamiento. No se conceden monedas. No se ejecuta IA para resolver las lecciones: los objetivos se comprueban con reglas y predicados acotados.

Las diez posiciones y las diez pistas se validaron con python-chess. Las pruebas C++ recorren todos los movimientos legales iniciales de cada lección: aceptan 23 respuestas que cumplen el objetivo y rechazan las otras conservando el estado. Incluyen desbloqueos, repetir, entrada ilegal, checksum/respaldo, datos truncados y fallos de escritura, flush, sync, close, backup y commit. El sistema no promete durabilidad absoluta ante cualquier corte eléctrico.

## Personalidades

`ai::Style` ofrece Equilibrado, Agresivo, Defensivo, Posicional, Táctico y Finales. `styleForBot` asigna configuraciones a los cuarenta identificadores estables. Son preferencias de este juego, no imitaciones auténticas de personas reales. La interfaz puede sustituirlas o elegir Equilibrado para revertir sus efectos.

Se reutilizan las características que ya calculaba la evaluación: movilidad, presión alrededor del rey, centralización, peones aislados/doblados y escudo de peones. Se añaden preferencias modestas por actividad del rey en finales y cambios de piezas cuando existe ventaja material. El sesgo total está limitado a ±180 centipeones. Los valores materiales, reglas, búsqueda de mate, repetición y reclamaciones no cambian. Equilibrado conserva exactamente la fórmula anterior. No se añaden asignaciones de memoria por evaluación ni se cambia el tamaño de la tabla, la pila, los seis niveles o sus presupuestos.

La tabla se reinicia entre búsquedas, por lo que no mezcla evaluaciones de diferentes estilos. El parámetro de estilo se copia al trabajador antes de iniciarlo y permanece inmutable durante su búsqueda. `Worker` conserva la cancelación y validación de posición previa a aplicar respuestas. Los argumentos nuevos son opcionales y Equilibrado es el valor por defecto para llamadas antiguas. Las preferencias iniciales de apertura por personaje existentes se conservan; el estilo afecta la evaluación durante la búsqueda. La sustitucion manual de Opciones se aplica al rival de la partida humana; las simulaciones IA contra IA usan el perfil asignado a cada rival.

## Evidencia disponible

Compilación host con clang++ C++17 `-O2 -Wall -Wextra` de pruebas independientes:

```
clang++ -std=c++17 -O2 -Wall -Wextra -Iinclude source/chess/chess.cpp source/training/academy.cpp tests/test_academy.cpp -o build/test_academy
./build/test_academy
clang++ -std=c++17 -O2 -Wall -Wextra -Iinclude source/chess/chess.cpp source/ai/search.cpp source/ai/styles.cpp tests/test_styles.cpp -o build/test_styles
./build/test_styles
```

Resultados: diez lecciones, 23 respuestas válidas, progreso/reintentos/fallos de guardado correctos. Corpus de cuatro posiciones con profundidad máxima tres y 6000 nodos por búsqueda: 24 búsquedas legales, cuatro elecciones diferentes respecto a Equilibrado y 76360 nodos totales; ninguna superó el límite. Los seis estilos tienen firmas de evaluación distintas en el corpus. Todos encuentran mate en una en el caso táctico y respetan cancelación previa con cero nodos.

Una ejecución host de 50000 evaluaciones de una posición de medio juego por estilo obtuvo: Equilibrado 18095 μs, Agresivo 18386 μs, Defensivo 18726 μs, Posicional 18251 μs, Táctico 18798 μs y Finales 18609 μs. Son mediciones de esta Mac, sensibles a ruido y compilación; no constituyen una mejora de rendimiento ni una estimación de tiempo en 3DS. La pequeña prueba táctica tampoco demuestra fuerza general ni Elo.

Las dos suites pasaron AddressSanitizer y UndefinedBehaviorSanitizer (`-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer`). Los cuatro objetos nuevos/modificados (`academy`, `styles`, `search`, `worker`) compilaron con las opciones ARMv6K del proyecto. La integración completa de interfaz y ejecutable la verifica la suite general.

Pendiente físico: legibilidad a resolución nativa, respuesta de controles, comportamiento subjetivo de estilos y duración real de búsquedas en Old 3DS XL. No se realizó prueba física ni se afirmó estabilidad de hardware.
