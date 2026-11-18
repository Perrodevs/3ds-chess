# Pistas offline

`ai::Hints` calcula una sugerencia con el motor fuerte en un hilo independiente.
El presupuesto de búsqueda es 600 milisegundos, con el límite de nodos del nivel
fuerte y cancelación segura. Es un presupuesto objetivo; la comprobación del
reloj de búsqueda se realiza por lotes acotados, no una interrupción exacta al
microsegundo. No depende de internet, servicios externos ni un libro de aperturas.

La pista **no ejecuta la jugada**, cambia el tablero ni decide por el jugador.
El módulo de interfaz controla el botón, la pieza destacada y la flecha hacia
el destino. Confirmar o seguir la recomendación sigue siendo una acción manual.
Una promoción puede incluir una pieza recomendada; la interfaz conserva la
selección de promoción correspondiente.

El reloj de la partida continúa consumiendo el turno humano mientras se calcula
una pista. La interfaz debe solicitarla solo durante un turno humano en curso,
cancelarla al mover/pausar/suspender/terminar y ocultar cualquier flecha anterior
cuando la posición cambia. Una pista es consejo de un motor limitado, no una
promesa de jugada perfecta ni una explicación de análisis humano.

## Protección de resultados atrasados

El módulo guarda una copia de la posición y la identidad de la partida al
solicitar la búsqueda. Antes de entregar una respuesta compara posición completa,
FEN inicial e historial de movimientos y repeticiones, y valida otra vez que la
jugada sea legal. Si la partida cambió, terminó o la petición fue cancelada, no
entrega una sugerencia antigua. La comparación de historial completo ocurre al
llegar la respuesta, evitando recorrer un historial largo cada cuadro.

`poll` devuelve una sugerencia válida una sola vez; un resultado falso también
puede significar que el motor sigue trabajando. `running()` permite distinguir
ese caso. `cancel()` une el hilo antes de liberar la petición o iniciar otra.

## Verificación

`tests/test_hints.cpp` comprueba mate en uno, captura de dama, que pedir pistas
no altere la partida, entrega única, cancelación, sustitución de una petición,
cambio de posición e historial, y rechazo de posiciones terminales. Son pruebas
de lógica en la Mac; por sí solas no validan la flecha GPU ni controles físicos
en una Nintendo 3DS.
