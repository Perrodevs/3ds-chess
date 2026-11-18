# Rivales adicionales — 0.5 RC

Observados el 15 de noviembre de 2026 en el catálogo abierto por el usuario en
Safari: https://www.chess.com/play/computer/ . Los nombres y Elos se leyeron de
las fichas/etiquetas de las miniaturas públicas. Algunas partidas del sitio
requieren suscripción; no se desbloquearon ni se jugaron esas partidas.

| ID estable | Nombre | Elo mostrado en Chess.com | Nivel interno |
|---|---|---:|---:|
| 5 | Martin | 250 | 0 |
| 6 | Wayne | 250 | 0 |
| 7 | Fabian | 250 | 0 |
| 8 | Juan | 400 | 1 |
| 9 | Filip | 400 | 1 |
| 10 | Elani | 400 | 1 |
| 11 | Noel | 550 | 1 |
| 12 | Oliver | 550 | 1 |

Se conservan los cinco personajes anteriores y sus IDs 0–4. El selector tiene
cinco tarjetas por página, tres páginas y navegación circular. El guardado v3
ya contiene un identificador de bot: no se cambió su formato ni se reordenaron
identidades. Las partidas anteriores conservan su rival.

## Retratos y expresiones

Las ocho imágenes fuente y sus direcciones exactas están en
`assets/portraits/chesscom-reference/manifest.json`. Se descargaron los archivos
visibles de las miniaturas públicas; no se extrajo el motor ni código del sitio.
Los retratos se adaptaron con la herramienta integrada de imágenes a pixel art,
en dos hojas de cuatro personajes. Cada personaje tiene normal, pensando,
contento y frustrado. Las expresiones son variaciones adaptadas para este juego,
no una copia verificada de expresiones oficiales de Chess.com.

Las hojas generadas se conservan en `assets/portraits/source/chesscom-0.png` y
`chesscom-1.png`; los prompts exactos están en `chesscom-prompts.json` de esa
carpeta. El conversor toma celdas iguales, compone transparencia sobre el fondo
del juego, reduce con nearest-neighbor y usa 24 colores por personaje.
Atlas de la versión0.5: 128×512 RGBA8, 256 KiB en GPU, 13×4 retratos de 32×32.

## Procedencia

Las imágenes de referencia son arte de Chess.com. No se les atribuye la licencia
MIT del código y no se obtuvo una licencia de redistribución de ese arte. Esta
integración es una copia/adaptación local solicitada por el usuario; no existe
publicación ni afiliación oficial. Antes de publicar recursos copiados o sus
adaptaciones se debe resolver su permiso o sustituirlos por arte propio.

## Fuerza

El número mostrado es **Elo de referencia de Chess.com**, no un resultado de
calibración de nuestro motor. Los nuevos rivales usan los niveles bajos propios
con semillas diferentes. No reproducen el motor, comportamiento, diálogos ni
fuerza exacta del servicio. Las frases de estado son originales y sencillas.

## Ampliación 0.6: los 18 indicados en capturas

Se añadieron exactamente los personajes resaltados por el usuario, con los Elos
que aparecen en sus capturas del 15 de noviembre de 2026. No se interpretan
como ratings FIDE actuales ni se sustituyen por otros valores.

| ID | Personaje | Elo de referencia |
|---|---|---:|
| 13 | Danya | 2650 |
| 14 | Kosteniuk | 2561 |
| 15 | Ding Liren | 2788 |
| 16 | Vishy | 2820 |
| 17 | Bok | 2650 |
| 18 | Lasker | 2640 |
| 19 | Abdusattorov | 2660 |
| 20 | Giri | 2800 |
| 21 | Anna Muzychuk | 2606 |
| 22 | Irina Krush | 2502 |
| 23 | Paul Morphy | 2600 |
| 24 | Vidit | 2730 |
| 25 | Aronian | 2830 |
| 26 | Judit Polgar | 2735 |
| 27 | Hou Yifan | 2686 |
| 28 | Ian | 2795 |
| 29 | Wesley So | 2820 |
| 30 | Capablanca | 2725 |

Todos usan el nivel fuerte propio (5), con semilla de personaje distinta. No
se reproduce su juego real ni el motor del servicio. Se mantienen los 13 IDs
anteriores y el formato de guardado v3. El selector ahora tiene siete páginas.

Las 18 referencias públicas y sus URLs se conservaron en
`assets/portraits/chesscom-reference/masters-manifest.json`. Las expresiones
normal/pensando/contento/frustrado se adaptaron con el generador integrado,
en cinco hojas `assets/portraits/source/masters-0.png` a `masters-4.png`.
Los prompts se conservan en `masters-prompts.json`.

El atlas actual completo es128×1024 RGBA8 (512KiB en GPU):31 personajes,124 celdas
de 32×32. Las primeras 13 filas se preservaron. Continúa aplicando la procedencia
y exclusión de MIT del arte derivado de Chess.com explicada arriba.
