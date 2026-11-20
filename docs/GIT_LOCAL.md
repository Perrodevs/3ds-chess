# Git local y remoto privado de 3DS Chess

El proyecto usa `origin` privado en [Perrodevs/3ds-chess](https://github.com/Perrodevs/3ds-chess). Un push requiere autorización del propietario. Compara los hashes y revisa los archivos a transferir antes de enviar una rama. Usa un worktree separado para cambios simultáneos y conserva `main` para cambios aceptados.

## Origen del historial

Git se inicializó en esta carpeta, con `main`, el 18 de noviembre de 2026.
Antes de esta incorporación no existía un historial Git del proyecto. Las copias
anteriores en `build/` y los paquetes se conservan intactos, fuera del historial.
No se reconstruyeron commits de 0.8.x/0.9.x durante la inicialización local.

El primer commit registra una instantánea del código actual
**1.1.0 RC Legacy**, no una prueba de estabilidad física ni un historial anterior.
La lista inicial presentada para revisión está en `GIT_INITIAL_FILES.txt` y la auditoría
en `GIT_INITIAL_AUDIT.md`. Los nombres de esos documentos describen la preparación;
comprueba `git log` para saber si el commit ya existe.

## Qué se guarda y qué permanece local

Se versionan fuentes C++17, cabeceras, pruebas, Makefile, scripts, documentación,
licencia del código y avisos de licencias. `.gitignore` excluye binarios, atlas
generados, piezas personales, imágenes/referencias de retratos de derechos mixtos,
guardados, FEN/PGN personales, paquetes, copias de recuperación, SDK, cachés y logs.
No se elimina ningún archivo al ignorarlo.

Por esa separación, un checkout de Git no constituye por sí solo una copia
completa de los recursos ni de las partidas. Los scripts de generación sí se
versionan; los retratos locales se deben conservar por separado. Los atlas
originales se pueden regenerar con `make assets`; `make portraits` necesita las
fuentes locales de retratos. El paquete personal requiere sus recursos privados.
No se afirma que una copia aislada de Git compile sin esos recursos y devkitPro.

Las partidas y recursos ignorados no están protegidos por Git. Mantén también
una copia de seguridad de esos archivos. `.gitignore` no analiza secretos ni
protege archivos que se añadan a la fuerza: revisa siempre el contenido preparado.

## Autor, ramas y commits

Configura el autor **solo en este proyecto**, usando tus valores elegidos:

```sh
git config --local user.name "Tu nombre elegido"
git config --local user.email "tu-correo-elegido@example.com"
```

`main` conserva la base inicial y, después, cambios revisados/aceptados. Que una
versión esté en `main` no significa que esté validada en una consola física.
Las actualizaciones grandes se desarrollan en ramas descriptivas, por ejemplo
`feature/chess-variants`, `fix/save-corruption` o `perf/chess-engine`. Una tarea
pequeña y segura puede reutilizar la rama de trabajo actual. Ejemplo de función nueva:

```sh
git switch main
git switch -c feature/opponent-personalities
git status --short
git diff
# Añade únicamente las rutas que revisaste:
git add source/ruta.cpp tests/prueba.cpp
git diff --cached --stat
git diff --cached
git commit -m "feat(ai): add configurable opponent personalities"
```

El ejemplo no implementa una función: sustituye la rama, las rutas y el mensaje
por los cambios reales de tu tarea. Utiliza `tipo(área): descripción breve` en
inglés, con verbos claros. Tipos permitidos: `feat`, `fix`, `perf`, `refactor`,
`test`, `docs`, `style`, `build` y `chore`. No uses mensajes genéricos como
`update`, `changes`, `fix stuff` o `final`.

Sustituye las rutas de ejemplo por archivos reales. Los cambios sin commit
pertenecen al directorio de trabajo y pueden acompañar un cambio de rama: guarda
el trabajo antes de cambiar de tarea. No necesitas crear ramas por anticipado.

Después de comprobar y aceptar una función:

```sh
git switch main
git merge --no-ff feature/opponent-personalities
```

El primer commit es raíz y pertenece a `main`. La rama `feature/next-update` se
crea desde esa misma base para el siguiente trabajo; todavía no añade cambios.
Su nombre genérico se conserva como parte de la configuración inicial; no se
elimina ni se reescribe. Para trabajo nuevo importante usa un nombre basado en
la función real. No hay ramas remotas ni comandos de publicación configurados.

## Etiquetas de versión

Etiqueta únicamente un commit revisado cuya versión coincida con el código y
los artefactos entregados, después de comprobar las pruebas correspondientes.
No se crea una etiqueta automáticamente por adoptar estas reglas. Para una candidata, usa un
nombre que no prometa una versión estable, por ejemplo:

```sh
git tag -a v1.1.0-rc.1 -m "Legacy: candidata pendiente de pruebas fisicas"
git show v1.1.0-rc.1
```

No declares una versión estable mientras falten pruebas físicas.
La etiqueta no compila, no ejecuta pruebas y no crea versiones antiguas. No se
crean etiquetas retrospectivas a partir de ZIP como si hubieran sido commits.

## Consultar o recuperar una versión

```sh
git status --short
git log --oneline --decorate --all
git diff COMMIT_ANTERIOR HEAD
```

Antes de cambiar de versión, confirma tus cambios o guárdalos temporalmente:

```sh
git stash push -u -m "Trabajo antes de consultar otra version"
git switch --detach ETIQUETA_O_COMMIT
# Regresar a main:
git switch main
git stash pop
```

Sustituye los identificadores por los reales de `git log`/`git tag`.
`stash -u` incluye archivos nuevos no ignorados; no respalda guardados ni recursos
privados ignorados. Una versión antigua del código puede necesitar los recursos
y guardados correspondientes: consultar Git no migra partidas hacia atrás.

Para trabajar a partir de un commit anterior conservando todo el historial:

```sh
git switch -c feature/recuperacion COMMIT_ANTERIOR
```

Para deshacer un commit concreto mediante otro commit revisable:

```sh
git revert HASH_DEL_COMMIT
```

No uses `reset --hard`, `clean -fdx` ni borrados del directorio `.git` como rutina
de recuperación: pueden destruir cambios o los recursos locales ignorados.

## Uso exclusivamente local

`git status`, `git add`, `git commit`, `git branch` y `git tag` no publican nada.
`git remote -v` permite comprobar que no hay destinos configurados. Este trabajo
no añade remotos, submódulos, Git LFS, credenciales de GitHub ni publicación.

## Reglas permanentes y registro de cambios

`AGENTS.md` establece el flujo para futuras tareas. Cada mejora coherente se
confirma junto con sus pruebas relevantes; cambios independientes deben ir en
commits distintos, sin fraccionar cada pequeña edición. Antes de confirmar,
revisa estado, diff preparado, privacidad e ignorados. Una prueba fallida debe
informarse y no puede presentarse como validación.

Mantén `CHANGELOG.md` actualizado durante cada tarea, sin esperar a que se solicite.
Registra funciones, correcciones, rendimiento, compatibilidad, migraciones y
limitaciones reales; no inventes entradas históricas a partir de los ejemplos.
El informe final incluye cambio, pruebas, rama, hash corto, mensaje exacto y
pendientes. No modifiques innecesariamente el commit raíz para adaptarlo al
formato nuevo: la convención se aplica a los commits siguientes.

No borres ramas, reescribas historial ni ejecutes operaciones destructivas sin
autorización. No se configura ningún remoto ni se ejecuta `git push`.
