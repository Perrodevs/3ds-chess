# Legacy Update — sistemas restantes

- Editor FEN interactivo, 30 posiciones y 30 puzzles por perfil, nombres, edicion,
  confirmaciones, importacion/exportacion local y practica sin premios.
- Demostracion cancelable de mate1/2/3 contra todas las defensas, alternativas,
  captura y defensa con criterios explicitos; pendientes cuando agota presupuesto.
- Chess960, Three-check y King of the Hill: reglas, IA, revision, guardado separado,
  estadisticas, promociones y tres medallas adicionales.
- Bateria PTMU 0–5/carga, consultas30s y avisos6s sin detener controles/relojes.
- Corregidos limites tactiles de variantes, reclamaciones50 en pruebas de mate,
  y seleccion/contadores del creador ante edicion/fallo SD.
- Notacion de variantes reutilizada al dibujar; sin ampliar pila principal.
- Pruebas, sanitizadores, ARMv6K y entregables locales; sin validacion fisica.

# 0.9.2 Legacy Preview

- Carrera y Supervivencia con sesión independiente, IA real y reanudación.
- Cinco divisiones, ascensos únicos, récords y entrega recuperable de hitos.
- Sala pixel art de 16 trofeos derivados del progreso de cada perfil.
- Diez aperturas, veinte líneas legales: tablero, práctica, pistas y favoritas.
- Perfil formato 4 con marcador de pago; lectura de formatos 1/2/3 conservada.
- Corrige el destino de «Menú principal» después de resultados de progresión.
- Corrige rótulos de academia, estudio y resultado de torneo.
- Fases F–I aún pendientes; no es Legacy 1.1.0 completa.

# 0.9.1 Legacy Preview

- Inicio de producción con tres perfiles, nombres, avatares y confirmación al
  retirar una ranura. Datos por generación independiente, con índice respaldado.
- Migración verificada de los seis grupos de guardado 0.8.x/0.9.0 al primer hueco;
  conserva archivos originales y respeta los validadores existentes.
- Cambio de perfil desde menú; cancela trabajos y recarga todos los Stores de
  progreso. Rechaza una ranura corrupta sin crear datos por encima de ella.
- Opciones → Developer, código 1234 y SELECT para abrir. Resultado forzado y
  cosméticos/monedas sobre copia temporal; no premian ni alteran la sesión normal.
- Mantiene SELECT como historial al apagar Developer y permite consultarlo desde
  el panel cuando está activo.
- Pruebas de aislamiento, migración, corrupción, fallos de escritura y flujos UI.
- No incluye todavía Carrera, Supervivencia, sala nueva, aperturas, editor,
  creador de puzzles, variantes ni batería. No cambia a Legacy 1.1.0.

Recuperación, límites y pruebas físicas pendientes: `LEGACY_UPDATE.md` y
`LEGACY_PROFILES.md`. Solo para pruebas locales; recursos pendientes de derechos.

## Correcciones del taller durante la integración Legacy

- Iniciar un puzzle requiere confirmar primero su nuevo contador de intento en
  disco; un fallo SD vuelve al menú sin permitir acreditar otro intento previo.
- Guardar la edición de un puzzle conserva su selección por identificador estable
  en la lista, sin saltar al último registro.
- Regresiones con fallo real del temporal y edición del primero de dos puzzles,
  verificadas con AddressSanitizer y UndefinedBehaviorSanitizer.
