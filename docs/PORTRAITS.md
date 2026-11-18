# Retratos de rivales — 8-bit

Se generaron cinco personajes con cuatro expresiones cada uno usando la herramienta
integrada **OpenAI image_gen** (sin API externa configurada por el usuario).
Los prompts completos se conservan en `assets/portraits/prompts.json`; los cinco
PNG originales generados están en `assets/portraits/source/`.

## Atlas y expresiones

`assets/portraits/portraits.png` mide **128×256 RGBA**. Ocupa **128 KiB** como
textura GPU RGBA8. Su versión para C++ está en `portraits.hpp`, namespace
`portraits`, array `atlas[32768]`, componente rojo en el byte menos significativo.
Cada retrato es de 32×32 píxeles; puede ampliarse a 64×64 con nearest-neighbor.

| Fila / ID | Personaje | Diseño |
|---|---|---|
| 0 | Pollito | Pollito amarillo original, pico naranja y pañuelo rojo |
| 1 | Hamchow | Ajedrecista chino ficticio, pelo negro, gafas y sudadera verde azulado |
| 2 | Byte | Robot original de acero azul con luces cian y detalles dorados |
| 3 | Magnus | Homenaje visual a Magnus Carlsen basado en fotografía real |
| 4 | Hikaru | Homenaje visual a Hikaru Nakamura basado en fotografía real |

Columnas, en este orden: **0 neutral**, **1 pensando**, **2 contento**,
**3 frustrado**. La celda se obtiene en `(expresión*32, personaje*32, 32, 32)`.
Las filas 5–7 quedan libres con fondo azul oscuro. Todas las celdas son opacas,
por lo que no se simula transparencia ni se depende de una clave de color.

`preview.png` muestra las veinte celdas a 6× para inspección visual. También se
conservan veinte PNG individuales con nombres como `byte-thinking.png`.

## Conversión reproducible

Con Pillow instalado en el entorno del proyecto:

```sh
.venv/bin/python tools/prepare_portraits.py
```

El script **no dibuja personajes**. Convierte las hojas raster creadas con
image_gen: separa los cuatro paneles, excluye la parte inferior del torso, conserva
las cabezas completas y reduce por nearest-neighbor a 32×32. Usa una paleta
compartida de hasta 24 colores por personaje y sus cuatro expresiones, sin
suavizado bilineal ni tramado. El manifiesto guarda dimensiones, orden y recorte.

Las cabezas no son fotografías reducidas automáticamente: fueron reinterpretadas
con la herramienta de imágenes para mantener identidad, expresión y estética de
videojuego, y luego convertidas a resolución nativa. Las fotografías se vieron
antes de usarlas como referencia.

## Créditos y licencias de las fotografías

Las siguientes fotos se descargaron de Wikimedia Commons el 15 de noviembre de
2026. Sus páginas de autoría declaran **Creative Commons Attribution 4.0
International (CC BY 4.0)** y permiten adaptaciones con atribución. La licencia
completa se conserva en `assets/portraits/LICENSE-CC-BY-4.0.txt`.

### Magnus Carlsen

- Fotografía: **“Magnus Carlsen in 2025.jpg”**.
- Autor: **Miroslav.vajdic**.
- Fuente y condiciones: [página del archivo en Wikimedia Commons](https://commons.wikimedia.org/wiki/File:Magnus_Carlsen_in_2025.jpg).
- [Archivo original](https://upload.wikimedia.org/wikipedia/commons/1/1a/Magnus_Carlsen_in_2025.jpg).
- Licencia: [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
- Copia local de referencia: `assets/portraits/references/magnus.jpg`.
- Cambios: reinterpretación 8-bit generada por IA, eliminación de fondo y marcas,
  cambio de pose/expresión, reducción, recorte y paleta; cuatro expresiones.

### Hikaru Nakamura

- Fotografía: **“Hikaru Nakamura at the 2026 Naroditsky Memorial Rapid and Blitz (v2).jpg”**.
- Crédito: **Kelly Centrelli / Charlotte Chess Center**; usuario **ChessKelly**.
- Fuente y condiciones: [página del archivo en Wikimedia Commons](https://commons.wikimedia.org/wiki/File:Hikaru_Nakamura_at_the_2026_Naroditsky_Memorial_Rapid_and_Blitz_(v2).jpg).
- [Archivo original](https://upload.wikimedia.org/wikipedia/commons/1/1f/Hikaru_Nakamura_at_the_2026_Naroditsky_Memorial_Rapid_and_Blitz_%28v2%29.jpg).
- Licencia: [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
- Copia local de referencia: `assets/portraits/references/hikaru.jpg`.
- Cambios: reinterpretación 8-bit generada por IA, eliminación de fondo y patrones,
  cambio de pose/expresión, reducción, recorte y paleta; cuatro expresiones.

Los autores de las fotos y los ajedrecistas **no avalan este videojuego**. Las
caras son homenajes visuales; no prueban que los bots reproduzcan la capacidad,
personalidad, declaraciones o decisiones reales de esas personas. No se usan
logotipos, fotografías privadas ni ilustraciones de Chess.com.

## Alcance de licencia de los recursos

Pollito, Hamchow y Byte son diseños originales del proyecto generados con
image_gen y se distribuyen bajo la licencia MIT del proyecto. Las fotografías
y sus adaptaciones de Magnus e Hikaru conservan los créditos y condiciones
**CC BY 4.0** anteriores. Esto se aplica a sus hojas fuente, PNG individuales y
regiones de atlas/header correspondientes (filas 3 y 4); no se les atribuye MIT
sin condiciones de atribución. El código del conversor sigue bajo MIT.

## Verificación hecha

- Se inspeccionaron las cinco hojas y la vista ampliada del atlas final.
- Hay veinte celdas de 32×32, cada personaje con cuatro imágenes diferentes.
- Todas las cabezas caben en la celda; las caras mantienen identidad entre estados.
- Atlas de potencia de dos 128×256 y valores RGBA opacos, sin filtrado bilineal.
- La inspección visual es en la Mac. La integración final y prueba del renderizador
  de consola corresponde al módulo gráfico; estos recursos por sí solos no prueban
  funcionamiento en Nintendo 3DS física.

## Ampliación 0.5

El atlas pasa a128×512 y contiene13 personajes con4 expresiones (52celdas).
Las primeras cinco filas conservan sus fuentes y licencias. Las ocho nuevas
son adaptaciones de los retratos de Chess.com solicitadas por el usuario; no
son MIT. Consulta [CHESSCOM_BOTS.md](CHESSCOM_BOTS.md) para fuentes, prompts
y Elos observados. El generador conserva el orden de IDs existente.

## Ampliación 0.6

El atlas completo actual es128×1024 RGBA8 (512KiB). Incluye31personajes
y124expresiones de32×32. Los18nuevos homenajes usan las miniaturas de Chess.com
señaladas en las capturas del usuario y expresiones adaptadas con la herramienta
de imágenes integrada. Las primeras13filas no cambiaron. Manifiestos, Elos y
procedencia se detallan en CHESSCOM_BOTS.md.

## Ampliación 0.7

39personajes con4expresiones:156celdas. La textura cambia a256×1024 RGBA8 (1MiB),
en dos bancos horizontales de32filas. Esto evita aumentar la altura sobre1024.
El renderer calcula banco/fila a partir del ID estable. Los31retratos anteriores
se conservaron. Las8caras nuevas proceden de dos hojas generadas de personajes
invitados; créditos y prompts en GUEST_CHARACTERS.md.

## Jensen Huang (0.8.1)

ID39, cuatro expresiones generadas con la herramienta integrada image_gen.
Fuente de arte: `assets/portraits/source/jensen.png`; prompt completo: `assets/portraits/source/jensen-prompt.json`.
Diálogos ficticios breves sobre GPU; no citas reales. Retrato no oficial.
Se identifica como fundador de NVIDIA conforme al comunicado del 12 de noviembre de 2026:
https://nvidianews.nvidia.com/news/nvidia-announces-ceo-transition
