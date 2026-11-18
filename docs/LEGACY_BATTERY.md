# Batería: fuente real y límites

`platform::Battery` usa exclusivamente las interfaces instaladas de libctru
`3ds/services/ptmu.h`: `ptmuInit`, `PTMU_GetBatteryLevel`,
`PTMU_GetBatteryChargeState` y `ptmuExit`.
La documentación de ese encabezado declara nivel **0–5** y carga **0/1**.
Se muestran barras aproximadas; no son porcentajes ni autonomía restante.
La API PTMU es la del modelo 3DS base, sin funciones exclusivas de New 3DS.
Esto no constituye una calibración en una Old 3DS XL real.

## Integración

App contiene un miembro `platform::Battery`. Cada actualización llama
`battery.update(monotonicMs)` con tiempo monotónico en milisegundos. Se usan
`reading().available`, `.level` y `.charging` para el icono en pantalla
inferior. `warning()` indica cuándo mostrar durante seis segundos el texto
«BATERIA BAJA - GUARDA TU PROGRESO». No abre diálogo, no cambia relojes ni guarda
archivos. El consumidor debe dibujarlo sin bloquear controles.

La primera consulta es inmediata; después se consulta cada treinta segundos.
El servicio se abre de forma diferida y se libera en el destructor. Un fallo de
inicialización o consulta produce estado no disponible; el siguiente intervalo
puede reintentarlo. El juego sigue funcionando sin datos de batería.

Un nivel 0/1 sin cargar activa aviso una vez. Descender desde 1 hasta 0 permite
un aviso adicional por empeoramiento. El nivel 2 no rearma el aviso: evita
repetirlo por fluctuaciones pequeñas. Cargar o recuperarse hasta 3–5 rearma la
advertencia. Una muestra no disponible no rearma ni fabrica lecturas. Un salto
atrás del reloj cancela el texto y reprograma las consultas sin duplicar avisos.
La carga se detecta en la siguiente consulta, no mediante sondeo continuo.

## Pruebas verificadas

`tests/test_battery.cpp` inyecta una fuente mediante `setSource(function,context)`.
Comprueba niveles sanos/bajos/críticos, carga, recuperación, indisponibilidad,
valor fuera de rango, treinta segundos entre lecturas, seis segundos de aviso,
histéresis y reinicio del reloj. No espera tiempo real.

Ejecutados: compilación y prueba host; AddressSanitizer y UndefinedBehaviorSanitizer
(`build/legacy-update/battery-sanitizers.log`); compilación ARMv6K del módulo
(`build/legacy-update/battery-arm.o`, atributos ELF revisados). No se ejecutó el
módulo en una consola física ni se verificó la respuesta real de PTMU en batería
baja. Los umbrales son política de interfaz, no mediciones físicas calibradas.

La batería es estado del dispositivo, no progreso de perfil. No se persiste ni
se mezcla con las estadísticas del jugador. El aviso sugiere guardar; no fuerza
un guardado en mitad de operaciones. Un usuario puede usar el guardado normal en
un punto estable, manteniendo sus validaciones existentes.
