# Créditos y licencias

- Código, motor de reglas, búsqueda propia, siluetas originales, fuente 5×7,
  icono y sonido procedural: proyecto 3DS Chess, MIT. No se copió código de Stockfish.
- [libctru](https://github.com/devkitPro/libctru#license): licencia estilo zlib;
  copia del aviso en `licenses/libctru.txt`.
- [Citro2D](https://github.com/devkitPro/citro2d/blob/master/LICENSE) y
  [Citro3D](https://github.com/devkitPro/citro3d/blob/master/LICENSE): zlib,
  copyright fincs. Se conservan sus avisos completos en `licenses/`.
  Citro3D se usa como dependencia de Citro2D; no hay modelos ni renderizado 3D.
- GCC/libstdc++: GPLv3 con GCC Runtime Library Exception; avisos en `licenses/`.
  La excepción permite distribuir este programa sin convertir su código original a GPL.
- Newlib: colección de avisos permisivos, reproducidos en `licenses/newlib.txt`.
- [8-bit Chess Free Asset Pack, Cosunosuke](https://cosunosuke.itch.io/8-bit-chess):
  ZIP proporcionado por el usuario. Sin archivo de licencia. La página consultada
  el 2026-11-15 describe el contenido y la descarga gratuita, sin permiso explícito
  de redistribución. No se atribuye MIT, CC0 ni otra licencia al paquete.
  Solo existe como fuente local ignorada por Git y compilación personal opcional;
  los ZIP locales de prueba usan las siluetas originales. Antes de publicar una versión
  con estos sprites hace falta confirmar con el autor sus condiciones.
- [python-chess](https://github.com/niklasf/python-chess): GPLv3+, utilizado
  únicamente como oráculo externo en las pruebas. No se enlaza al juego, no se
  empaqueta su código ni es necesario para ejecutar el `.3dsx`.
- Pillow se usa solo para convertir PNG; no es una dependencia de la consola.
- [Azahar](https://github.com/azahar-emu/azahar/releases/tag/2126.2), GPLv2+:
  herramienta externa de pruebas descargada de su repositorio oficial. No se
  distribuye en los paquetes del juego. La carpeta de pruebas es `build/tools`.

Las copias de avisos se obtuvieron de los repositorios oficiales. No se publica
ningún componente de Nintendo ni se incluyen ROMs, firmware DSP o claves.

## Retratos de los cinco rivales (0.2 RC)

Pollito, Hamchow y Byte: diseños originales generados con la herramienta de
imágenes integrada, MIT del proyecto. Magnus y Hikaru: adaptaciones pixel art
de fotografías reales **CC BY 4.0**, con autor, fuente y modificaciones en
[PORTRAITS.md](PORTRAITS.md). La licencia correspondiente se adjunta al paquete
SD y las fuentes. Sus regiones del atlas y hojas fuente no se atribuyen a MIT.
Las frases de los homenajes fueron escritas para el juego; no son citas reales.

## Ocho retratos de Chess.com (0.5 RC)

Martin, Wayne, Fabian, Juan, Filip, Elani, Noel y Oliver: imágenes de Chess.com
observadas en Safari y adaptadas a 8 bits con cuatro expresiones nuevas. No son
MIT ni se obtuvo permiso de redistribución. Las fuentes exactas, fecha y prompts
se conservan; consulta [CHESSCOM_BOTS.md](CHESSCOM_BOTS.md). Integración local,
sin afiliación oficial. El motor y las frases siguen siendo propios.

## Ampliación de maestros (0.6 RC)

Se añaden18adaptaciones más de las miniaturas de Chess.com señaladas por el
usuario, bajo la misma procedencia y exclusión de MIT anterior. El listado
completo y el manifiesto de referencias figuran en CHESSCOM_BOTS.md.

## Personajes invitados (0.7 RC)

Ocho homenajes de Resident Evil, The Last of Us y Minecraft. Fan art generado
para el juego, sin extraer recursos de los videojuegos. Los personajes no se
atribuyen a la licencia MIT del código. Véase GUEST_CHARACTERS.md para nombres,
fuentes generadas y prompts. No hay afiliación oficial.


## Alcance de empaquetado0.8.2

El atlas completo mantiene recursos pendientes de autorización incluso usando
piezas originales. Los paquetes se marcan LOCAL TESTING ONLY y no están habilitados
para redistribución pública. Véase ASSET_RIGHTS.md. La auditoría no obtuvo permisos
nuevos ni publicó recursos. Se conserva Jensen como retrato generado no oficial,
con prompt local; sus frases son ficticias, sin citas ni afirmaciones personales.
