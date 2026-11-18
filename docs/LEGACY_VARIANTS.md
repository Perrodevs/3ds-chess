# Variantes: reglas, motor y persistencia

## Alcance del backend

`variants::Game` conserva tres juegos distintos del ajedrez clásico: Chess960,
Three-check y King of the Hill. Reutiliza los ataques y movimientos ordinarios
del motor propio, sin modificar `chess::Position` ni añadir Stockfish.
`Position::board` es la posición visible; sus métodos clásicos no deben utilizarse
para adjudicar una variante. Se debe usar `Position::legal/after` y `Game::play`.
El adaptador `snapshot()` es solo para presentación. Las anotaciones y la
reconstrucción usan `notation()` y `atPly()` del módulo.

## Chess960

Los índices Scharnagl 0–959 producen las 960 posiciones, alfiles en colores
opuestos y rey entre las torres. 518 es la posición clásica. El índice persistido
reproduce siempre el mismo inicio; no depende de un generador aleatorio.

Los cuatro bits de derechos conservan su sentido K/Q/k/q, asociados a las
casillas originales de las torres. El movimiento de enroque usa UCI960:
**rey a su torre original**. Esto evita confundir un paso ordinario del rey con
un enroque y representa también los casos donde el rey ya está en c/g.
El destino final es c/d o g/f. Se quitan ambos orígenes antes de colocar ambos
destinos, de modo que funcionan cruces, intercambios y piezas estacionarias.
La legalidad comprueba ocupación de ambos trayectos, jaque inicial, tránsito y
ataque final después del desplazamiento de la torre. Mover/capturar una torre
original elimina su derecho; no lo restaura una torre promovida.

## Three-check y King of the Hill

Three-check cuenta una vez cada movimiento legal que deja al contrario en jaque;
un jaque doble sigue contando como uno. El tercer jaque gana inmediatamente,
incluso al llegar simultáneamente a 150 medias jugadas. Mate anterior también
finaliza la partida. No se declaran tablas por rey y pieza menor contra rey:
una pieza menor puede dar tres jaques. Solo reyes sí es material insuficiente.

King of the Hill gana al entrar legalmente con el rey en d4/e4/d5/e5. Nunca se
permite entrar en una casilla atacada. No se utiliza material insuficiente de
ajedrez clásico: incluso dos reyes pueden competir por el centro.

El resto conserva mate, ahogado, 75 movimientos automáticos y repetición
quíntuple. Hay reclamación de 50 movimientos/triple repetición, también por
jugada prevista validada. Las claves distinguen variante, derechos, orígenes de
torres, jaques acumulados y captura al paso legal. No se comparten tablas de
transposiciones entre modos.

## IA y límites

Búsqueda propia negamax/alpha-beta con profundización iterativa, material y
actividad del evaluador existente, más jaques acumulados o distancia del rey al
centro según el objetivo. Identifica victorias de variante dentro del árbol;
no se limita a una bonificación estética. No se promete fuerza Elo.

Mantiene los seis presupuestos existentes de nodos y tiempo; limita la
profundidad de variantes a seis. No utiliza tabla de transposiciones adicional.
Se comprueba cancelación en cada nodo; un resultado cancelado no ofrece jugada.
El worker tiene el mismo límite de pila de 256 KiB del worker clásico existente.
La integración debe detener el worker clásico antes de arrancar este y viceversa.
`poll` solo entrega respuestas si coinciden variante, semilla, historial, estado,
contadores y posición completa, y la jugada sigue siendo legal.

La capacidad de una sesión es 4096 medias jugadas. Alcanzarla rechaza jugadas
adicionales, conservando revisión/guardado/rendición; no inventa tablas oficiales.
No se han medido tiempos, consumo ni memoria efectiva en Old 3DS XL.

## Guardado y estadísticas

`variants.dat` por perfil es independiente de la partida normal. Formato
`VARIANTS1`, checksum FNV de integridad accidental, límite de 1 MiB, temporal,
respaldo y seis puntos de fallo inyectables. No es protección criptográfica.
El lector reconstruye desde variante/índice inicial y valida cada movimiento,
resultado y razón; no acepta un resultado automático inventado.

`State` guarda bando humano, uno de seis niveles y resultados por variante.
Antes de guardar un resultado se llama a `settle`, que actualiza el contador y
la marca de liquidación juntos. Repetirlo no suma otra partida. No concede
monedas ni cambia estadísticas competitivas. No migra ni altera formatos antiguos.
La reanudación corresponde al último movimiento confirmado, no al instante exacto
de una búsqueda. Un archivo corrupto no sustituye el perfil por valores vacíos.

## Evidencia automatizada del backend

`tests/test_variants.cpp`: 960 inicios distintos, estructura de piezas, enroques
con rey/torre estacionarios y con intercambio, ataque descubierto al mover torre,
tercer jaque frente a regla de 75, rey central legal/ilegal, IA ganadora de las
dos variantes y ambos colores, hasta 48 medias jugadas de autojuego real por
variante, presupuestos, cancelación y respuesta atrasada, reconstrucción,
liquidación única, truncados, corrupción, respaldo y seis fallos de escritura.

`tests/differential_variants.py`: 8279 posiciones válidas de todos los índices
comparadas con python-chess 1.11.2 instalado: conjuntos legales y posición/FEN
resultante de cada movimiento, incluido enroque UCI960. Incluye partidas legales
deterministas y posiciones despejadas con bloqueadores/atacantes. La comparación
es de reglas, no una afirmación de rendimiento o equivalencia absoluta.

El backend compiló para ARMv6K sin avisos. Pasaron AddressSanitizer +
UndefinedBehaviorSanitizer y ThreadSanitizer sobre la suite del backend.
Registros: `build/legacy-remainder/variants-host.log`,
`variants-sanitizers.log`, `variants-tsan.log` y `variants-differential.log`.
La integración final tiene sus registros separados. No hubo pruebas físicas.
