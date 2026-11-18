# Selección inicial para Git local

Preparación del 18 de noviembre de 2026, sobre **1.1.0 RC Legacy**.
Git no estaba inicializado. La rama inicial se llama `main`; no hay remotos,
publicación ni commits históricos reconstruidos.

## Contenido de la instantánea propuesta

La relación completa se encuentra en `GIT_INITIAL_FILES.txt`. Incluye código,
pruebas, scripts, Makefile, documentación y avisos de licencias. La selección
excluye todas las imágenes locales y sus cabeceras de atlas generadas, incluidos
los retratos de procedencia mixta. Esto evita incorporar por accidente contenido
personal o recursos cuyos permisos siguen pendientes de revisión.

## Exclusiones verificadas

- `build/`: compilaciones, herramientas externas, copias de recuperación, logs y
  datos de las pruebas manuales. Las copias antiguas permanecen intactas.
- `dist/` y `private/`: ejecutables y paquetes de ambas variantes.
- `.venv/`, cachés, temporales, datos de editor y archivos de macOS.
- `assets/source/`: paquete personal y recursos extraídos, salvo su README.
- `assets/generated/`: atlas, cabeceras e iconos regenerables.
- `assets/portraits/`: imágenes, referencias, prompts y atlas, salvo el aviso
  CC BY4.0. También se excluyen las capturas generadas en `docs/screenshots/`.
- Guardados `.dat` y sus respaldos, exportaciones FEN/PGN, tarjetas SD emuladas,
  claves/certificados y archivos `.env`.

## Revisión de privacidad

Se comprobó que los archivos candidatos son texto UTF-8, sin enlaces simbólicos
ni binarios. Se buscaron claves privadas, formatos habituales de tokens de GitHub,
OpenAI, AWS y Google, URLs con credenciales y asignaciones evidentes de secretos;
no se detectaron coincidencias en la selección final. También se revisaron las
referencias genéricas a contraseñas/tokens: incluyen pruebas y el PIN deliberado
1234 del panel Developer, no credenciales externas.

La documentación conserva nombres de personajes, créditos públicos de licencias,
resultados técnicos y una ruta local del proyecto mencionada en la auditoría
histórica; no incluye archivos de partida personales ni el contenido de las
copias de prueba. El análisis es una comprobación práctica, no una garantía
matemática de ausencia de cualquier dato sensible.

No se configura un autor ficticio. El nombre y correo deben ser los elegidos por
el propietario, con configuración local a este repositorio. Antes del commit se
presenta la selección; `git log` es la fuente para comprobar si ya se creó.

## Comprobación de formato

`git diff --cached --check` detectó espacios finales o líneas finales adicionales
en avisos de licencia existentes. No son hallazgos de secretos. Se conservaron
los textos de terceros sin reformatearlos para que la instantánea respete sus
archivos originales. El informe detallado permanece local en
`build/git-setup/whitespace-review.txt`.

La selección se mostró antes del commit. El propietario eligió el autor Perrodev
y un correo específico para este repositorio; solo se configura el correo elegido,
sin modificar la identidad global de Git ni configurar GitHub.
