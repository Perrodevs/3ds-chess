# Integración de interfaz de la actualización

`tests/test_update_ui.cpp` utiliza únicamente la interfaz pública de `ui::App`, eventos de botones y toques, y los lectores reales de archivos. No accede a campos privados ni sustituye el motor de reglas.

Cobertura ejecutada:

- Crear torneos de cuatro y ocho participantes desde entrenamiento.
- Guardar y abandonar una partida de torneo sin sustituir la partida normal.
- Retomar el enfrentamiento humano.
- Rendirse mediante confirmación, regresar al cuadro y conservar el resultado.
- Iniciar la simulación asíncrona y salir; comprobar que el progreso no cambia estando fuera.
- Recargar el perfil y comprobar que la partida normal conserva sus jugadas.
- Archivar una partida competitiva realmente terminada, reproducirla y exportar su PGN.
- Cancelar la confirmación de eliminación y luego confirmar la eliminación.
- Conservar estadísticas al borrar el registro; no volver a archivar ni contar el resultado al guardar otra vez.
- Archivar inmediatamente un final por tiempo mediante reloj inyectado, sin duplicarlo al guardar de nuevo.
- Abrir el historial con SELECT y regresar a la partida sin modificarla.
- Entrar en academia, comprobar el bloqueo inicial de la segunda lección y resolver la primera mediante movimientos reales siguiendo la pista.
- Guardar el progreso de academia, desbloquear la segunda lección y comprobar que abandonarla no la completa.
- Conservar monedas y los bytes del perfil normal al completar o abandonar lecciones.
- Generar legalmente una partida de 239 medias jugadas sin tablas reclamables, guardarla y recargarla (archivo mayor de 1024 bytes), y aplicar una respuesta real de IA.
- Al llegar a 240 medias jugadas, mostrar resultado de tablas, preparar la revancha invirtiendo colores y regresar al cuadro conservando la partida normal.
- Rendirse siendo humano con blancas mientras la IA tiene el turno; el resultado debe ser victoria de negras, sin aplicar una respuesta atrasada.

La prueba de integración pasó en host; el registro está en `build/update-090/update-ui-test.log`. Las vistas se renderizaron además como SVG locales `build/preview-update-*.svg` para inspección. El estado de ejecución definitivo debe leerse junto con el registro general de pruebas. Las verificaciones del archivo no equivalen a pruebas físicas de tarjeta SD, suspensión o pantalla nativa.

## Regresión adicional detectada

La prueba de 239 medias jugadas detectó que el lector de torneo comprobaba el salto de línea final al final de cada bloque de lectura de 1024 bytes. Eso rechazaba archivos válidos que ocupaban varios bloques. Se corrigió para comprobar el tamaño mientras lee y validar el salto final únicamente sobre el archivo completo. La misma prueba exige recargar el guardado antes de iniciar la partida en la aplicación.

La interfaz permite ahora rendirse también durante el turno de la IA, identificando expresamente al humano. Antes la interfaz bloqueaba esa acción; la prueba es una mejora de comportamiento y cobertura de la API, no evidencia de una victoria fraudulenta alcanzable en la interfaz anterior.
