# actualopenskyrim

An open-source reimplementation of Skyrim Special Edition, native on Linux. It targets version **1.7.104.0**, and needs that version's game data.

The code lives in `TESV/` and follows the layout of the original engine: `TESV/Skyrim/`, `TESV/TES Shared/`, `TESV/BSCore/`, `TESV/BSGraphics/`, and so on. Classes, functions and files keep their original names. Empty files mark the rest of the original tree and fill in as the code is reimplemented.

## Progress

<!-- progress -->320 of 196,911 functions reimplemented (0.16%)<!-- /progress -->

## Replaced third party code

| Original | Replacement |
| --- | --- |
| Havok | Jolt Physics, behind the original `bhk*` classes |
| Scaleform GFx | stubbed |

## Building

```sh
nix develop
cmake -S . -B build -G Ninja
cmake --build build
```

You need your own copy of Skyrim Special Edition for the game data.

## License

GNU General Public License v3.0. See [LICENSE](LICENSE).
