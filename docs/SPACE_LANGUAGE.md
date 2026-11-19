# 3DS Chess — Space & Language Update (informe RC1)

La continuación RC2 se documenta en [ELON_COMPLETE.md](ELON_COMPLETE.md) y
[ELON_TEST_RESULTS.json](ELON_TEST_RESULTS.json). Los resultados RC1 de este
informe se conservan como evidencia histórica.

## Estado

Actualización sobre 1.1.0 RC Legacy, en `feature/space-language-update`.
El código anterior, sus commits, copias y recursos privados se conservan. No se
publicó, no se añadieron remotos y no se hicieron pruebas físicas.
La candidata es **1.2.0 RC1**, identificada en `include/app_version.hpp`.

## Rival secreto

El catálogo tiene41rivales, IDs estables0–40. Elon ocupa40, categoría invitado
especial ficticio, nivel Fuerte4 y preferencia agresiva del motor existente. No
se atribuye su comportamiento ni sus frases a la persona real.

Se mantienen los160retratos previos del atlas y se añaden cuatro expresiones
32×32 originales dibujadas por código: neutral, pensando, contento y frustrado.
Hay42temas procedurales originales contando el menú y los41rivales. Se incluyen
los nueve eventos ES/EN propuestos, adaptados al espacio de pantalla.

Se desbloquea al vencer a Byte2 y Jensen39 en partidas competitivas válidas.
Práctica/local/entrenamiento/torneo y abandonos tempranos no dan ese progreso.
Un mate legal temprano sí puede contar, conforme a las reglas económicas ya
existentes. Solo se reconocen victorias antiguas mediante partidas archivadas
conservadas y verificables: los totales estadísticos no permiten distinguir todos
los abandonos, y no se inventan victorias cuando el registro fue eliminado.

El selector tiene nueve páginas; la última contiene solo el rival40 y no acepta
casillas vacías. El rival bloqueado no puede confirmarse ni iniciarse. Los pools
aleatorios de Carrera y torneos conservan los40rivales públicos para no sortear
un secreto todavía bloqueado. La selección explícita desbloqueada sigue siendo
compatible con las partidas y el torneo existente.

## Persistencia y escenas

`space.dat` por perfil tiene formato versionado, checksum, temporal y respaldo.
Registra flags Byte/Jensen, desbloqueo, anuncio y alternancia de escenas. Solo un
resultado competitivo verificado y liquidado con ID nuevo actualiza esos datos.
No cambia resultados, monedas ni logros. El anuncio se marca en disco antes de
mostrar la animación discreta; saltar o recargar no duplica premios ni progreso.

Las victorias válidas de Elon alternan Roadster/cohete: primera, tercera, etc.,
Roadster; segunda, cuarta, etc., cohete. Una victoria del jugador usa el cohete
con despegue fallido. Tablas, práctica y resultados duplicados no avanzan la
alternancia competitiva. El estado guardado no ordena reproducir escenas viejas
al abrir repetidamente un resultado.

Las escenas usan rectángulos, estrellas, siluetas y el retrato del rival:
Roadster180frames (~3s a60FPS), cohete240(~4s) y lanzamiento fallido120(~2s).
A/B saltan; movimiento/render siguen en el bucle normal, sin vídeo ni esperas.
No se amplía la pila ni se carga una textura nueva. Los otros rivales conservan
sus escenas originales. La velocidad real de consola no está medida.

El perfil económico se confirma antes del sidecar. Si falla el guardado secreto,
se mantiene el resultado real, se muestra el error y se reintenta antes de iniciar
otra partida. No se vuelve a pagar el mismo resultado. No se promete atomicidad
universal entre archivos FAT ni resistencia absoluta a cortes de energía.

## Idiomas

Sistema central ES/EN, selector inicial, preferencia global y elección editable
por perfil en Opciones → Idioma. Cambio instantáneo sin alterar partida ni
notación; persistencia, respaldo y migración independiente de los formatos de
partida. FEN/UCI/PGN no se traducen como datos y los nombres personalizados se
conservan literalmente. La biblioteca lee v1(40rivales) y escribe v2(41); los
lectores antiguos no conocen v2 ni el ID40, por lo que volver a ellos requiere
restaurar la copia previa correspondiente, sin promesa de compatibilidad inversa.

Los catálogos cubren menús, diálogos, lecciones, editor/puzzles, variantes,
progresión, tienda, estadísticas, ayuda, errores, batería y cinemáticas. La fuente
incluye acentos, ñ y signos invertidos. `LOCALIZATION.md` detalla arquitectura,
prioridades, recursos y límites; las auditorías de contenido y herramientas
incluyen las comprobaciones de tamaño realizadas.

## Compilaciones ES y EN

Ambas proceden del mismo código. Solo cambia el idioma inicial predeterminado:
`DEFAULT_LANGUAGE=ES` o `EN`. La elección explícita guardada tiene prioridad.
Los paquetes `3DS-Chess-ES.zip` y `3DS-Chess-EN.zip` son alternativas: usan la misma
ruta `SD:/3ds/3ds-chess/` y los mismos datos. Copiar el otro paquete reemplaza el
ejecutable, no crea un segundo conjunto de perfiles. No se promete coexistencia
independiente; los ZIP no llevan guardados ni los sobrescriben deliberadamente.

Ejecutables y SMDH por idioma están en `dist/languages/ES/` y `EN/`. El ZIP de
fuentes y `SHA256SUMS-LANGUAGES` se generan junto a los dos paquetes. Los recursos
anteriores de derechos mixtos mantienen la condición **solo pruebas locales**;
no se genera CIA, no se publica y no se distribuye el paquete privado de piezas.

## Verificación y pendientes

Comandos, resultados, fallos de integración investigados, hashes y cobertura están
en `SPACE_LANGUAGE_TEST_RESULTS.json`. Hay pruebas de catálogo, glifos, espacios,
perfiles, cambios/reinicio, migración, desbloqueo, duplicados, historial confiable,
fallos SD, selección41, escenas, salto, resultados y liberación de recursos.
Se conservan las regresiones previas y el perft inicial4=197281.

Pendientes: pruebas físicas Old3DSXL, teclado y táctil reales, rendimiento/fluidez,
audio, suspensión y SD bajo condiciones físicas. No se ejecutó Azahar para esta actualización; se conservó la autorización de
continuar sin él. Los sanitizadores de host no sustituyen esas pruebas. Los artefactos
anteriores de captura/tinte de Vulkan no se declaran corregidos.

## Resultado de entrega

Pasaron 42 programas host de regresión, 7 pruebas Python de catálogos y 4 de
empaquetado. ASan/UBSan pasaron en integración de idiomas/perfiles y rival/escenas;
la prueba independiente del progreso secreto también pasó con sanitizadores.
La compilación y comprobación de los 36 objetos ARMv6K pasó para ES y EN.
Los ZIP se verificaron completos, con hashes correctos, sin guardados ni recursos
privados; las únicas diferencias entre ellos son ejecutable y SMDH.

No se midieron FPS, consumo ni RAM de consola. Las duraciones verificadas son
180/240/120 actualizaciones del bucle, no mediciones de segundos en hardware.
No se hizo recorrido manual de todas las pantallas; los límites estáticos y
comprobaciones de render host no demuestran legibilidad completa en consola.

Veredicto: **LISTO PARA PRUEBAS FÍSICAS**, como candidata local 1.2.0 RC1.
No es una versión declarada estable ni autorizada para redistribuir sus recursos.
