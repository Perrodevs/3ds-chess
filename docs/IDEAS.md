> Archivo histórico de ideas, no inventario funcional. En0.8.2 ya existen pistas,
> capturas, revancha, práctica con deshacer, revisión básica, campaña, puzzles,
> retos, diálogos y temas por rival. Consultar README/AUDIT_082 para estado real.
> Las propuestas restantes no se consideran implementadas por aparecer aquí.

# Ideas para futuras versiones

Propuestas, todavía no implementadas. Las pantallas de inicio, guía, pistas,
resultados animados y relojes opcionales ya existen en 0.4 y no se cuentan aquí.

## Aprender y entender (prioridad alta)

1. Tutorial interactivo: mover cada tipo de pieza en ejercicios pequeños.
2. Lección de jaque: explicar por qué no puedes hacer una jugada concreta.
3. Lecciones de enroque, captura al paso y promoción con ejemplos jugables.
4. Explicación de por qué terminó la partida, marcando rey y atacante.
5. Pistas graduales: primero una idea, luego la pieza, al final la flecha.
6. Explicación breve de la pista: proteger, capturar, desarrollar o dar mate.
7. Ejercicios de mate en uno y dos.
8. Ejercicios de horquillas, clavadas, ataques dobles y piezas colgadas.
9. Entrenamiento de finales: rey y dama, rey y torre, oposición de peones.
10. Modo práctica con deshacer, separado de las partidas con recompensas.
11. Aviso opcional de pieza amenazada antes de confirmar un movimiento.
12. Diccionario visual de términos: jaque, mate, ahogado y tablas.

## Revisar y mejorar partidas

13. Recorrer la partida jugada hacia atrás y adelante.
14. Análisis posterior con errores principales y alternativas calculadas.
15. Reintentar una posición donde cometiste un error.
16. Mostrar material capturado y diferencia de material.
17. Resaltar siempre la última jugada de ambos bandos.
18. Resumen final: duración, número de jugadas, capturas y promociones.
19. Biblioteca de partidas guardadas con nombres y fecha.
20. Importar PGN desde la tarjeta SD.
21. Editor de posiciones y carga de FEN.
22. Repertorio de aperturas con explicaciones y práctica.

## Bots y progreso

23. Campaña: avanzar de Pollito a Hamchow y Byte con retos concretos.
24. Torneos offline contra varios bots.
25. Más bots originales, cada uno con retrato y personalidad propios.
26. Estilos de juego diferenciados en evaluación, además de aperturas.
27. Dificultad adaptativa opcional tras varias partidas.
28. Diálogos reactivos más variados, escritos de antemano.
29. Revancha conservando rival, color y reloj, con confirmación.
30. Desafíos con desventaja de material o posiciones especiales.
31. Misiones semanales locales sin conexión obligatoria.
32. Estadísticas por rival, color y control de tiempo.

## Controles y presentación

33. Girar el tablero al elegir negras.
34. Giro automático opcional en modo local.
35. Tablero táctil opcional en pantalla inferior, reorganizando el historial.
36. Usar Circle Pad y permitir ajustar repetición del cursor.
37. Animación corta de desplazamiento y captura de piezas.
38. Sonidos distintos para jaque, mate, promoción y reloj bajo.
39. Melodías originales por rival y controles separados de música/efectos.
40. Más temas de tablero y piezas con siluetas fáciles de distinguir.
41. Ajustes de contraste, colores alternativos y tamaño de texto.
42. Velocidad de animaciones y opción para reducirlas.
43. Resumen visible de botones que cambia según la pantalla.
44. Confirmación opcional antes de ejecutar un movimiento.
45. Guardar una imagen del tablero en la SD.

## Funciones de mayor alcance

46. Varios perfiles locales con progreso independiente.
47. Varias partidas en curso y ranuras de guardado.
48. Desafío diario offline basado en una colección incluida.
49. Modo Chess960 con reglas y pruebas propias de enroque.
50. Multijugador local entre dos consolas; requiere investigar comunicación y medir rendimiento real.

## Orden recomendado

Primero: tutorial interactivo, explicación del final, práctica con deshacer,
material capturado y giro para negras. Después: puzzles, repasar partidas y
campaña. Dejar análisis avanzado, Chess960 y comunicación entre consolas para
una etapa posterior; necesitan más trabajo y pruebas en hardware.

El análisis y las explicaciones de pistas necesitan una implementación propia:
la pista actual calcula un movimiento legal fuerte, pero no produce una
explicación ajedrecística fiable ni una clasificación de errores humanos.
