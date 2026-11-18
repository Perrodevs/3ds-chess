# Diseño reversible de personalidades — no activado

La auditoría conserva los seis niveles y las decisiones del motor: no hay cambios
de fuerza/personajes sin pruebas comparativas. Los perfiles siguientes son una
propuesta de configuración futura, no una función ya implementada.

Una configuración experimental puede seleccionar un perfil para el motor común,
con valor predeterminado Balanced y feature flag apagado. El perfil no modifica
legalidad, puntuaciones de mate, tablas, cancelación ni presupuesto de recursos.

| Perfil | Señal a experimentar | Riesgo que medir |
|---|---|---|
| Agresivo | Actividad/presión sobre rey rival | Sacrificios injustificados |
| Defensivo | Refugio y seguridad del propio rey | Pasividad |
| Posicional | Estructura/actividad estable | Tácticas omitidas |
| Táctico | Ordenación de capturas/jaques | Coste de búsqueda y sesgo |
| Equilibrado | Coeficientes actuales | Línea base reproducible |
| Finales | Actividad del rey/peones pasados en final | Interpolación de fases |
| Desarrollo | Actividad inicial y enroque | Repertorio demasiado rígido |
| Intercambios | Preferencia por simplificar con ventaja | Cambiar piezas sin justificación |

Cada cambio debe ser pequeño y acotado, con mismos nodos/semillas, corpus táctico,
perft sin cambios, pruebas de mates/repeticiones/50movimientos, cancelación,
pérdida frente a oráculo y autojuego amplio. Publicar distribución de decisiones,
no una promesa de Elo. Si no mejora diversidad sin regresiones, retirar el perfil.
Conservar IDs y permitir volver a Balanced sin migrar el historial de partidas.
No atribuir fidelidad ajedrecística a personas reales.
