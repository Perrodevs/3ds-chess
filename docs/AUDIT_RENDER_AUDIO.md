# Auditoría 0.8.2 RC: gráficos, audio y recursos

Fecha: 2026-11-16. Sin pruebas físicas. Las comprobaciones de SDK instrumentado
no ejecutan PICA200, NDSP ni un DSP real; verifican el código de propiedad y las
llamadas de las ramas `__3DS__` del proyecto.

## Hallazgos y cambios

| ID | Estado / gravedad | Probabilidad / disparador | Evidencia y corrección |
|---|---|---|---|
| RA-01 | Confirmado, alta | Baja en uso normal; fallo de reserva de un framebuffer | `Renderer::init` ignoraba un target nulo y devolvía éxito; `screenBegin` lo pasaría a funciones que desreferencian el target. Ahora falla limpiamente, libera la inicialización parcial y permite reintentar. |
| RA-02 | Confirmado, alta | Ventana temporal al salir con un frame en vuelo | `Renderer::shutdown` liberaba atlas y buffers C2D antes de que `C3D_Fini` esperase su cola. `C3D_FrameEnd` es asíncrono. Ahora elimina los targets mediante la API pública bloqueante antes de liberar texturas y vértices. Se confirma el orden incorrecto; no se afirma haber reproducido un fallo físico. |
| RA-03 | Confirmado, media | Baja; reserva fallida de uno de dos buffers musicales | `Audio::init` devolvía audio listo aunque faltase un buffer de música. Ahora cualquier fallo de sus tres reservas revierte recursos y devuelve false; la aplicación puede continuar sin audio. |
| RA-04 | Endurecimiento verificable, media | Reutilización o reinicialización accidental de una instancia | Inicializar dos veces sobrescribía punteros/estado y podía duplicar referencias de servicio. `Renderer::init` y `Audio::init` son idempotentes. Audio no puede copiarse, para evitar dos propietarios de punteros NDSP. |
| RA-05 | Endurecimiento, media | Salida con samples en uso | NDSP se detiene antes de liberar memoria de muestras. Limpiar la cola solo actualiza el estado de canal; no se usa como garantía de terminación física inmediata del DSP. |
| RA-06 | Documentación/interfaz, baja | Abrir ayuda de modos | La ayuda decía “Libre”; ahora indica práctica con deshacer, sin reloj ni premios. |

Se redujo únicamente la capacidad sobrante del buffer de efectos de 12000 a 7000
samples, manteniendo intactas las muestras generadas. No se cambió el presupuesto C2D, dimensiones de atlas, frecuencia, volumen,
longitud de bucles, dificultad ni tiempos de búsqueda. No se añadieron efectos
costosos ni una dependencia nueva.

## Tinte blanco y capturas incompletas

El contrato del SDK se revisó en los headers instalados y el código primario
Citro2D v1.7.0. El juego usa un atlas blanco para la fuente,
`C2D_TintSolid`, color ABGR correcto y fuerza 1 para texto coloreado.
La prueba del límite SDK comprueba que realmente se entrega ese color/fuerza.
Cambiar la textura o el modo de mezcla no se justifica con esa evidencia.

Existe además un reporte primario de Azahar exactamente sobre este síntoma:
[issue 2471](https://github.com/azahar-emu/azahar/issues/2471).
Afecta a 2126; el autor describe que el tinte falla con Vulkan y funciona con
OpenGL/software. El issue está cerrado por PR 2627. Esto respalda investigar el
backend de Azahar 2126.2 antes de atribuir el blanco a un error del juego; no
sustituye una comparación A/B de esta compilación. Root puede verificar la
misma pantalla con un backend alternativo y restaurar su configuración.

Las capturas inmediatas parcialmente negras no tienen una causa confirmada
por esta auditoría. No hay evidencia de que el fallo de propiedad al salir
explique un frame incompleto mientras se juega. Permanecen pendientes de
reproducción controlada/comparación de backends; no se declaran corregidas.

## Audio

- Cuarenta temas de rival más menú: 41 muestras de bucle distintas.
- PCM16 mono, 22050 Hz, 88200 muestras = 4 segundos y 176400 bytes por buffer.
- Dos buffers musicales; el siguiente se prepara separado del que está en cola.
- Buffer de efectos: 7000 muestras = 14000 bytes; efectos generados de 1800 o 7000 muestras.
  Antes reservaba 12000 muestras: ahorro exacto de 10000 bytes, comprobado en las
  peticiones de reserva del test SDK y con los siete códigos de evento bajo ASAN.
- Salida estéreo con la señal mono mezclada a ambos lados.
- `linearAlloc` garantiza alineación 0x80 según el SDK instalado.
- Se vacía la caché antes de enviar los datos al DSP.
- Inicio/fin del bucle en cero; las ocho muestras de resto del reparto en 16 notas
  son cero. Los tests verifican amplitud acotada, energía, desplazamiento medio
  limitado, entradas inválidas y reproducción determinista.
- Los cambios de tema y el apagado se prueban en la frontera SDK. No se asegura
  ausencia perceptual de clics al cortar un tema a mitad ni que cuatro segundos
  no resulten repetitivos: son evaluaciones auditivas pendientes.
- NDSP ausente o memoria insuficiente: initfalse y llamadas posteriores seguras.
- Shutdown repetido es seguro. No se modifican las preferencias guardadas.

## Inventario y memoria objetivo

Cifras derivadas de dimensiones, tipos del SDK y reservas visibles; no son
mediciones de memoria máxima en hardware ni RSS de la Mac:

| Reserva | Bytes calculados |
|---|---:|
| Textura fuente/piezas256×256 RGBA8 | 262144 |
| Textura retratos256×1024 RGBA8 | 1048576 |
| Audio lineal (14000+2×176400) | 366800 |
| C2D:16384vértices×32bytes +24576índices×2bytes | 573440 |
| Command buffer C3D predeterminado | 262144 |
| Framebuffers color/depth de ambas pantallas, sin overhead | 1036800 |

Las primeras cinco reservas suman 2513104 bytes de memoria lineal estimada,
excluyendo shader/colas/alineación y otros componentes. Los framebuffers usan
reservas de render target/VRAM. Los arrays constantes de atlas también existen
en la imagen del programa: 1310720 bytes de datos RGBA, separados de su copia de
texturas. No se suman estas cifras con RSS host como si fuesen la misma métrica.

El script de verificación comprobó 40 IDs, 160 celdas distintas y opacas,32×32,
con atlas256×1024. No se regeneró ni reemplazó arte. Los límites de lado de
textura permanecen 1024. Manifest/paletas y orden de expresiones se conservan.

## Evidencia de pruebas

`tests/test_native_resources.cpp` compila las fuentes reales renderer/audio con
`__3DS__` y `tests/native_stubs/` delante de los headers reales. Inyecta fallos de
C3D/C2D, los dos targets, los dos atlas, NDSP y las tres reservas de audio.
Comprueba rollback, limpieza, reintento, idempotencia, orden de espera GPU,
servicio DSP detenido antes de liberar, doble buffer y contrato de tinte.

Comando ejecutado con AddressSanitizer y UndefinedBehaviorSanitizer:

```sh
clang++ -std=c++17 -O1 -g -Wall -Wextra -fsanitize=address,undefined \
  -fno-omit-frame-pointer -D__3DS__ -Itests/native_stubs -Iinclude \
  -Iassets/generated -Iassets/portraits source/graphics/renderer.cpp \
  source/graphics/audio.cpp source/ai/bots.cpp source/chess/chess.cpp \
  tests/test_native_resources.cpp -o build/audit-082/test_native_resources
./build/audit-082/test_native_resources
```

Resultado: pasó. Log: `build/audit-082/native-resources.log`.
`tests/test_audio_dialogue.cpp` también pasó con ambos sanitizadores: 41 temas,
360 frases. Los objetos reales renderer/audio compilaron con devkitARM ARMv6K,
O2, Wall/Wextra y sin avisos.

Fuentes primarias del SDK consultadas:

- Headers instalados en `/opt/devkitpro/libctru/include`.
- [Citro2D v1.7.0 base.c](https://github.com/devkitPro/citro2d/blob/v1.7.0/source/base.c).
- [Citro3D renderqueue.c](https://github.com/devkitPro/citro3d/blob/master/source/renderqueue.c).
- [libctru ndsp-channel.c](https://github.com/devkitPro/libctru/blob/master/libctru/source/ndsp/ndsp-channel.c).

No se instalaron versiones nuevas del SDK ni se modificaron librerías globales.
