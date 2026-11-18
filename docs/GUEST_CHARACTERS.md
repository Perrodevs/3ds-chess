# Personajes invitados — 0.7 RC

Ocho personajes solicitados por el usuario, añadidos después de los31rivales
existentes sin cambiar sus identificadores ni el formato de guardado v3.

| ID | Personaje | Juego de origen | Dificultad | Nivel interno |
|---|---|---|---|---:|
| 31 | Leon S. Kennedy | Resident Evil | Fuerte | 4 |
| 32 | Jill Valentine | Resident Evil | Fuerte | 4 |
| 33 | Claire Redfield | Resident Evil | Normal | 3 |
| 34 | Chris Redfield | Resident Evil | Fuerte | 4 |
| 35 | Joel | The Last of Us | Normal | 3 |
| 36 | Ellie | The Last of Us | Normal | 2 |
| 37 | Steve | Minecraft | Bajo | 1 |
| 38 | Alex | Minecraft | Normal | 2 |

Las dificultades son decisiones de diseño de este juego; no se asigna un Elo
oficial ni se pretende que esos personajes tengan capacidades ajedrecísticas
canónicas. Usan el motor propio y frases de estado originales compartidas.

## Retratos

Fan art generado con la herramienta de imágenes integrada, sin extraer archivos
de los videojuegos. Dos hojas de cuatro personajes, cuatro expresiones por
personaje: normal, pensando, contento y frustrado. Se conserva la geometría
cuadrada de Steve y Alex. Los retratos no contienen armas, sangre ni heridas.

Fuentes generadas: `assets/portraits/source/fiction-0.png` y `fiction-1.png`.
Prompts exactos: `assets/portraits/source/fiction-prompts.json`.

Con Jensen, los40personajes suman160celdas de32×32. La textura de consola es256×1024 RGBA8,
1MiB; se organiza en dos bancos horizontales de32filas para respetar la altura
máxima usada por el juego. Los primeros31retratos se conservan píxel por píxel.
El selector mantiene cinco tarjetas por página: ocho páginas, cuatro tarjetas
en la última. No se requiere descargar imágenes desde la consola.

## Créditos

Personajes de Resident Evil, The Last of Us y Minecraft: homenajes no oficiales.
La autoría de los personajes no se atribuye al proyecto ni a la licencia MIT de
su código. Estos retratos son adaptaciones para la copia local solicitada por el
usuario; no hay afiliación ni autorización oficial de las franquicias.
