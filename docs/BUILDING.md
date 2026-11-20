# Building 3DS Chess

Check the branch first. These commands describe **main / 1.1.0 RC Legacy**.
Use each candidate's instructions for branch-specific options, including build
language on the later bilingual branches.

## Host engine checks from a clean checkout

Use Make and a C++17 compiler with POSIX threads. The Makefile defaults to
`clang++`. Override it with `HOST_CXX=g++` if needed.

```sh
make -j2 build/test_core build/test_strength
./build/test_core
./build/test_strength
```

These targets do not need an atlas, Python or devkitPro. See [Testing](TESTING.md)
for full-suite and target checks.

## Full local 3DS build

Install the devkitPro 3DS SDK tooling with devkitARM, libctru, Citro2D, Citro3D
and 3ds-tools. This Makefile defaults to `DEVKITPRO=/opt/devkitpro` and
`DEVKITARM=$DEVKITPRO/devkitARM`. Set them if your SDK lives elsewhere.
Python and Pillow are development dependencies for the generated art.

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r tools/requirements.txt
make assets
make portraits
make -j3 all
make test-target
```

Run `make portraits` only when you already have the branch's authorized local
portrait sources. It reads the local sheets under `assets/portraits/source/`;
a fresh repository checkout lacks those sheets. If an authorized generated
atlas has already been prepared for the branch, supply its generated files
instead. `make assets` creates `assets/generated/atlas.hpp` and the icon.
The team must resolve the [mixed portrait atlas](ASSET_RIGHTS.md) before publishing
a reproducible full distribution.

The ELF and ARM objects live under `build/`. The launcher files are
`dist/3ds-chess.3dsx` and `dist/3ds-chess.smdh`. Copy authorized builds into
`SD:/3ds/3ds-chess/` and start them from Homebrew Launcher. Keep an SD backup.
[Common failures](TROUBLESHOOTING.md)
