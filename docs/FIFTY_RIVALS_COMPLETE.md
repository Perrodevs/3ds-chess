# 3DS Chess — 50 Rivals Update

Candidata **1.3.0 RC1**, rama `feature/50-rivals`. Solo pruebas locales del
propietario. Sin merge, push, publicación ni validación en consola física.

1. **Base real:** 1.2.0 RC2, `750c05b`, desde feature/space-language-update.
   main/origin/main siguen en61bf97d. Se conservaron la etiqueta RC2 y copias de
   sus ZIP/ES/EN en build/fifty-rivals/recovery-packages-rc2.
2. **Catálogo:** exactamente50 rivales, IDs0–49. Los anteriores0–40 no cambian;
   Elon sigue siendo40. [Catálogo completo](BOTS.md).
3. **Nuevos IDs:**41 Garry Kasparov;42 Mikhail Tal;43 Bobby Fischer;44 Ada Wong;
   45 Albert Wesker;46 Enderman;47 GLaDOS;48 Linus Torvalds;49 NULL.
4. **Retratos:**36 expresiones nuevas originales de32×32 dibujadas con código;
   total200 (160 atlas +40 código). No aumenta la textura256×1024 de retratos.
5. **Diálogos:**81 eventos nuevos ES/EN; total450. Nueve rivalidades nuevas,
   además de las anteriores. Son frases ficticias, no citas auténticas.
6. **Música:** nueve semillas nuevas del sintetizador original;51 bucles contando
   menú. La prueba comprueba unicidad de muestras, límites y seguridad del ciclo
   de audio sin dispositivo. La evaluación auditiva en consola queda pendiente.
7. **IA:** seis niveles intactos. Kasparov/GLaDOS Táctico; Tal/Wesker Agresivo;
   Fischer Posicional; Ada/Linus Defensivo; Enderman Equilibrado con aperturas
   razonables variadas por semilla. Interpretaciones del motor, sin Elo prometido.
8. **Adaptación NULL:** antes de tres muestras, Equilibrado. Después favorece
   Defensa, Agresivo o Posicional según capturas/desarrollo. Las aperturas se
   ponderan3:1, solo tras tres primeros movimientos e4/d4/c4 observados con el
   humano jugando blancas. No deduce e4 de partidas jugadas solo con negras.
9. **Datos estudiados:** colores, primera jugada, capturas, enroques y desarrollo
   temprano. Solo partidas competitivas normales con resultado confirmado y
   reconstrucción legal coincidente. Las primeras4096 muestras; ninguna red,
   información oculta, modelo neuronal ni búsqueda adicional.
10. **Persistencia:** null.dat versionado/checksum/límites/respaldo por generación
    de perfil. IDs evitan duplicados y reservan futuros IDs si se recupera un
    perfil anterior. Reconstruye solo historiales fiables retenidos en biblioteca;
    no inventa el pasado. Corrupción independiente preserva la partida normal.
11. **Desbloqueo:** victorias válidas sobre Byte2, Elon40 y GLaDOS47, más Carrera
    Diamante (36 puntos y victoria registrada). Permanente por perfil, anuncio
    breve único ES/EN y requisitos visibles. No mezcla perfiles ni premia práctica.
12. **Escenas:**180 frames lógicos para cada resultado NULL compatible. Victoria
    NULL: oscurecer tablero, interferencia morada, desaparición cosmética de
    piezas, ojos y mensaje. Victoria humana: silueta, parpadeo/interferencia,
    desaparición pixelada, mensaje y ERROR404 la primera vez. A/B salta, reducir
    animaciones respeta la preferencia. No cambia reglas, resultado ni historial.
13. **Premio:** NULL TERMINAL BOARD, producto13 gratuito, negro/morado y bordes
    digitales. No se puede comprar antes de ganarlo. ERROR404 usa el bit7 nuevo y
    el trofeo19 nuevo; se preservan los IDs anteriores. El logro aplica la regla
    existente de25 monedas; las fórmulas de victoria/tablas/derrota no cambian.
14. **Pruebas:**43 programas host iniciales pasaron antes de modificar. La suite
    completa posterior ejecutó46 programas y pasó; perft inicial profundidad4
    sigue en197281. Ver [resultados](FIFTY_TEST_RESULTS.json). ASan+UBSan pasan para
    el modelo y la integración UI, incluida revancha y cancelación del worker.
15. **Problemas corregidos:** nombre de quien habla en rivalidades (antes siempre
    Elon); reutilización de objetos del idioma anterior cuando Make compara tiempos
    al segundo (recompilación forzada por cambio y comparación binaria ES); diferencia uint32 de GCC ARM en min; supuestos antiguos de pruebas
    sobre versión/formato y respuesta e4 de Enderman; inferencia sin datos de
    apertura; reserva de IDs tras recuperación de un perfil anterior. Paletas
    de tablero/revisión/tienda aceptan producto13 sin indexar fuera del arreglo.
16. **Pendientes:** revisión visual completa, especialmente la escena de victoria
    humana, aviso único, tablero en todos los estilos y sesión prolongada. La Mac
    se bloqueó durante Azahar antes de la revancha visual. Esta sí pasó en host.
    Sigue pendiente la detección general de posiciones muertas ya documentada;
    no se modificó en esta expansión. No hay edición pública autorizada.
17. **Recursos:** sizeof objetivo ARM: State NULL56 bytes, EndScene104 bytes
    (64 cosméticos del tablero), App6448 bytes frente a6040 en RC2: +408 bytes
    dentro del objeto. EndScene pasó de32 a104 bytes. Son tamaños de tipos, no el pico
    real de RAM. Audio PCM mantiene366800 bytes de buffers; texturas/pilas y
    presupuestos del motor no aumentan. Autopartidas acotadas de los nueve
    rivales:64800 nodos/9 trazas de12 medias jugadas,5 trazas distintas; tiempo
    host de una muestra en el JSON, sin equivalencia con FPS o consumo en3DS.
18. **Rama:** feature/50-rivals; main no fue tocada. Commits funcionales:
    `9dafadc feat(characters): expand rival catalog with bilingual art and dialogue`;
    `6f6374f feat(null): add bounded adaptive profile with verified history storage`;
    `640eef2 feat(null): integrate secret boss progression, cinematics and rewards`.
    El commit de entrega se identifica en Git al final del proceso.
19. **Entregables:** dist/languages/{ES,EN}/3ds-chess.{3dsx,smdh},
    dist/3DS-Chess-{ES,EN}.zip, dist/3ds-chess-source.zip,
    dist/SHA256SUMS y dist/SHA256SUMS-LANGUAGES; metadatos en dist/RELEASE.json
    y dist/SPACE_LANGUAGE_RELEASE.json. Se validan versión, cabeceras y ZIP.
20. **Estado:** implementado y probado automáticamente; visual parcial en Azahar
    2126.2/Vulkan. No probado en Old3DS XL real. Ningún CIA ni publicación online.

## Verificación visual realizada

Con fixtures sintéticos, no victorias ganadas por el propietario: página10/10,
retratos de Wesker/Enderman/GLaDOS/Linus/NULL, requisitos bloqueados y rechazo de A,
B vuelve a Nueva partida. En inglés se observó la escena de victoria NULL con
piezas que desaparecen, ojos morados, mensaje y derrota normal. No se completó
la revancha visual por bloqueo de macOS. Se cerró el proceso aislado y restauraron
los seis archivos originales; sus SHA-256 coinciden exactamente.

## Compatibilidad y rollback

Lee perfiles1–4 y archivos de biblioteca1(40)/2(41), escribe perfil5/biblioteca3.
Las versiones viejas no leen los escritores nuevos: **haz una copia de toda tu
carpeta de guardados antes de probar**. Los lectores antiguos de entrenamiento,
academia, torneos, Carrera, editor, variantes y Space permanecen. El grupo
aleatorio de Carrera/torneos sigue en0–39 para no cambiar su progresión existente.
Un archivo NULL corrupto deshabilita ese rival y conserva los demás datos.
La recuperación ante cortes de energía no es una garantía absoluta.

## Pruebas físicas que hará el propietario

- Arranque y uso prolongado en Old3DS XL, controles/táctil y páginas1–10.
- Cuatro expresiones de los nueve rivales; contraste con todas las piezas/paletas,
  coordenadas/flechas/orientación de negras y tablero NULL en revisión y tienda.
- Música/efectos reales, bucles, transiciones y funcionamiento sin NDSP.
- Presupuesto de IA fuerte, respuesta de controles, cancelar/pausar/revancha.
- Desbloqueos, anuncio único, ERROR404/tablero, cambio de idioma y tres perfiles.
- Guardar/reanudar, retirar perfiles, respaldo SD y recuperación controlada.
- Suspensión/reanudación física, relojes y condiciones reales de batería/temperatura.

No se han medido batería, temperatura, suspensión física, FPS reales ni RAM pico
real. Las compilaciones siguen siendo **LOCAL TESTING ONLY**, por derechos de
recursos anteriores y homenajes. No se redistribuyen los recursos personales.
