# 0.9.0 RC — contenido, progresión y rejugabilidad

Candidato para pruebas físicas; no certificación de estabilidad en hardware.

## Añadido

- Academia interactiva de diez lecciones, objetivos comprobados con reglas,
  respuestas alternativas, explicaciones, pistas, repetición y progreso separado.
- Torneos offline de cuatro/ocho participantes, cuadro, partidas reales entre
  IA mediante un trabajador limitado/cancelable, reanudación, desempate explícito,
  final y trofeos simbólicos persistentes.
- Biblioteca local de treinta partidas compactas, revisión reutilizada, PGN,
  eliminación confirmada y rechazo visible de nuevas entradas cuando está llena.
- Estadísticas desde esta actualización por rival/color/nivel, victorias,
  derrotas, tablas, porcentaje y rachas; progreso educativo y torneos separados.
- Seis estilos configurables de evaluación, asociados a los cuarenta rivales;
  equilibrado permite revertir los cambios sin sustituir el motor.
- Historial rápido con SELECT, información opcional de pieza y reducción de animaciones.

## Revisión de navegación — 2026-11-17

- B vuelve al menú padre desde Opciones avanzadas, Biblioteca, Torneos y fin de entrenamiento.
- B desde el menú principal vuelve a la portada; se conserva la fila de origen al regresar de un submenú.
- B y Volver/Cancelar comparten rutas; las confirmaciones no ejecutan acciones al cancelarse.
- Los toques en márgenes/huecos de partida, revisión y Ayuda no activan botones vecinos.
- Se mejoraron pruebas que forzaban pantallas o no comprobaban el destino de B.
- Nueva suite de navegación desde el inicio, sin saltos de pantalla; 26 programas host en total.
- Deshacer queda limitado a práctica libre, excluyendo sesiones temporales de torneo.

Ver NAVIGATION_AUDIT_090.md. Comprobación manual de estas correcciones pendiente por falta del control de Computadora.

## Correcciones durante la integración

- Los finales por tiempo se archivan inmediatamente tras guardar el resultado,
  sin exigir otro guardado o reinicio; prueba con reloj inyectado y sin duplicados.

- Configuracion local de enlace con barrera CP15 de ARMv6K: el SDK instalado
  incorporaba un objeto de libgcc etiquetado ARMv7. ELF y objetos se comprueban
  mediante `make test-target`, incluida una llamada enlazada de prueba. No se
  atribuye a esto un bloqueo físico observado ni se modifica el SDK global.
- Reenlace al cambiar Makefile/specs para impedir reutilizar un ELF obsoleto.

- El lector nuevo de torneos acepta archivos válidos mayores de un bloque de 1024 bytes.
- Al alcanzar 240 medias jugadas, la partida humana muestra tablas y permite
  seguir con la revancha/cuadro en vez de quedar detenida en una jugada rechazada.
- Se reconcilia el próximo identificador de partida cuando el perfil se recupera
  desde un respaldo anterior a los datos de biblioteca/estadísticas.
- Rendición explícita por color, también durante el turno de la IA. La interfaz
  anterior bloqueaba ese momento; no se afirma que permitiera obtener una victoria falsa.
- Revisión de biblioteca utiliza su propia partida y color sin alterar el guardado normal.
- Los errores educativos se muestran en varias líneas dentro del tablero inferior.
- Guardar biblioteca evita validar otra vez un respaldo que será reemplazado,
  conservando recuperación y pruebas de fallos; mediciones en el informe de rendimiento.

## Conservado y límites

Cuarenta rivales/identificadores, 160 expresiones, seis niveles, reglas, tienda,
logros, música, práctica, campaña, puzzles y personalización. Perfil sigue usando
su formato 3 y lee versiones 1/2; los sistemas nuevos tienen archivos separados.
Las estadísticas históricas por rival no se inventan. Fecha desconocida si no
existe una fuente fiable. No hay red ni Stockfish integrado.

Para volver al ejecutable antiguo se recomienda restaurar el respaldo anterior:
0.8.2 no conoce los archivos nuevos ni la rendición fuera de turno que ahora es válida.
No hubo pruebas físicas ni nuevas pruebas en Azahar. Los recursos mantienen sus
restricciones de uso local; las piezas personales siguen separadas.
