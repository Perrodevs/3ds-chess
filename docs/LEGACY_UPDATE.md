# Legacy Preview — fases A–E (registro de 0.9.2)

La entrega posterior completa los modulos restantes: [estado actual](LEGACY_COMPLETE.md).
Los pendientes descritos abajo corresponden a esta entrega historica.

## Estado y alcance real

Base inspeccionada: 0.9.0 RC, motor propio, 40 rivales, academia de diez lecciones,
torneos de cuatro/ocho, biblioteca de treinta, estadísticas y seis estilos IA.
Las pruebas host existentes pasaron antes de modificar código; la comprobación
manual anterior de Azahar fue parcial. Ninguna versión está validada en consola
física. Este trabajo no cambia las reglas clásicas ni elimina modos anteriores.

Identificación de esta entrega: **0.9.2 Legacy Preview**. No es Legacy 1.1.0
completa. Las fases integradas aquí son preparación, perfiles, Developer, Carrera,
Supervivencia, sala de trofeos y explorador de aperturas.

Recuperación previa: `build/legacy-update/recovery-090.zip`, SHA-256
`38c8ee2c9888b908fc69eae7dd93efb0ba7bf14169c767c7ea8d6b266b7ca56e`.
Contiene fuentes, pruebas, recursos y entrega 0.9.0; no incluye guardados privados
del emulador. El proyecto no contiene repositorio Git; no se crearon commits.

## Panel Developer

En Opciones, «Developer: bloqueado» abre un código de cuatro dígitos. La cruceta
elige y cambia dígitos; A o Confirmar valida; B o Cancelar vuelve. El código es
**1234**. No es autenticación segura ni protege información privada: es un acceso
deliberado a herramientas locales. Solo se activa en memoria durante esta sesión;
se apaga al cambiar de perfil o reiniciar. La misma opción permite desactivarlo.

En una partida, SELECT abre Developer si está activo. Apagado conserva el acceso
original al historial. Developer también ofrece historial, y Herramientas lo
mantiene disponible. B/Volver retorna al tablero. Abrir el panel detiene las
búsquedas y pausa el descuento del reloj, igual que otros menús de pausa.

Opciones de prueba: victoria, tablas, derrota, 10000 monedas temporales y todos
los cosméticos (vista de marco/piezas). El primer truco guarda la partida normal
y crea una copia temporal sin reloj. No concede monedas, estadísticas, logros ni
entradas de biblioteca; las monedas de prueba no se escriben en disco. Guardar,
salir o suspender conserva el original. Al volver al menú o comenzar otra partida
se restaura el original. Los trucos se rechazan en entrenamiento y torneos para
no falsear su progreso. No hay trucos de desbloqueo de academia/campaña.

## Perfiles e integración

El ejecutable de 3DS inicia con tres ranuras: elegir/crear, nombre, avatar,
renombrar y eliminar con confirmación. Menú → Perfiles permite cambiar después de
guardar. Para renombrar se entra primero al perfil; se reutiliza el teclado nativo
existente. El avatar recorre los cuarenta retratos. B conserva la ruta de retorno.

Cada selección reconstruye los Stores con la ruta exclusiva de esa ranura,
descarga el estado anterior y cancela IA/pistas/torneo. Perfil, compras, monedas,
sesión normal, entrenamiento, academia, torneo, biblioteca, estadísticas y
preferencias quedan separados. Los módulos futuros deberán usar esta ruta.

El constructor App mantiene un adaptador de un solo perfil para las pruebas
históricas de 0.8.x/0.9.0. El ejecutable real lo desactiva y usa perfiles desde
arranque. `test_legacy_ui` prueba explícitamente ese recorrido de producción.

La migración importa al primer hueco sin borrar originales y confirma los datos
antes de publicar el índice. Una ranura dañada se rechaza sin reemplazarla por
valores nuevos. El borrado retira la ranura y conserva su generación recuperable;
una creación posterior utiliza otra generación sin heredar progreso. Se necesita
recuperación manual para los archivos retirados; no hay purga automática.

Detalles, formatos, límite de 4 MiB por archivo y fallos de escritura:
`docs/LEGACY_PROFILES.md`. No se promete durabilidad absoluta ante cortes físicos.

## Pruebas y compatibilidad

Nuevos casos: contraseña incorrecta/correcta, SELECT encendido/apagado, resultado
forzado sin premio, inventario temporal, guardar/suspender sin contaminar partida,
creación de tres perfiles, cambio, nombre, eliminación/cancelación, aislamiento de
monedas/compras/estadísticas/academia/opciones y rechazo de perfil corrupto.
El backend prueba corrupción del índice, migración y seis fallos de I/O.

Registros bajo `build/legacy-update/`: `baseline.log`, `integration.log`,
`final-verification.log`, `profiles-sanitizers.log`. No se atribuye rendimiento de
Mac a Old 3DS. La pila principal/worker y presupuestos de IA se mantienen.
No se han agregado dependencias externas ni consultas de red.

## Carrera, Supervivencia, trofeos y aperturas

Desde el menú principal, **SELECT** o tocar el pie inferior abre Modos Legacy.
Ofrece Carrera, Supervivencia, Sala de trofeos y Explorador de aperturas.

Carrera usa cinco divisiones (0/9/21/36/54 puntos), +3 por victoria, +1 por tablas,
−1 por derrota con mínimo cero. Solo el primer ascenso concede monedas y un
cosmético existente. Conserva mejor división, resultados y rangos alcanzados.
Supervivencia sube un nivel cada dos victorias; tablas/derrota terminan el intento.
El récord persiste. Los hitos de 3/5/10 victorias premian una sola vez por perfil.
Una partida Legacy puede estar pendiente a la vez; se retoma desde su modo sin
reemplazar la partida normal. Salir conserva lo confirmado. Terminar un intento
requiere confirmación. No se crean partidas IA ficticias: se usa el worker real.

Los resultados se guardan en `progression.dat`, sin inflar estadísticas normales.
Primero se confirma el resultado y su derecho acumulado a monedas. Después se
actualizan saldo y marcador de pago en una sola escritura de `profile.dat`.
Una interrupción en el pago deja el premio pendiente y no permite cobrarlo dos
veces. El total de hitos asciende como máximo a 1850 CC por vida del perfil; no
hay premios ilimitados por repetir ascensos. `docs/LEGACY_PROGRESSION.md` detalla
las reglas y límites. Developer no altera estos modos.

La sala presenta 16 trofeos/medallas derivados de logros, divisiones, récords,
torneos y academia. No inventa fechas ni concede monedas al visitarla. La
colección se obtiene de los estados persistidos del perfil, con identificadores
fijos en esta entrega; no hay trofeos de variantes que aún no existen.

Aperturas incluye diez familias y dos líneas por familia (20 líneas legales).
Permite avanzar/retroceder en el tablero, cambiar rama, guardar favoritas y
practicar ambos colores con pistas. Una jugada legal distinta no se presenta
como mala: simplemente no continúa la línea elegida. No avanza ni modifica el
tablero del ejercicio. Completar guarda progreso y no concede monedas. Se
conservan favoritas, familias estudiadas y última apertura en `openings.dat`.
El ejercicio en curso se puede reiniciar; no se promete reanudar cada intento
parcial después de cerrar. Los datos estáticos son compartidos y no históricos.
`docs/LEGACY_OPENINGS.md` describe sus líneas y pruebas.

## Guardados versión 4

El perfil añade `legacyCoinsClaimed`, junto con el saldo, para hacer idempotente
la entrega de hitos. El lector conserva versiones 1, 2 y 3; no fabrica progreso
histórico. Los dos sidecars nuevos tienen validadores, checksum, temporales y
respaldo independientes. Una corrupción del modo no sustituye el perfil normal
ni sus monedas por datos nuevos. La migración de raíz reconoce también esos
sidecars cuando están presentes. No hay una transacción FAT universal entre
archivos; la estrategia separa resultado confirmado y pago recuperable.

Los ejecutables antiguos no conocen versión 4: para volver atrás se debe usar
la copia previa completa de guardados. No se promete que 0.9.0/0.9.1 pueda leer
un perfil escrito por esta entrega. El juego nuevo sí carga sus guardados.

## Pendientes de la expansión solicitada

Editor de posiciones, creador de puzzles, Chess960/Three-check/King of the Hill
con IA adaptada y sus guardados, indicador/avisos de batería. No se anuncian como
terminados. La validación física y los recorridos manuales no cubiertos siguen
pendientes. No se cambia a 1.1.0.

## Pruebas físicas y manuales pendientes

En Old 3DS XL: arranque/migración con copia de SD, teclado de nombre, SELECT y
código, rechazo de trucos en entrenamiento/torneo, conservación del reloj y de
partidas al cambiar perfil, suspensión, audio DSP y presión de RAM con archivos
grandes. No se realizaron pruebas físicas ni se midieron batería, temperatura o
consumo. La nueva interfaz tuvo una comprobación manual parcial en Azahar, descrita en
`docs/LEGACY_MANUAL_AZAHAR.md`; faltan los recorridos no cubiertos y la consola.
Las pruebas host no sustituyen esa validación.

Entregables locales en `dist/`; variante personal permanece en `private/` cuando
se compila. Los permisos de retratos y piezas conservan sus límites anteriores:
no se autoriza redistribución pública ni se genera CIA.

Veredicto de alcance: **IMPLEMENTACIÓN PARCIAL — FUNCIONES PENDIENTES**.
