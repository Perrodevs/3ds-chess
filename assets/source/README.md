# Recursos fuente locales

Coloca aquí `8 Bit Chess.zip`, el paquete proporcionado por el usuario de
[Cosunosuke](https://cosunosuke.itch.io/8-bit-chess).

Inspección del ZIP proporcionado: `pieces.png`, RGBA, 70×40; `board.png`, RGBA,
100×100. No contiene licencia ni README. Los directorios `__MACOSX` son metadatos,
no recursos. La página pública revisada el 15 de noviembre de 2026 tampoco
expone condiciones de licencia. Precio gratuito no implica permiso de redistribución.

`make assets-personal` extrae exclusivamente los dos PNG esperados, sin aceptar
rutas arbitrarias, y convierte las piezas para una compilación personal local.
No publica nada ni incluye estos recursos en los ZIP de distribución. El tablero
se dibuja con 64 casillas y paletas propias a la resolución nativa de 3DS.

`make assets` restaura los gráficos originales MIT, que son el valor predeterminado.
La fuente bitmap y el icono siempre son originales del proyecto.
