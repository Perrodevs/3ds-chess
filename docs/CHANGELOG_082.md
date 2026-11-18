# Cambios — 0.8.2 RC (2026-11-16)

Versión de estabilización de0.8.1, conserva40rivales y funciones. Sin pruebas físicas.

## Corrección y persistencia

- Formatear UCI/SAN/PGN rechaza movimientos/historiales corruptos sin salir del tablero.
- FEN rechaza cantidades imposibles de peones y material promovido; perft negativo termina.
- Estadísticas serializadas se acotan antes de sumar; no acepta overflow unsigned.
- Logros validan índices y jugadas candidatas; no premian una promoción ilegal.
- Exportar PGN comprueba flush/cierre antes de anunciar éxito.
- Entrenamiento sincroniza el temporal y rechaza categorías inválidas.
- Inyección opcional de6fallos de guardado; se preservan formatos v1/v2/v3 y TRAINING1.

## Interfaz, búsquedas y reloj

- Portada tiene3opciones seleccionables, igual que las3visibles.
- Tocar huecos/márgenes no activa botones; táctil+A no encadena dos pantallas.
- Un A simultáneo al botón de pausa no vuelve a reanudar la partida.
- Reclamar tablas no contamina ejercicios, revancha ni regreso al menú.
- Guardas de índices antes de App::play; resultados de IA cotejan posición e historial.
- Pistas de promoción conservan la pieza recomendada en el selector, evitando confirmar dama por defecto cuando el motor propuso torre.
- Fuente de reloj monotónica inyectable: fracciones probadas sin sleep real.
- Coordenadas inferiores del tablero dentro de240píxeles; versión canónica visible.

## Recursos

- Targets gráficos fallidos ya no se usan como punteros nulos.
- La salida espera la cola GPU antes de liberar atlas/vértices.
- Inicialización de Renderer/Audio idempotente y propietarios no copiables.
- Fallos parciales de buffers de audio revierten y permiten continuar sin audio.
- NDSP finaliza antes de devolver memoria de sus muestras.
- Efectos:12000→7000PCM16, ahorro exacto10000B, mismos sonidos.
- Generador legal filtra en su búfer: marco ARM3424→360B, sin heap adicional.
- Se conservan pila256KiB, TT16384entradas, atlas,6niveles y presupuestos de búsqueda.

## Verificación y entrega

- Regresiones nuevas de UI, estrés de cancelación, persistencia y empaquetado.
- Pruebas de ramas __3DS__ con SDK instrumentado, sin confundirlas con hardware.
- Perft adicionales y diferencial ampliado con python-chess.
- ASAN/UBSAN y ThreadSanitizer del estrés de búsquedas.
- Documentación actualizada:40personajes,160expresiones,360frases,41temas,
 giro con negras, práctica con deshacer y revisión aproximada.
- Inventario de derechos; empaquetado exige alcance local y versión coincidente.
- Sin personajes nuevos, pesos de personalidad nuevos, Stockfish integrado ni CIA.

Límites: posiciones muertas generales, derechos pendientes y validación física.
El alcance exacto de emulación y los resultados están en AUDIT_082.md.
