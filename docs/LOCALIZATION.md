# Español e inglés

## Catálogo y presentación

El módulo `localization` centraliza claves de texto fuente y traducciones inglesas
en tres tablas estáticas: interfaz, contenido educativo/diálogos y herramientas.
No utiliza red ni dependencias externas. Los nombres propios son invariantes;
los nombres personalizados y FEN usan `Renderer::rawText`, evitando traducirlos.

`Renderer::text` aplica el catálogo al texto completo. Los párrafos se traducen
antes de envolverlos y los títulos antes de medir su centrado. Los formatos
numéricos `%d`/`%02d` preservan sus argumentos; los textos compuestos usan fragmentos
con límites de palabra y prioridad de coincidencia más larga. Los resultados
compuestos tienen una caché acotada de64 entradas (fuentes de hasta512bytes), no
una caché creciente. Las tablas y búsqueda ordenada se preparan una vez. El modelo
interno de reglas, razones persistidas, FEN, UCI y SAN no se traducen en disco.

Esta arquitectura usa claves fuente al estilo gettext, sin introducir una copia
ES/EN de cada menú. Los catálogos están en el mismo código de ambos paquetes. Las
pruebas rechazan claves duplicadas, traducciones vacías, formatos incompatibles y
glifos no admitidos. Las claves de40rivales originales se conservan; hay41rivalIDs.

La fuente bitmap ahora decodifica UTF-8 y dispone de16glifos adicionales:
`áéíóúüñÁÉÍÓÚÜÑ¿¡`. Se generan en celdas libres antes de los sprites; no se amplía
la textura256×256. Los demás caracteres ajenos a la fuente siguen usando `?`.
Los nombres de perfil/puzzle mantienen su límite ASCII existente; esto no cambia
los formatos antiguos ni introduce nombres UTF-8 que sus lectores rechazarían.

## Preferencias y prioridad

1. Primera ejecución sin preferencia global válida: selector bilingüe, cruceta/A
   o pantalla táctil. B no cancela la decisión inicial.
2. `global-language.dat` conserva la elección global. El idioma predeterminado
   del paquete solo se utiliza antes de una elección guardada.
3. Un perfil sin preferencia hereda el valor global. `language.dat` de su directorio
   conserva la elección propia; otros perfiles no la comparten.
4. Opciones → Idioma cambia la presentación al instante. No reinicia ni modifica
   movimientos, resultado, monedas o notación.

Los archivos `LANG1` son de8bytes, con validación, temporal y respaldo. La carga
prueba el respaldo; la escritura comprueba sus resultados. Un archivo personal
inválido no provoca el borrado del perfil normal; se muestra un aviso y se usa el
valor global hasta una elección explícita. No se promete durabilidad absoluta
ante cualquier corte de energía.

La notación española/internacional sigue siendo una preferencia independiente en
Opciones. Cambiar idioma no la cambia automáticamente. PGN usa las reglas del
exportador existente, con SAN internacional; no se traduce FEN/UCI ni se cambian
nombres de jugadores en exportaciones.

## Compilación y distribución

`make DEFAULT_LANGUAGE=ES all` y `make DEFAULT_LANGUAGE=EN all` compilan el mismo
árbol con distinto valor inicial. Una marca de compilación invalida los objetos
y SMDH al cambiar ese valor, evitando mezclar objetos de idiomas distintos.

`tools/build_language_packages.py --local-testing` genera ambos ZIP, ejecutables,
SMDH, código fuente y hashes. Son **alternativas de instalación** con las mismas
rutas en SD; instalar el segundo reemplaza los ejecutables del primero. No se
prometen dos instalaciones independientes simultáneas. Ambos utilizan los mismos
guardados y respetan idiomas ya seleccionados, por lo que cambiar de paquete no
reinicia perfiles ni cambia preferencias guardadas. No contienen archivos `.dat`.

Las imágenes de derechos mixtos siguen limitadas a pruebas locales. El recurso
nuevo de Elon y sus vehículos se dibuja con código propio; no hay fotografía ni
vídeo descargado. Los recursos privados anteriores permanecen separados.

## Validación y límites

Los catálogos incluyen menús, perfiles, rivales,450eventos de diálogo, tienda,
logros, estadísticas, academia, puzzles, campaña, retos, Carrera, Supervivencia,
torneos, aperturas, editor, variantes, biblioteca, resultados, revisión, errores,
confirmaciones, batería y ayuda. Los detalles de727contratos de contenido están
en `LOCALIZATION_CONTENT.md`; la auditoría de herramientas, en
`LOCALIZATION_TOOLS.md`. Las pruebas de ejecución cubren cambio instantáneo,
reinicio, separación entre perfiles, nombres literales, notación independiente,
fallos de escritura y migración de biblioteca v1 a v2.

La inspección automatizada de tamaños y cualquier recorrido parcial de Azahar no
sustituyen revisar las pantallas de una Old 3DS XL física. No se han validado
físicamente el teclado, fluidez, sonido ni lectura de SD de esta actualización.

50 Rivals añade81 eventos ES/EN, nueve rivalidades y los textos breves de NULL.
