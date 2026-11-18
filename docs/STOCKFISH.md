# Decisión sobre el motor offline

Revisión: 2026-11-15. La versión integrada es un motor original, no Stockfish.

## Evidencia primaria

- [README oficial de Stockfish](https://github.com/official-stockfish/Stockfish):
  soporte general de CPU de 32 y 64 bits, evaluación NNUE y GPLv3.
- [Makefile actual](https://github.com/official-stockfish/Stockfish/blob/master/src/Makefile):
  enumera objetivos como armv7 y general-32, sin plataforma 3DS/ARM11 específica.
- [Stockfish 16.1](https://github.com/official-stockfish/Stockfish/releases/tag/sf_16.1):
  documenta la eliminación de la evaluación clásica y el uso de redes duales.
- [Stockfish 8, Makefile](https://github.com/official-stockfish/Stockfish/blob/sf_8/src/Makefile)
  y [thread.cpp](https://github.com/official-stockfish/Stockfish/blob/sf_8/src/thread.cpp):
  C++11, general-32, hilos estándar y sincronización. No requiere NNUE.
- [Hilos libctru](https://devkitpro.github.io/libctru/thread_8h.html): núcleo 0
  disponible en Old 3DS; los núcleos adicionales de New 3DS no son necesarios.

## Evaluación

Stockfish moderno no se considera un port directo validado: ARM11 requiere un
objetivo ARMv6K, adaptación de la plataforma, y comprobar memoria de NNUE, tablas,
stack, mecanismos de sistema e interfaz de cancelación. El soporte “32 bits” no
prueba compatibilidad de todo el programa con 3DS. No se confunden los requisitos
de entrenamiento NNUE con los requisitos de ejecutar el motor.

Stockfish 8 es un candidato más razonable para un port futuro: evaluación clásica,
C++11 y objetivo genérico. Se descargó el tag sf_8, commit
`369eff437cc081eb4b8ab5f519cf3f86b79e87d0`, bajo `build/stockfish8` (excluido de
los paquetes). `thread.cpp` **sí compiló como objeto ARMv6K** con devkitARM 16.1.0.
Un programa mínimo con `std::thread` también enlazó con libctru. Por tanto no se
atribuye un fallo inexistente al compilador ni se afirma que Stockfish sea imposible.
Estos ensayos no validan un port completo: no se enlazó ni ejecutó Stockfish 8,
ni se midieron memoria, fuerza, consumo o tiempos en una Old 3DS. Un port funcional
exige otra integración y validación. No se encontró una versión completa ya
verificada para este proyecto que justificara sustituir el rival propio funcional.

GPLv3 permite distribuir un port, cumpliendo las obligaciones correspondientes,
incluido el código fuente y avisos. Ningún código de Stockfish se integra en esta
versión; su GPL no se asigna al motor original.

## Motor elegido: actualización 0.2 RC

Se mantiene un motor original para el juego offline. La versión0.2 incorpora
PVS, tabla de512KiB, claves de posiciones y evaluación de rey/peones/movilidad
por fase, además de tres rangos cualitativos. La implementación, presupuestos
vigentes, pruebas y comparación con el motor anterior están documentados en
[ENGINE_UPGRADE.md](ENGINE_UPGRADE.md).

Stockfish8 sí se compiló después para **macOS** como oráculo externo de pruebas;
se usó para contrastar20posiciones con ambos motores. No se integra, enlaza ni
redistribuye con el juego, y ese ejecutable de Mac no demuestra un port a ARM11.
La ejecución completa de Stockfish en una Old3DS sigue sin validar.

El hilo libctru usa el núcleo0, prioridad0x31 y stack256KiB; no requiere funciones
New3DS ni aumentar frecuencia. Guarda su posición independiente y comunica
cancelación/finalización de forma atómica. Las texturas actuales totalizan1280KiB GPU
(256KiB piezas/fuente +1024KiB retratos); la tabla añade384KiB en heap según el ABI ARM medido.

Los rangos Bajo/Normal/Fuerte son relativos a esta aplicación. No hay ELO medido,
calibración humana ni mediciones de batería o rendimiento de hardware físico.
