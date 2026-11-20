# Architecture

The default branch is **1.1.0 RC Legacy**. Branch-specific changes are noted
below. The target is Old Nintendo 3DS XL and the build targets ARMv6K.

| System | Code and responsibility |
| --- | --- |
| Rules | [`source/chess/`](../source/chess) owns positions, legal moves, SAN and results. Clocks and draw rules are independent of rendering. |
| AI | [`source/ai/`](../source/ai) owns the original evaluator, bounded search, rival preferences and cancellable worker. Stockfish has only been used as an external reference. |
| UI | [`source/ui/`](../source/ui) handles input and separate views for the board, menus, training and result review. |
| Renderer and sound | [`source/graphics/`](../source/graphics) draws the two native screens through Citro2D/Citro3D. NDSP is optional. The host renderer supports previews and tests. |
| Persistence | [`source/storage/`](../source/storage) handles profile isolation, versioned save files and backups. Modes store progress separately. [Formats](STORAGE.md) |
| Modes | [`source/training/`](../source/training), [`source/progression/`](../source/progression), [`source/tournament/`](../source/tournament) and [`source/variants/`](../source/variants) own their rules and progress. |

The main loop samples physical controls and touch, advances the application view,
then renders. AI and hints search copies of the position and completed results
are checked against the current game before application. Keep UI callbacks short
and bound RAM and work per frame. Profile, SDK and hardware behavior are covered
at different levels by the [test guide](TESTING.md).

[Space & Language](https://github.com/Perrodevs/3ds-chess/tree/feature/space-language-update)
adds the ES/EN catalog and per-profile language. The
[Diagnostic System](https://github.com/Perrodevs/3ds-chess/tree/feature/diagnostic-system)
adds startup probes and a local bounded journal. See the
[Stability and Quality report](https://github.com/Perrodevs/3ds-chess/blob/fix/stability-quality-update/docs/STABILITY_140_RC2.md)
for the newer profile, dialog and trophy flows.
