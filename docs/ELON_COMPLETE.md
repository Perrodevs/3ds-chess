# Elon Musk — auditoría de personaje y escenas, 1.2.0 RC2

## 1. Estado encontrado y alcance

Elon **ya existía** en 1.2.0 RC1. No se duplicó ni se reordenó el catálogo.
Se reutilizó `feature/space-language-update`, cuya integración es la base más
reciente; `main` y `origin/main` siguen en 1.1.0 RC Legacy. No se hizo merge ni push.
La candidata RC2 incorpora únicamente las diferencias y correcciones verificadas.
Los paquetes anteriores RC1 se conservaron en `build/elon-audit/recovery-rc1/`.

## 2. Catálogo y características

Hay **41 rivales**, IDs permanentes 0–40; Elon es 40, Byte 2 y Jensen 39.
El selector tiene nueve páginas y solo una casilla real en la última.
Elon es un invitado tecnológico ficticio, nivel **Fuerte (interno 4)**, con
preferencia Agresivo y semilla propia en el motor C++ existente. «Experimental»
es personalidad ficticia, no otro algoritmo ni una promesa de Elo real.

Tiene cuatro retratos originales dibujados por código: neutral, pensando, feliz
y frustrado. No se modifica el atlas de los 40 anteriores ni se carga otra textura.
Su tema es síntesis original y distinto en las pruebas de muestras: 42 temas en
total, menú incluido. Esto no es una evaluación auditiva en consola.

## 3. Desbloqueo y tres perfiles

Requiere vencer a Byte y Jensen en partidas competitivas válidas contra IA,
desde la posición inicial clásica y con historial legal verificable y liquidado.
Práctica, local, entrenamiento, torneos, resultados repetidos y abandonos/acuerdos
antes de ocho medias jugadas no cuentan. Un mate legal corto sí cuenta.
Solo se reconocen victorias antiguas si permanece una entrada fiable en biblioteca.

`space.dat` pertenece a cada generación de perfil. Guarda ambos flags, anuncio,
contador de victorias de Elon e IDs procesados. Se probaron los tres directorios
reales de perfiles: uno desbloqueado, otro con Jensen y otro nuevo, sin compartir
progreso. El anuncio ES/EN se confirma en disco antes de mostrarse y no paga monedas.
No se cambió el formato SPACE1 ni el formato principal de partidas en RC2.

## 4. Los nueve diálogos

Son ficción humorística, no citas de la persona real. El catálogo central los
presenta en el idioma activo:

| Evento | Español | English |
|---|---|---|
| Saludo | ¿Solo 64 casillas? | Only 64 squares? |
| Pensando | Necesito más cómputo. | Need more compute. |
| Captura | Movimiento optimizado. | Move optimized. |
| Jaque | Tu rey necesita un parche. | Your king needs a patch. |
| Victoria | Ajedrez demasiado simple. | Chess is too simple. |
| Derrota | Error de simulación. | Simulation error. |
| Puzzle inicial | ¿Y el árbol tecnológico? | Where's the tech tree? |
| Puzzle resuelto | Grok ya lo sabía. | Grok knew that already. |
| Error de puzzle | Necesito más datos. | I need more data. |

No se insertó a Elon como mentor de los puzzles fijos: sus eventos existen en el
catálogo y se prueban, pero esos puzzles mantienen sus mentores anteriores.

## 5. Interacciones añadidas

`ai::rivalInteraction(speaker, opponent, event)` devuelve una frase opcional;
los otros pares/eventos conservan su comportamiento. Para Elon/Jensen:
«Necesito más VRAM» y «Ese cálculo usa demasiados CUDA». Para Elon/Byte:
«¿Quién programó a este robot?» y «Necesito actualizar mi IA», ambas parejas ES/EN.

Aparecen en la línea inferior del cuadro de torneos solo si **el emparejamiento
actual IA contra IA** contiene a esos rivales; saludo antes de la primera jugada
y comentario de cálculo después. Los avisos importantes tienen prioridad. No hay
popup, búsquedas nuevas ni rivales añadidos automáticamente. Elon solo participa
si se seleccionó explícitamente una vez desbloqueado; los pools aleatorios siguen
usando los 40 públicos. La prueba usa un cuadro sintético válido, no afirma haber
jugado un torneo completo manualmente.

## 6. Tres cinemáticas y alternancia

- **Roadster, 180 actualizaciones:** automóvil rojo entra desde un lateral,
  asciende, cruza el espacio con estrellas y sale. Texto final revelado por letras:
  «DERROTA EN ÓRBITA» / «DEFEAT IN ORBIT».
- **Cohete, 240 actualizaciones:** plataforma, encendido, llamas/humo, retrato en
  ventanilla, despegue acelerado, transición gradual al espacio y salida. Texto:
  «SOLO 64 CASILLAS. QUÉ SIMPLE.» / «ONLY 64 SQUARES. TOO EASY.».
- **Fallo, 120 actualizaciones:** subida corta, detención, error parpadeante,
  descenso suave al mismo sitio y expresión frustrada. Texto:
  «EL AJEDREZ NO ERA TAN SIMPLE.» / «CHESS WASN'T THAT SIMPLE.».

A/B saltan y la accesibilidad de movimiento reducido sigue funcionando. No hay
vídeos, hilos propios, nuevas texturas ni aumento de pila. Las duraciones de
~3/4/2 segundos suponen 60 actualizaciones por segundo; no se midieron en hardware.
El texto final queda visible con el menú de resultado hasta otra acción.

Las derrotas competitivas humanas alternan Roadster/cohete por perfil, incluso
entre sesiones. Las victorias humanas usan el fallo sin avanzar el contador.
Práctica, tablas y simulaciones de torneo no lo avanzan. La escena no modifica
partida, estadísticas ni recompensas. Se rechazan solicitudes incompatibles con
el resultado o sin un humano válido. Otros personajes conservan sus animaciones.

## 7. Problemas encontrados y correcciones

- Faltaban interacciones contextuales: añadidas modularmente al cuadro existente.
- Cuatro traducciones/frases no coincidían exactamente con el nuevo prompt:
  ajustadas y comprobadas evento por evento.
- El cielo cambiaba bruscamente; ahora usa una transición por colores.
- El fallo tenía un salto de posición al detenerse; ahora sube, se mantiene y
  desciende sin saltos, con error visual y rostro frustrado.
- El texto aparecía desde el principio; ahora se revela al final, sin cortar UTF-8.
- La punta del cohete se estrechaba hacia abajo, detectado al revisar el render
  host: corregida y cubierta con comprobación de geometría dibujada.
- `EndScene::begin` aceptaba una escena especial incoherente si el llamador se la
  pasaba: ahora valida resultado, modo y color humano además del rival.

Durante la creación de una prueba se usó inicialmente una API de lista de
movimientos inexistente; se corrigió al iterador real. Otra prueba detectó las
traducciones distintas mencionadas. Esos fallos no se ignoraron para cerrar verde.

## 8. Pruebas, estados y límites

Los comandos, resultados y hashes de logs están en `ELON_TEST_RESULTS.json`.
La línea base de 42 programas pasó antes de modificar código; la regresión pasó
con 43, incluyendo el nuevo test de diálogo. El perft inicial de profundidad 4
sigue dando **197281**. Tras corregir la punta se repitieron las pruebas afectadas
de escenas, resultado, integración y compilación ARMv6K.

ASan/UBSan pasó en diálogo, escenas, preferencias/idiomas y resultado/guardado de
Elon. Se añadieron contratos de entrada/salida, transición, descenso, expresión,
caption, resultado coherente, revisión/PGN/biblioteca, estadísticas, revancha y
práctica sin monedas. Se conservan pruebas de 41 selecciones, cancelación de IA,
recuperación, truncamientos y seis fallos de escritura del progreso secreto.

| Estado | Alcance |
|---|---|
| Implementado y probado automáticamente | Catálogo, selección, IA/nivel, desbloqueo, tres perfiles, nueve eventos ES/EN, cuatro interacciones, alternancia, salto, resultados, biblioteca/PGN, revancha y persistencia |
| Revisión visual de host | Fotogramas de las tres escenas en ambos idiomas, generados con el renderizador compartido; no son capturas de consola |
| Revisión parcial en Azahar | Ver registro manual al final y JSON; no equivale a un recorrido completo |
| Pendiente físico | Old3DSXL: fluidez, lectura nativa, botones/táctil, audio, suspensión y SD real |
| No aplicable / no añadido | Espectador independiente (no existe), campaña nueva de Elon, online, CIA, Elo real |

## 9. Archivos principales y arquitectura

- `include/ai/bots.hpp`, `source/ai/bots.cpp`: API opcional y textos del rival.
- `include/localization/content_catalog.inc`: traducciones centralizadas.
- `source/ui/app.cpp`: comentario contextual en la línea existente del torneo.
- `include/ui/end_scene.hpp`, `source/ui/end_scene.cpp`: timeline determinista,
  render por primitivas y guardas de resultado; estructura temporal pequeña.
- `tests/test_elon_dialogue.cpp`, `test_space_scene.cpp`, `test_space_secret.cpp`,
  `test_space_ui.cpp`: nuevas regresiones/integración.
- `tests/make_elon_fixtures.cpp`: generador host de casos sintéticos aislados y
  fotogramas; no se integra como opción del juego ni concede progreso real.
- `Makefile`: pruebas, sanitizadores y generador; versión en `app_version.hpp`.
- README, CHANGELOG, BOTS y estos informes: estado real actualizado.

## 10. Paquetes y Git

ES y EN comparten código, catálogos y guardados; solo cambia el idioma inicial.
Ambas permiten cambiar idioma y respetan la preferencia guardada de cada perfil.
Son alternativas en la misma ruta SD, no dos instalaciones independientes.
`dist/3DS-Chess-ES.zip`, `dist/3DS-Chess-EN.zip`, `dist/3ds-chess-source.zip`,
`dist/languages/{ES,EN}/3ds-chess.{3dsx,smdh}` y `SHA256SUMS-LANGUAGES`.
La publicación de recursos anteriores sigue sin estar autorizada: uso local.

Commits funcionales de esta auditoría:

- `4fbedd4 feat(dialogue): add contextual technology rival exchanges`
- `c88fc2f feat(cinematics): complete staged space sequences and result guards`

El commit de entrega identifica RC2 y conserva los dos anteriores. No se cambió
ningún commit inicial, no se subieron ramas ni etiquetas a GitHub. La etiqueta
RC1 previa se mantiene en su commit y no se mueve a código diferente.

## 11. Registro manual de Azahar

18 de noviembre de 2026, Azahar 2126.2/Vulkan, usando Computadora. Tras un intento
inicial sin acceso al clic/teclado, el propietario dejó la app al frente y se
reintentó. Se observaron portada RC2, menús ES/EN, Roadster moviéndose y su título
final en español; cohete con ventana/llamas y desenlace espacial en inglés; y
lanzamiento fallido con rostro frustrado, título español y resultado VICTORIA.

Se pulsó Revancha rápida: regresó a posición inicial con Elon. Pausa por táctil
y B (tecla s sostenida 120ms) regresó al tablero. A/cruceta jugó e4 y la IA
contestó e5; ambos movimientos se vieron en tablero e historial. El saldo de la
prueba permaneció 210 CC después de escena, revancha y esos movimientos.

Los casos se prepararon con **fixtures sintéticos**, no victorias ganadas por el
propietario ni partidas completas jugadas manualmente. Se conservaron separados.
Los intentos con teclas muy breves durante la escena no constituyen prueba manual
concluyente de salto; A/B sí pasan las pruebas automatizadas. Las capturas Vulkan
a veces salieron negras/incompletas; no se declara resuelto ese artefacto.
No se capturó cada fotograma ni se evaluó el sonido.

Se detuvo Azahar desde Emulación → Detener y se restauraron los seis archivos
originales exactos por SHA-256. Evidencia local:
`build/elon-audit/manual/restoration-final.json`, que incluye hash del 3DSX probado.
No hubo pruebas físicas. **Veredicto: LISTO PARA PRUEBAS FÍSICAS como RC local**,
sin afirmar estabilidad en consola ni autorización de distribución de recursos.
