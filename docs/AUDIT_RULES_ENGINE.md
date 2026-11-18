# Auditoría 0.8.2 RC: reglas, búsqueda y revisión

Fecha: 2026-11-16. Solo análisis de código, pruebas host y compilación ARMv6K;
no se ha probado una consola física. El estado inicial pasó `make -j3 all test`.
Las evidencias de esta auditoría están en `build/audit-082/rules/` (no se empaqueta build).

## Inventario y resultados

- Legalidad, SAN/FEN, ambos enroques, captura al paso, promociones, jaque/mate/ahogado:
  implementados; perft y comparación independiente pasan.
- Triple/quíntuple repetición, 50/75 movimientos, reclamaciones actuales/intencionadas
  y prioridad de mate: implementados y cubiertos por pruebas C++.
- Material insuficiente: casos convencionales, sin equiparar incapacidad de forzar mate
  a imposibilidad de mate cooperativo. KNN contra K y menores opuestos no se declaran
  tablas por el mero material.
- Posiciones muertas generales: detección parcial; límites y contraejemplo más abajo.
- Motor propio: seis perfiles preservados; PVS, alpha-beta, profundización iterativa,
  quiescencia, ordenación, evaluación por fase y tabla contextual de transposiciones.
- Revisión: reconstrucción legal e inmutable y análisis acotado, no precisión global.
  Regresión de la partida de 42 medias jugadas, con promoción a torre y enroque mate.

## Problemas confirmados y correcciones

| Problema | Gravedad técnica | Probabilidad en uso normal | Evidencia y corrección |
|---|---|---|---|
| `Move::uci()` indexaba el texto de piezas con cualquier `promotion` | Alta (lectura fuera de límites) | Baja; requería un Move inválido en memoria | ASAN/UBSAN reprodujeron `Move{0,1,99}`. Se validan casillas y promoción antes de acceder; devuelve `0000` para una entrada inválida. |
| `Game::notation()` y `Game::pgn()` aplicaban `after(m)` incluso cuando SAN rechazaba `m` | Alta (acceso fuera del tablero) | Baja; las rutas normales ya validan movimientos | ASAN reprodujo historial `{{-1,80,0}}`. Ambos rechazan atómicamente el historial inválido antes de aplicarlo; PGN rechaza también FEN inicial inválido. |
| FEN aceptaba cantidades imposibles de peones o material promovido | Media (datos/reglas inválidos) | Baja; no existe importador FEN en menús | Se rechazan más de ocho peones por bando y más piezas adicionales que peones faltantes. El objeto de salida permanece intacto. No se pretende resolver toda alcanzabilidad histórica de un FEN. |
| `perft` con profundidad negativa no tenía condición de término | Media (recursión sin límite por argumento inválido) | Baja; arneses normales usan profundidades positivas | Retorna cero para un argumento negativo, con regresión. No se impone un nuevo máximo arbitrario de profundidad. |
| Lista duplicada en `Position::legal()` | Coste de pila/rendimiento confirmado | Siempre que se generan movimientos | Filtrado estable sobre la misma lista, sin asignaciones heap y preservando exactamente el orden. Mediciones abajo. |

Los dos errores ASAN anteriores se conservaron antes del arreglo en
`repro-uci.log` y `repro-notation.log`. Las pruebas corregidas pasan con sanitizadores.
La API `Position::after()` conserva su precondición explícita: solo movimientos legales
(o candidatos generados internamente). No se convirtió en una función de validación completa.

## Investigación del bloqueo previo de revisión

Medición del compilador objetivo devkitARM GCC16.1, ARMv6K, `-O2`, `-fstack-usage`:

| Marco/objeto ARM | Bytes |
|---|---:|
| `ui::review::analyze` antes del arreglo de listas | 17,840 |
| `Search::runWithBudget` | 8,408 |
| Cada llamada `Search::negamax` | 4,376 |
| Cálculo de clave del motor | 3,104 |
| `Position::legal` anterior | 3,424 |
| `Position::legal` con filtrado en el mismo búfer | 360 |
| `sizeof(ai::Search)` (objeto incluido en el marco de revisión) | 17,240 |

Estas cifras explican por qué 32KiB no alcanzaban: revisión + raíz + un nivel de
búsqueda + cálculo de clave y auxiliares ya superan ese presupuesto. El tamaño del
historial aumenta trabajo/heap, no una recursión de replay: la reconstrucción es iterativa.
No se encontró un índice fuera de límites en el replay legal de aquella partida.

Se conserva la pila principal de256KiB ya existente; no se eleva ni se reduce.
El límite de recursión de la búsqueda sigue en31medias jugadas, incluida quiescencia
y evasiones de jaque. Los31marcos de negamax suman135,656B; con revisión, raíz y
auxiliares la estimación estática queda bajo256KiB. Esta estimación no es una
medición de marca máxima en hardware y no incluye garantías sobre todos los
marcos internos de las bibliotecas. La prueba de42medias jugadas evita perder la
cobertura funcional del escenario que reveló el bloqueo.

`-fdump-lang-class` confirma que cada entrada de la tabla mide24B, alineada a8:
16,384entradas consumen393,216B =384KiB de elementos, no512KiB como indicaban notas
anteriores. Se añaden el vector y el historial de claves; no se cambió su capacidad.

## Medición limitada de la optimización

Microbenchmark host clang++ `-O2`, cinco muestras por versión, cada muestra ejecuta
cinco perft del inicio a profundidad4 (986,405nodos). Se compiló la versión anterior
antes de modificar el generador y se conservaron ambos binarios y registros.

- Mediana anterior: 43,286microsegundos.
- Mediana con filtrado: 39,622microsegundos.
- Diferencia en esta muestra: aproximadamente8.5% menos tiempo.
- Pila ARM de `legal`: 3,424→360B (3,064B menos).
- Sin heap adicional ni aumento de límites de búsqueda.

Las mediciones host pueden tener ruido por la carga concurrente. No prueban una
mejora porcentual en ARM, batería o fuerza ajedrecística. No se tocaron los pesos
ni las decisiones de evaluación del motor.

## Pruebas y comparaciones

- `make -j3 build/test_core build/test_review build/test_strength`: compila; los tres ejecutables pasan.
- Perft inicial0..4 conserva `1,20,400,8902,197281`.
- Permanecen Kiwipete y otras referencias anteriores.
- Cinco referencias nuevas (todas verificadas también con python-chess):
  enroques `26/568/13744`; EP `7/38/276`; promoción `9/41/500`;
  EP ilegal por exposición del rey `4/66/289`; evasión de jaque `3/41/505`,
  para profundidades1/2/3 respectivamente.
- `make differential`: 3,012posiciones FEN/legal/SAN y329historiales de resultados
  automáticos, reclamación actual y repetición, semilla20261115, pasan.
- python-chess incluye jugadas previstas en sus métodos `can_claim_*`; se compara
  explícitamente la condición actual con `Game::canClaim`, dejando la reclamación
  por jugada prevista a las regresiones C++. Perft enumera jugadas legales y no se
  detiene por tablas. Los resultados usan `claim_draw=False` para reglas automáticas.
- Comparación adicional de tabla activada/desactivada a profundidad3:18posiciones
  con profundidad completa coinciden en puntuación. Es un muestreo, no una prueba
  formal de ausencia de colisiones o de corrección universal de la búsqueda.
- ASAN+UBSAN de core, review y strength pasan. Logs `sanitize-{core,review,strength}.log`.
- Regresión de revisión: replay de0a42, análisis de41y42, SAN del enroque mate y
  contador de capturas válido; sin modificar el original.
- Los objetos auditados se compilaron para ARMv6K sin aumentar las reservas.

## Posiciones muertas: límite deliberado

El material habitual se resuelve de forma conservadora. No se añadió una heurística
de peones bloqueados o de imposibilidad de forzar mate: podría producir tablas falsas.

Caso adicional demostrable que sigue sin adjudicarse anticipadamente:
`kR6/8/1K6/8/8/8/8/8 b - - 0 1`. La única jugada legal es `a8b8`, que elimina la
última torre y deja KvsK. Por tanto no existe una secuencia legal posterior que dé
mate, aunque tanto la detección material actual como python-chess esperan la captura.

Se conserva la semántica de0.8.1 por compatibilidad: adelantar la adjudicación haría
que algunos historiales antiguos se detuvieran antes de sus movimientos guardados.
Añadir esa regla exige diseñar y probar la migración de replay/resultados, no solo
insertar una condición en el motor. La exploración general de todos los estados
alcanzables no tiene un presupuesto práctico justificado para Old3DS en esta versión.
La limitación queda explícita en lugar de introducir falsas tablas o romper guardados.

## Diferenciación de rivales: diseño conservado para una fase posterior

No se añadieron personajes ni se atribuyen estilos reales a los homenajes. Un diseño
reversible sería un perfil de pesos separado de los seis presupuestos: agresivo,
defensivo, posicional, táctico, equilibrado, finales, desarrollo e intercambios;
un perfil neutro mantendría los pesos actuales. Cualquier sesgo debería acotarse,
conservar material/mate/reglas y activarse mediante una configuración que pueda volver
a neutro. Antes de habilitarlo hacen falta posiciones etiquetadas y comparaciones
con presupuestos idénticos (legalidad, decisiones, nodos y coste). Esta auditoría no
inventó coeficientes sin evidencia ni cambió el juego para aparentar40motores distintos.
