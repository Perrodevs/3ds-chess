# Pruebas físicas pendientes — 0.9.0 RC

No se ejecutó el juego en una Nintendo 3DS física durante esta actualización.
La compilación ARMv6K, las imágenes host y los sanitizadores no sustituyen estas pruebas.
Azahar se omitió siguiendo la indicación del propietario de continuar sin él.

## Preparación y datos

- Respaldar la carpeta de guardados de la SD antes de copiar el candidato.
- Abrir un perfil real de 0.8.2 RC: nombre, monedas, inventario, rival, partida,
  reloj, historial, entrenamiento y logros deben conservarse.
- Probar biblioteca vacía/llena, guardado y recuperación desde respaldos con
  copias de prueba. No cortar la alimentación de la consola para probar fallos.
- Para regresar a 0.8.2 RC, restaurar su copia de código/binarios y el respaldo
  de datos anterior. No usar el retroceso de versión como procedimiento de migración.

## Controles, pantallas y progresión

- Resolver las diez lecciones, equivocarse, pedir pistas, repetir, salir y volver.
  Comprobar legibilidad de explicaciones y progreso; no deben aumentar monedas.
- Jugar torneos completos de cuatro y ocho participantes; abandonar y reanudar
  durante partida humana y simulación; comprobar cuadro, empate, revancha,
  final, trofeo y acumulados al volver a abrir el juego.
- Simulación IA contra IA: comprobar respuesta de controles y latencia real.
  Los límites de búsqueda son cooperativos; no hay una medición física de tiempo.
- Consultar treinta partidas, recorrer promociones/enroques/al paso, exportar
  PGN, cancelar y confirmar borrado; verificar aviso de biblioteca llena.
- Elegir negras: coordenadas, cursor, flechas, último movimiento y revisión.
- Probar información de pieza, historial SELECT, reducir animaciones y los
  seis estilos; confirmar que las preferencias se conservan al reiniciar.
- Revisar los defectos gráficos históricos pendientes: tinte del texto y
  algunas capturas inmediatas incompletas observadas anteriormente en emulación.

## Rendimiento y servicios de consola

- Arranque en Old 3DS XL, partidas largas y revisiones de biblioteca llena;
  observar memoria, fluidez, escritura SD y transición entre menús.
- Pausa, tapa/suspensión, reanudación, guardar y salir durante IA y torneos.
  Confirmar que no aparecen respuestas atrasadas ni avances fuera del modo.
- Audio con DSP disponible y sin DSP, efectos y 41 temas existentes, bucles y
  transiciones; escucha final pendiente. No se midieron audio físico ni consumo.
- Verificar controles de reloj, promoción y rendición durante cualquier turno.
- Probar por separado la variante personal, sin mezclarla con el paquete principal.

Batería, temperatura y consumo físico no fueron medidos ni simulados.
Solo después de estas pruebas corresponde decidir si se publica una versión estable.
