# Catálogo ES/EN de herramientas y modos Legacy

Se redactaron **420 claves ES/EN** para este conjunto de módulos. Tras eliminar
las coincidencias ya presentes en los otros catálogos,
`include/localization/tools_catalog.inc` conserva **396 claves propias** con
traducciones inglesas revisadas. Es un fragmento de inicializadores
`{"texto fuente", "traduccion"}` para el catálogo central; no cambia código de
reglas, cadenas internas, formatos ni recursos.

Cobertura de los literales visibles inspeccionados:

- Taller: editor, paleta, FEN, biblioteca de posiciones/puzzles, objetivos,
  confirmaciones, teclado, demostración, pistas, errores y contadores.
- Variantes: configuración, enroque Chess960, tres jaques, objetivo central,
  selección, pausa, resultados, revisión y errores de búsqueda/guardado.
- Carrera, Supervivencia, sala de trofeos y las 19 descripciones de trofeo.
- Exploración y práctica de aperturas: menús, favoritos y avisos.
- Developer y perfiles: avisos; también errores del gestor de perfiles que se
  concatenan en la interfaz, aunque proceden de `storage/profiles.cpp`.
- Revisión: explicación de resultados, material, sugerencias y límites de análisis.
- Escena de resultado: titulares y bandos dorado/azul.
- Razones de finalización clásicas, de variantes y de reloj; límites FEN y objetivos
  del creador; nombres de divisiones usados por Carrera.
- `preferences.cpp` solo contiene formatos UI01/UI02: no tiene texto visible.

No se traducen FEN, UCI, SAN almacenado, claves de posición, O-O/O-O-O,
COMPOSER1/VARIANTS1/PROFILES1, CHECK, archivos ni etiquetas PGN. `Review` en el
presupuesto de búsqueda y `Tiempo`/`tiempo` de comparación interna tampoco se
sustituyen. Chess960/Three-check/King of the Hill conservan su nombre de variante.
No se traduce contenido creado por el usuario: nombres de perfiles, puzzles o
posiciones. «Posicion» y «Jugador » del catálogo son etiquetas/predeterminados,
no permiso para modificar un nombre que coincida con ellas.

## Integración central necesaria

Además de coincidencias exactas, hay fragmentos para los textos con datos:
`Mis posiciones (`, `Turno: `, `Pista: `, `Objetivo: `, `Solucion: `, `Intentos `,
` / completados `, `Semilla `, `JAQUES B: `, `CARRERA: `, `DIVISION: `, etc.
La capa central debe combinarlos únicamente en estructuras conocidas, nunca
mediante reemplazos indiscriminados dentro de nombres propios.

Casos que requieren conservar sus datos y traducir la estructura completa:

- `V <wins> / D <losses> / T <draws>` → `W ... / L ... / D ...`.
- `Material final: blancas %d / negras %d`.
- `Perdida aprox: %d.%02d peones`.
- `<pieza> en <casilla>` y `Da jaque: <pieza> en <casilla>.`.
- `<nombre de usuario> / <estado del puzzle>`: traducir el estado aparte.
- Prefijos numéricos, indicadores de página y sufijos de puntos/monedas: mantener
  números, identificadores, coordenadas y unidades que no cambian.

Traducir antes de cortar/envolver cadenas: el taller corta algunos errores en
23 caracteres; la revisión corta razones a 44 y algunas frases se envuelven
antes de dibujar. `end_scene.cpp::centered` debe medir la traducción, no el
original español. El idioma de SAN mostrado se elige en la presentación, sin
reescribir movimientos ni partidas guardadas.

## Verificación ejecutada para este fragmento

Se extrajeron y cotejaron los literales de los módulos enumerados. Las exclusiones
fueron únicamente sintaxis de datos, identificadores y cadenas internas indicadas
arriba. Un programa host incluyó el `.inc` como inicializadores C++17 y comprobó
420 claves originales únicas y ambas cadenas no vacías antes de la deduplicación
entre módulos:
`build/legacy-update/check-tools-catalog.cpp` y su ejecutable.

Esto verifica el catálogo y su sintaxis; las pruebas del selector de idioma,
composición de textos, cobertura de otros módulos y capturas ES/EN pertenecen a
la integración central. No se declara una comprobación física de legibilidad.

## Auditoría estática de integración

`tests/test_localization_catalog.py` recorre los tres catálogos y comprueba
cuatro contratos: claves/traducciones no vacías y sin duplicados globales, tipos
y orden de conversiones printf preservados, caracteres disponibles en la fuente
ES/EN, y longitud de controles concretos de una línea. Los cuatro casos pasaron.
No impone un límite artificial de 49 caracteres a los párrafos educativos.

El cotejo de literales de todas las fuentes confirmó que los niveles
Bajo/Normal/Fuerte y el límite de torneo tienen traducción. Se señalaron para
integración la frase de error de progreso secreto y la cadena completa de
`drawRules()` del torneo; la interfaz actual usa una versión breve distinta.
Se verificó después de su corrección que perfiles, nombres personalizados de
colecciones y FEN usan representación literal; las etiquetas que los acompañan
se traducen por separado. El teclado de perfil traduce su indicación antes de
llamar al servicio nativo. Los ejemplos numéricos W/L/D, material final, pérdida
y jaque por una pieza se comprobaron con el traductor real.

La inspección de llamadas directas con literal y límite numérico de caracteres
no encontró traducciones inglesas que excedieran ese límite. Esta revisión no
demuestra el ajuste visual de todas las combinaciones dinámicas ni reemplaza
las capturas y pruebas de ejecución del agente integrador.
