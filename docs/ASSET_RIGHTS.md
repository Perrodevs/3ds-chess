# Inventario de procedencia y alcance de uso — 0.8.2 RC

Esta auditoría no obtiene permisos nuevos. Los artefactos completos son para
pruebas locales del propietario; no están autorizados automáticamente para
publicación. La licencia MIT del código no licencia nombres, personajes ni
imágenes de terceros.

| Grupo | Ubicación | Procedencia / estado |
|---|---|---|
| Código propio, motor, fuente bitmap, siluetas, icono, audio procedural | source/, include/, assets/generated con manifest Original | MIT del proyecto; conservar LICENSE |
| Pollito, Hamchow, Byte | assets/portraits/source/{pollito,hamchow,byte}.png | Diseños generados propios; prompts guardados |
| Magnus, Hikaru | assets/portraits/source/ y docs/PORTRAITS.md | Adaptaciones fotográficas CC BY4.0; conservar créditos y licencia; no atribuir MIT |
|26avatares adaptados de Chess.com | source/chesscom-*.png y masters-*.png bajo assets/portraits | No consta permiso de redistribución. Revisión necesaria antes de publicar |
|8invitados de videojuegos | assets/portraits/source/fiction-*.png | Fan art generado; identidades de Resident Evil, The Last of Us y Minecraft de terceros; no licencia MIT de esas identidades |
| Jensen Huang | assets/portraits/source/jensen.png y jensen-prompt.json | Retrato generado no oficial de persona real; frases ficticias, no citas. Revisar condiciones antes de publicar |
| Piezas personales Cosunosuke | assets/source/8 Bit Chess.zip, private/ | Paquete aportado por propietario sin licencia de redistribución confirmada. Nunca incluido como fuente en ZIP; binario personal separado |
| devkitPro/libctru/Citro2D/Citro3D/GCC/newlib | docs/licenses/ | Avisos incluidos; no se incorpora firmware DSP ni claves Nintendo |
| python-chess, Pillow, Azahar, Stockfish8 externo | .venv/, build/ | Herramientas de desarrollo/pruebas; no se enlazan al juego ni se incluyen sus ejecutables en paquetes |

El atlas de retratos mezcla grupos con distintos derechos. Por eso un binario con
piezas originales también contiene recursos pendientes de revisión. No basta
con excluir la carpeta private/ para convertirlo en una distribución pública.
`tools/package.py --local-testing` debe marcar expresamente esta limitación.

## Space & Language

Elon, sus cuatro expresiones, el automóvil rojo, el cohete y sus animaciones
se dibujan mediante código original (`include/graphics/elon_sprite.hpp` y
`source/ui/end_scene.cpp`). No hay fotografías ni vídeo nuevo descargado.
Las identidades/personas y marcas aludidas no son una afiliación ni se licencian
por la licencia MIT del código. Sus diálogos son ficción, no citas. El tema nuevo
es síntesis procedural original. Los recursos privados y atlas anteriores
conservan las restricciones del inventario; los paquetes siguen siendo locales.
