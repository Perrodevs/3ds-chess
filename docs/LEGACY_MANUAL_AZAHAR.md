# Comprobación parcial de Legacy en Azahar

17 de noviembre de 2026, Azahar 2126.2, Vulkan, mediante Computadora.
Binario identificado en portada como **0.9.1 Legacy Preview**. Hash del ejecutable
probado y restauración: `build/legacy-update/manual/restoration.json`.

Observaciones directas:

- Arranque con tres perfiles; el primero muestra Pollito importado y dos vacíos.
- Seleccionar el importado muestra portada y menú con los 45 CC anteriores.
- Opciones incluye Developer bloqueado; permite introducir 1234 con los botones
  táctiles de dígitos y activarlo mediante A.
- Se inicia una partida local 5+3; SELECT abre las seis opciones Developer.
- Ganar muestra un resultado de prueba y cabecera DEVELOPER / TEMPORAL. El saldo
  permanece en 45. Volver y Continuar recuperan la partida en posición inicial,
  todavía activa, con su reloj normal. Pausa y Guardar y menú siguen funcionando.
- Desde Perfiles se crea Jugador 2, cuyo menú muestra 0 CC, separado de Pollito.

Se observaron dos textos confusos, corregidos después: el selector mostraba el
saldo provisional o del jugador anterior en su cabecera; ahora no muestra saldo.
El tablero conservaba «SELECT: HISTORIAL» con Developer activo; ahora indica
«SELECT: DEVELOPER». Estas correcciones de presentación se compilaron y se
verificaron con la integración host; no se volvieron a recorrer en el emulador.

La prueba se hizo sobre una copia recuperable. Se detuvo la emulación mediante
su menú, se movieron los datos de prueba a `build/legacy-update/manual/test-save/`
y se restauraron exactamente los seis archivos originales por SHA-256. La nueva
ranura y la migración de prueba no quedaron en los guardados reales del usuario.

Algunas pulsaciones realizadas inmediatamente tras cambiar pantalla requirieron
una observación y nueva pulsación. No se determina si se debe a la entrega de
entrada de Computadora, la captura o el juego; las pruebas host de transiciones
no reprodujeron ese comportamiento. No se declara investigado ni resuelto.

Esta sesión no valida el teclado nativo de renombrado, eliminación/recuperación
manual, tres ranuras completas, todas las herramientas Developer, IA concurrente
en emulador, sesiones largas, audio DSP ni consola física. Los sanitizadores y
pruebas de almacenamiento tienen evidencia separada.

## Segunda comprobación: 0.9.2 Legacy Preview

Se observó la migración de Pollito con 45 CC, la cabecera PERFILES sin saldo
provisional, SELECT desde Inicio → Modos y el catálogo de diez aperturas.
Explorar mostró la posición inicial de la Italiana, variante 1, 0/12.
Derecha avanzó a e4, turno azul y 1/12. B regresó al catálogo.
Los cambios de tintes/elementos incompletos en algunas capturas Vulkan continúan
observándose; no se declara resuelto ese comportamiento.

No se recorrieron manualmente Carrera, Supervivencia, trofeos ni las veinte
líneas completas. Sus verificaciones actuales son automatizadas. Se detuvo
Azahar desde Emulación → Detener; se conservaron los datos de prueba separados
y se restauraron los seis archivos originales, comparados por SHA-256. Evidencia:
`build/legacy-update/manual-092/restoration.json`, incluido hash del binario probado.
No se realizaron pruebas físicas ni evaluación auditiva.

## Comprobación parcial: 1.1.0 RC Legacy

18 de noviembre, Azahar2126.2/Vulkan. Portada identificada 1.1.0 RC Legacy,
perfil Pollito45CC y tres ranuras. El nuevo icono muestra barras/carga desde la
API emulada: eso no verifica una batería física.

Se abrió SELECT→Modos→Editor y puzzles. Se observó paleta/mini-tablero, se eligió
Borrar y A retiró efectivamente el peón e2. B volvió al taller y luego a Modos.
En Variantes se inició Chess960 índice518. A/cruceta jugó e4 y la IA contestó
Nc6; la posición y notación lo mostraron. SELECT abrió revisión; izquierda
retrocedió a1/2 y el caballo volvió a b8.

La sesión no recorrió manualmente todos los campos FEN, teclado, guardado de
puzzles, pruebas largas ni otras dos variantes: esas verificaciones tienen
evidencia automatizada separada. No se hizo validación física ni auditiva.
Se detuvo mediante Emulación→Detener, se conservó la carpeta de datos de prueba
y se restauraron exactamente los archivos originales por SHA-256. Hash del
binario y restauración: `build/legacy-update/manual-complete/restoration.json`.
