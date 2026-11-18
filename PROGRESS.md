# Estado vigente — 0.9.0 RC

Academia10, torneos4/8, biblioteca30, estadisticas nuevas y seis estilos implementados.
Consultar docs/UPDATE_090.md y docs/CHANGELOG_090.md para evidencia y limites.
Se conserva build/update-090/baseline-082.zip con SHA-256. Sin pruebas fisicas.
Los apartados siguientes son historial, no el inventario vigente.

# Estado vigente — 0.8.2 RC

Estabilización auditada. Consultar docs/AUDIT_082.md y docs/CHANGELOG_082.md.
Los apartados inferiores conservan la historia de desarrollo; no sustituyen
el inventario actual. Sin validación física ni redistribución pública autorizada.

# Actualización 0.8.1 — Jensen Huang

40 rivales, 160 expresiones y 360 diálogos originales breves. Jensen ocupa el nuevo
ID39, sin cambiar identidades guardadas. Tema musical propio y etiqueta fundador
de NVIDIA. No se añadió un segundo personaje. El atlas conserva el mismo tamaño.

# 3DS Chess 0.8.0 RC — entrenamiento y revisión

## Entrenamiento y revisión (0.8 RC)

- Práctica sin reloj, monedas ni logros, con deshacer una decisión humana y la respuesta de IA.
- Revisión por media jugada: inicio/final, explicación del mate/tiempo/rendición, atacante del rey,
  alternativa del motor y pérdida estimada con búsqueda corta. No ofrece precisión calibrada.
- Revancha con mismo rival/color/modo/reloj; capturas reconstruidas, incluso al paso/promoción.
- Tablero girado al jugar negras, con coordenadas, controles, flechas y animación coherentes.
- 8 puzzles, 4 retos (coronación, al paso, enroque, mate con negras) y campaña de 8 rivales invitados.
- Progreso de entrenamiento independiente con respaldo; restauración de partida normal al salir.
- 351 diálogos originales y 40 bucles musicales originales (menú + 39 rivales).
- Pila principal ampliada de32a256KiB: evita un bloqueo ARM al analizar partidas largas.


Estado real de 2026-11-15. Implementación y compilación hechas; **no certificada
como estable en una Old Nintendo 3DS XL física**.

## Implementado

- Cinco rivales: Pollito/Bajo, Hamchow/Normal, Byte/Fuerte, Magnus/Fuerte e
  Hikaru Nakamura/Fuerte. Tres rangos relativos; no se promete ELO humano.
- Selector con miniaturas, ficha y control físico/táctil. Selección persistente
  separada del rival de la partida existente; el modo local sigue siendo humano.
- Veinte retratos (cuatro expresiones por bot), procesados a32×32 con nearest.
  Cambian al pensar, capturar, dar/recibir jaque y terminar la partida.
- Pollito/Hamchow/Byte originales; los dos homenajes adaptan fotos reales CC BY4.0.
  Atribuciones, licencia, prompts exactos y hojas fuente conservados.
- Frases ficticias y preferencias iniciales distintas; los fuertes comparten el
  motor principal, no simulan exactamente estilos ni capacidades humanas.
- Motor original mejorado: evaluación por fase, rey/peones/movilidad, PVS,
  ordenación y tabla512KiB en heap con contexto de repetición/50movimientos.
- Guardado v3 con migración v1/v2 a partidas sin límite sin perder posición, monedas, inventario o premios.
- Se mantienen tienda, logros, perfil, SAN, PGN, audio procedural y guardado seguro.

## Relojes opcionales (0.3 RC)

- Sin límite es el valor inicial. Controles 3 min, 5 min, 10 min y 5 min + 3 s seleccionables.
- Reloj independiente por lado, incremento solo tras movimiento legal y caída exacta.
- Pausa/suspensión detienen ambos; promoción y cálculo IA sí consumen tiempo.
- Tiempo restante persistido y cargado sin cobrar el intervalo fuera del juego.
- IA adapta búsqueda y no aplica respuestas tardías; resultado por tiempo con
  excepción de material y premio idempotente. PGN conserva TimeControl.
- Integración probada automáticamente en Mac y compilada ARMv6K. Falta hardware.

## Verificación

- Compilación real ARMv6K y `.3dsx`.
- Pruebas perft y comparación externa de3000posiciones para reglas/SAN/FEN.
- UI: cinco selecciones, persistencia, cancelación, apertura e identidades distintas.
- Veinte expresiones32×32 diferentes y paleta limitada; layout inspeccionado en Mac.
- Suite táctica, presupuestos, cancelación, historial con TT activada/desactivada,
  promoción, terminales y horizonte de50movimientos.
- Comparación frente al motor anterior en20posiciones con igual presupuesto:
  mejora >30cp en5, diferencias dentro de±30cp en15, sin pérdida >30cp. Muestra
  pequeña; no es un torneo ni permite deducir ELO. Metodología en ENGINE_UPGRADE.md.
- Partida completa entre IAs y recarga sin duplicar premio, en las pruebas nativas.
- Sanitizadores de reglas, IA, interfaz y selección de bots.

Registro de esta versión: `build/bots-final-verification.log`, copiado a
`docs/verification-build.log`. Estado detallado y límites en TESTING.md.

## Pendiente de validación externa

La prueba de Azahar de la versión anterior verificó pantalla, jugada, respuesta
IA y escritura SD. Para0.2 la Mac bloqueada impidió abrir el emulador; no se
presentan los previews de Mac como prueba GPU/DSP ni de hardware físico.

Faltan prueba real Old3DS XL, mediciones de memoria máxima/batería/tiempos reales,
audio/suspensión prolongados y detección general de posiciones muertas complejas.
El formato CIA sigue sin generar por falta de herramientas. Los sprites del ZIP
Cosunosuke siguen solo en la compilación personal con licencia sin confirmar.

Las fuentes y paquete SD se preparan localmente; no se publicó nada en internet.

## Pulido 0.4 RC

Inicio sin partida automática, guía de cuatro páginas, pistas offline con X o botón
y flecha, animación de piezas perdedoras y resultado explícito con dama central.
Probados resultados de ambos colores, pausa/cancelación de pistas y relojes en Mac.
Inicio, ayuda, derrota por mate y pista Cc3 seguida manualmente se observaron en
Azahar 2126.2; no es una certificación de Old 3DS física.

## Rivales adicionales (0.5 RC)

Trece identidades estables, ocho nombres y Elos consultados en Safari, cuatro
expresiones adaptadas por nuevo personaje, selector de tres páginas, metadatos
de procedencia y Elo de referencia. Se conserva formato de guardado v3.

## Los18rivales de las capturas (0.6 RC)

31personajes,124retratos, siete páginas. Los nombres/Elos se conservaron tal como
aparecen en las capturas. Los18nuevos son fuertes usando el motor propio; no se
promete equivalencia humana. Identidades anteriores y guardado v3 preservados.

## Personajes invitados (0.7 RC)

Se añaden Leon/Jill/Claire/Chris, Joel/Ellie y Steve/Alex.39rivales,156expresiones,
ocho páginas. Sin Elo inventado para personajes ficticios. Atlas256×1024 en dos
bancos, manteniendo los retratos anteriores.
