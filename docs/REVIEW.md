# Repaso y practica

`ui::review` es una utilidad sin estado ni hilos propios. La interfaz conserva la partida original y usa `positionAt(game, ply, out)` para reconstruir otra partida desde su FEN inicial y movimientos legales. `ply` cuenta medias jugadas, desde cero. Se reconstruyen tambien enroques, en passant, promociones y repeticiones. El resultado externo (tiempo, rendicion, tablas acordadas) se conserva solo al llegar al ultimo movimiento. Ante un historial invalido, no cambia `out`.

`undoPractice(game, human, versusAi)` devuelve la partida al ultimo turno humano anterior. Deshace una jugada mientras piensa el rival o dos despues de su respuesta. Con negras, la apertura del rival sola no se puede deshacer: aun no hubo decision humana. En modo local (`versusAi=false`) quita una media jugada. Debe detenerse primero el worker de IA; el llamador controla que solo este habilitado en practica y restaura su reloj de practica. No toca recompensas, sesiones ni almacenamiento.

`captured(game, ply)` cuenta las victimas de las capturas realmente jugadas. `white[Pawn]` significa peones blancos capturados, por tanto las piezas que capturo el jugador blanco estan en `black`. No cuenta piezas ausentes de un FEN inicial ni peones promovidos como capturas. `valid` distingue fallo de una lista vacia.

`explain(game, human)` ofrece texto ASCII de hasta 44 caracteres por linea. Solo afirma mate cuando el tablero demuestra jaque y ausencia de movimientos legales; enumera las piezas que dan jaque. En otros finales muestra la razon registrada. El material es un resumen con valores convencionales (1/3/3/5/9), no una explicacion causal de la derrota.

`analyze(game, ply)` analiza el movimiento que termina en `ply` (1..N). Hace una busqueda real antes y otra despues, cada una limitada a 100 ms, 3000 nodos y profundidad 3, sin ruido. Solo publica alternativa y comparacion cuando se completo al menos profundidad 1; no inventa precision, Elo ni etiquetas de brillantez. La perdida en peones compara evaluaciones aproximadas desde el lado que movio. Las busquedas cortas pueden alcanzar distinta profundidad y pasar por alto tacticas; el texto lo advierte. Mate no se convierte en una cifra de peones. El llamador debe ejecutarlo solo al seleccionar una jugada, nunca por frame. No analiza todas las posiciones de golpe ni escribe datos persistentes.

Pruebas: `tests/test_review.cpp` cubre reconstruccion atomica, turnos de deshacer, en passant, promocion con captura, mate, rendicion y alternativa tactica real.

En 3DS, main reserva256KiB de pila (la predeterminada de32KiB no alcanza para el análisis y las listas legales). Las capturas y explicaciones se calculan al entrar/cambiar posición y se conservan para dibujar, no se reconstruyen cada cuadro.
