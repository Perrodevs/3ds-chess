# Pruebas pendientes en Old Nintendo 3DS XL

Guarda una copia de la carpeta `SD:/3ds/3ds-chess/` antes de instalar. Usa uno de
los paquetes ES o EN y reemplaza el ejecutable y su SMDH. Conserva la carpeta
`profiles/` y los archivos existentes. Los dos paquetes usan la misma ruta.

1. Abre el juego y mira el diagnóstico. Confirma que la prueba de lectura y
   escritura de SD y la prueba aislada de perfiles indiquen PASS. Comprueba que A
   entre a perfiles. Un aviso de NDSP permite continuar.
2. En una ranura vacía pulsa A o toca «Crear perfil nuevo». Cancela el teclado y
   comprueba que la ranura siga vacía. Ábrelo otra vez, escribe un nombre ASCII no
   vacío de hasta 24 caracteres y confirma. Prueba también «Crear con nombre
   sugerido» en otra ranura vacía disponible.
3. Inicia una partida, juega, guarda y sal. Vuelve a abrir el juego. Comprueba
   que la ranura conserva el nombre, el color y las jugadas. Cambia de perfil y
   verifica que el otro no comparta partida, monedas ni inventario.
4. Observa el saludo y las reacciones a una captura, un jaque y un resultado en
   la pantalla inferior. Comprueba que el historial, los relojes y las acciones
   siguen accesibles. Prueba Opciones → Accesibilidad → Diálogos.
5. Desde el menú principal pulsa SELECT, abre Sala de trofeos y pulsa SELECT
   de nuevo. Revisa el orden de los perfiles y sus detalles con cruceta y táctil.
   B debe volver primero a la sala y después al menú de modos.
6. Revisa fluidez de selección de piezas, pausa/reanudación, pantalla de
   promoción, revancha, orientación con negras, suspensión y audio. Prueba
   una partida con varios minutos de juego. Estas comprobaciones requieren la
   consola y sus propios controles, SD y DSP.

Si algo falla, apunta la versión, la etapa, el botón pulsado y, si aparece, el
código de error. Copia `SD:/3ds/3ds-chess/logs/session.log` antes de volver a
intentar muchas veces. Si ya rotó, copia también `session.previous.log`.
En Developer → Diagnósticos → Build y ruta de logs se muestra el ID de sesión.
En Errores recientes se ve la etapa y el resultado o errno. Para reproducir un
problema con un perfil anterior usa una copia del guardado.
