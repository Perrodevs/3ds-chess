# Prueba manual de navegación — 17 de noviembre de 2026

Ejecutable: `dist/3ds-chess.3dsx`, portada identificada como 0.9.0 RC.
Emulador: Azahar 2126.2, Vulkan, Mac ARM64. Interacciones mediante Computadora,
con observación de ambas pantallas. No se utilizó una consola física.

## Recorridos observados

| Comprobación | Resultado observado |
| --- | --- |
| Entrar desde portada y B desde menú principal | Regresa a portada |
| Entrenamiento → Biblioteca → revisión | Abre la partida archivada de una media jugada, e4, tablas acordadas |
| Izquierda en revisión | Retrocede de 1/1 a 0/1 y devuelve el peón a e2 |
| B en revisión | Regresa a Biblioteca |
| Eliminar → B | Cancela, mantiene 1/30 registros y recupera la fila Eliminar |
| B en Biblioteca | Regresa a Entrenamiento |
| Abrir Torneos → B | Regresa a Entrenamiento; se pueden seleccionar las categorías contiguas |
| Academia, primera lección | Presenta posición preparada y objetivo de capturar dama con torre |
| X en la lección | Marca d2–d4 mediante flecha y selecciona la torre |
| B con pista activa | Quita selección y flecha; permanece en el tablero |
| Ejecutar d2d4 con cruceta y A | Muestra objetivo completado y explicación de la captura |
| B después de completar | Vuelve al catálogo; muestra 1/10 completadas |
| B catálogo → Entrenamiento → menú | Regresa por las pantallas correspondientes |
| Opciones → Accesibilidad → B | Regresa a Opciones, conserva seleccionada la entrada Accesibilidad |
| Accesibilidad → botón táctil Volver | Regresa a Opciones |
| Guardar y salir → botón táctil Cancelar | Regresa al menú y el juego permanece abierto |

El saldo visible permaneció en 45 CC tras completar la lección.
No se reprodujeron los fallos de retorno corregidos en estos recorridos.

## Observaciones y límites

- La pantalla de resultado de academia usa el texto genérico «RETO COMPLETADO»
  y «Siguiente reto». Es una inconsistencia de nomenclatura observada, sin
  impedir completar la lección o volver. No se modificó código en esta sesión.
- Opciones muestra «SIN DSP: CONTINUA EN SILENCIO». La sesión verifica que la
  interfaz continúa operativa en ese estado; no valida música ni efectos audibles.
- Se perdió temporalmente la captura de pantalla de Computadora. Recuperar y
  elevar la ventana permitió continuar. No se atribuye ese fallo al juego.
- No se completó un torneo, no se simuló IA contra IA manualmente, no se
  recorrieron las diez lecciones, ni se verificaron aquí todos los márgenes
  táctiles, la rendición, el reloj, el guardado tras reinicio o el tinte Vulkan.
- Abrir revisión sin bloqueo en una partida corta no descarta problemas con
  historiales largos. Los resultados automatizados anteriores conservan su
  alcance separado de esta sesión manual.

## Protección de guardados

Antes de la prueba se copiaron los seis archivos de guardado. Después se detuvo
la emulación desde su menú y se preservó el estado de prueba por separado.
Los seis archivos originales fueron restaurados y comparados por SHA-256:
coincidencia exacta de nombres y contenido. Azahar quedó sin emulación activa.

Evidencia local: `build/manual-navigation-20261117/original-save/`,
`test-save/`, `hashes-before.json` y `restoration.json`.
La academia completada queda únicamente en `test-save/academy.dat`; no se
añadió progreso de pruebas al perfil del propietario.

Veredicto limitado: recorridos anteriores comprobados en emulador. Validación
manual integral y pruebas físicas pendientes.
