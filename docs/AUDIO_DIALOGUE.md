# Audio y dialogos originales

La musica se activa en Opciones (desactivada por defecto). Los efectos estan
activados por defecto y tienen su propio interruptor: seleccion, movimiento,
captura, jaque, victoria y derrota. El sonido requiere que `ndspInit` funcione
en la consola; si falla, el juego sigue funcionando en silencio.

## Musica

`graphics::Audio::music(bool enabled, int theme = -1)` conserva las llamadas
anteriores. `-1` reproduce el tema del menu y `0..39` el tema de ese rival.
Cada tema es un bucle original de cuatro segundos con dieciseis notas cortas,
un bajo suave y uno de cuatro timbres. La semilla individual cambia el motivo
y el bajo; no son grabaciones ni temas de Resident Evil, The Last of Us,
Minecraft o Chess.com. Los cambios de tema preparan otro buffer antes de
retirar el anterior de la cola del DSP. No se regenera el bucle cada cuadro.
El volumen es bajo para que los efectos destaquen.

`musicSample(theme, sampleIndex)` expone la misma sintesis PCM, sin dispositivo
de audio, para comprobar sus limites y variedad. Fuera del bucle retorna cero;
un tema invalido usa el del menu. `musicSampleRate=22050` y
`musicLoopSamples=88200`. Dos buffers PCM de unos 172 KiB cada uno mantienen
separados el tema sonando y el tema que se esta preparando.

## Dialogos

`ai::botDialogue(id, ai::Dialogue event)` contiene nueve frases originales
por cada uno de los 40 rivales. Los eventos son `Greeting`, `Thinking`,
`Capture`, `Check`, `Win`, `Loss`, `PuzzleStart`, `PuzzleSolved` y `Mistake`.
Las frases son desde la perspectiva del rival o mentor. Una derrota suya usa
`Loss`; una victoria del jugador no debe usar `Win` del rival.

Todas las frases son ASCII imprimible y tienen un maximo de 36 caracteres.
Los invitados usan alusiones a exploracion, recursos, equipo o construccion,
sin copiar citas de sus obras. No son declaraciones reales de los ajedrecistas.
`botLine(id, expression)` sigue disponible para las expresiones anteriores;
los primeros 13 mantienen sus frases previas y el resto recibe frases propias.

## Verificacion

`tests/test_audio_dialogue.cpp` verifica que los 41 bucles sean distintos,
reproducibles, no silenciosos, de amplitud acotada y sin un desplazamiento
continuo significativo; comprueba tambien las 360 frases, limites y entradas
invalidas. El mismo generador PCM se usa en consola. Compilacion host:

```sh
clang++ -std=c++17 -O2 -Wall -Wextra -Iinclude \
  source/graphics/audio.cpp source/ai/bots.cpp source/chess/chess.cpp \
  tests/test_audio_dialogue.cpp -o build/test_audio_dialogue
./build/test_audio_dialogue
```

Los objetos de audio y bots tambien se compilan para ARMv6K. La comprobacion
de muestras no sustituye escuchar el volumen y la mezcla en una 3DS real.
