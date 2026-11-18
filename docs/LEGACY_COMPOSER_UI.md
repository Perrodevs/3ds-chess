# Taller: editor y puzzles personalizados

`ui::ComposerUI` es una pantalla independiente con su propio tablero, sesión de
práctica y colección. App la crea usando el directorio del perfil seleccionado;
no recibe referencias a la economía ni a la partida normal. Inicio → SELECT →
Modos → Editor y puzzles abre el taller. B vuelve por la jerarquía de pantallas.

## Controles

- Editor: cruceta mueve el cursor del tablero superior y A coloca la pieza
  elegida. X recorre borrador, seis piezas blancas y seis negras. La paleta táctil
  permite elegirlas directamente. El minitablero inferior mueve el cursor;
  «Colocar pieza» ejecuta la colocación. El minitablero no altera piezas al tocarlo.
- START abre guardar, turno/enroques/EP, crear puzzle, importar/exportar FEN y
  jugar práctica local. Los derechos incoherentes permanecen editables, pero
  bloquean guardar y jugar hasta corregirse. La casilla EP recorre ocho archivos
  del rango correspondiente al turno; izquierda/derecha permiten cambiarla.
- «Limpiar» deja un borrador incompleto y sin enroques. «Restaurar» recupera la
  posición clásica inicial. Las casillas no saltan al alcanzar un borde.
- Guardar/renombrar utiliza el teclado nativo existente con 1–24 caracteres ASCII.
  Guardar sobre un registro previamente cargado pide confirmación. Cancelar no lo
  reemplaza. Eliminar una posición o puzzle también pide confirmación.
- `import.fen` y `export.fen` se encuentran únicamente en el directorio del perfil.
  La importación valida el archivo antes de cambiar el borrador. Exportar no
  comparte datos ni requiere servicios externos.
- Durante juego o grabación: A selecciona origen y destino legales; las promociones
  muestran dama/torre/alfil/caballo. La práctica es local para ambos bandos.
  START abre opciones. B en juego abre esas opciones; B durante grabación deshace
  una media jugada y START permite regresar al editor.

## Puzzles

Elegir objetivo desde una posición válida y grabar la solución sobre el tablero
(incluyendo las respuestas del defensor, hasta cinco medias jugadas). START →
Comprobar ejecuta una demostración limitada en un worker independiente: el texto
permanece visible y B cancela. Una sola variante grabada no basta para probar mate;
el backend comprueba todas las defensas. Si agota su presupuesto queda pendiente.
Los puzzles pendientes se pueden guardar y editar, pero no jugar como validados.

Jugar vuelve a verificar el puzzle antes de iniciar. Las jugadas y pistas que
necesitan demostración también usan el worker; no bloquean el hilo de controles.
La partida normal y sus relojes no avanzan dentro del taller. En puzzles de mate
se pueden intentar otras jugadas si el backend demuestra el objetivo. Los errores
no modifican el tablero. Cada inicio/reinicio registra un intento; cada intento
puede completarse una vez. Jugar ambos bandos permite explorar respuestas legales.
X ofrece una pista demostrada cuando el presupuesto permite encontrarla.
No concede monedas ni estadísticas competitivas.

Solo hay un worker del taller, con cancelación atómica y unión antes de reutilizar
sus datos o cerrar. Tiene pila de 64 KiB para un árbol acotado a cinco medias
jugadas, sin ejecutar en paralelo otra búsqueda del propio taller. La pila
principal no cambia. El límite actual de prueba es 50000 nodos. Esto no demuestra
rendimiento físico ni tiempo máximo real en una consola.

## Persistencia y alcance

`composer.dat` contiene hasta treinta posiciones y treinta puzzles por perfil,
con sus nombres, identificadores, FEN, objetivo, solución, estado y contadores.
Los guardados hacen rollback en memoria si falla el archivo. Las escrituras,
checksum y respaldo pertenecen al backend descrito en `LEGACY_COMPOSER.md`.
Una colección corrupta bloquea guardados de ese módulo y conserva sus archivos;
no vacía ni reemplaza el perfil normal.

Se conservan los registros guardados y los contadores confirmados. El cursor,
la edición incompleta, la grabación sin guardar y el punto intermedio de una
práctica son temporales: se puede repetir desde el registro guardado, pero no
se promete reanudarlos tras cerrar. Suspender cancela demostraciones antes de
persistir la colección. No se guardan puzzles a medio demostrar como si fueran
válidos. Los ocho puzzles originales siguen en su sistema existente.

## Verificación

`tests/test_composer_ui.cpp` usa controles reales de la clase y la ruta de App.
Cubre edición táctil/física, límites de cursor, ajustes, FEN, guardado y cancelación
de reemplazo, mate grabado y demostrado, error legal que conserva el tablero,
pista, contador de completado único, eliminación confirmada/cancelada, promoción
a caballo, teclado cancelado y colección corrupta. También entra desde Modos,
usa el resultado del teclado nativo, suspende/reanuda y cambia entre dos perfiles:
comprueba los archivos separados y que no se alteran partida normal ni monedas.

Host y ASan/UBSan pasaron; el módulo UI compiló para ARMv6K. La integración final
incluye estas pruebas con el resto del proyecto. No se ha validado interacción
física, teclado de consola, suspensión real ni rendimiento Old 3DS XL.

## Correcciones de integración verificadas

- Si falla la escritura del contador al iniciar un puzzle, se descarta el intento
  en memoria y se vuelve a su menú. No queda jugable una sesión sin intento
  confirmado que pudiera acreditar su solución a un intento anterior incompleto.
  La regresión crea un directorio `composer.dat.tmp` para provocar el fallo real,
  conserva contadores y permite reiniciar después de recuperar la escritura.
- Guardar una edición selecciona el identificador del puzzle editado, en vez del
  último elemento de la lista. La regresión edita el primero de dos registros y
  comprueba que abrir/renombrar sigue apuntando al primero.

Ambas regresiones, junto al recorrido de App y perfiles, pasaron ASan/UBSan en
`build/legacy-update/composer-ui-fixes-sanitize`.
