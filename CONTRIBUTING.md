# Contributing to 3DS Chess

Discuss the scope with the maintainer before starting a wide change. Use the
private repository and a focused branch such as `fix/profile-recovery` or
`docs/build-guide`. If someone else is editing the same checkout, create an
independent Git worktree. Keep changes and tests together in small conventional
commits, for example `fix(engine): preserve castling rights on replay`.

1. Read [the repository rules](AGENTS.md) and [build prerequisites](docs/BUILDING.md).
2. Start from the appropriate reviewed branch. Check `git status` before touching files and inspect the diff before staging.
3. Follow the surrounding C++17 style. Keep the board and search independent of rendering. Bound allocations, search work and main-thread I/O for Old 3DS.
4. Run the [relevant checks](docs/TESTING.md). Record what you ran and distinguish host, emulator and physical results.
5. Push only after review of the exact commits and files. Open a pull request to the intended branch using the repository template. Ask for review before integrating gameplay into `main`.

Do not include personal saves, logs, credentials, generated releases or assets whose redistribution is unresolved. Use synthetic fixtures. Consult the [asset inventory](docs/ASSET_RIGHTS.md) and [Git workflow](docs/GIT_LOCAL.md). The [Code of conduct](CODE_OF_CONDUCT.md) applies to project spaces.

Se aceptan contribuciones y reportes en español e inglés. Comparte los pasos y
resultados exactos para que alguien más pueda reproducirlos.
