# blutter-windows

![CI](https://github.com/luotianyiismywife/blutter-windows/actions/workflows/build.yml/badge.svg)
![License](https://img.shields.io/badge/license-MIT-green)
![Platform](https://img.shields.io/badge/platform-Windows%20x64-blue)

Windows build support and (planned) precompiled binaries for [worawit/blutter](https://github.com/worawit/blutter) — the Flutter/Dart AOT snapshot reverse engineering tool.

Upstream blutter targets Android (arm64) on Linux/macOS. This repository makes it build & run **on Windows, against Windows desktop Flutter apps** (`data/app.so` + `flutter_windows.dll`).

Verified against a real-world Flutter Windows desktop app (Dart 3.3.4, x64): full pipeline from source build to successful snapshot parsing (`pp.txt` / `objs.txt` produced).

## What's different from upstream (4a60ac6)

All patches are Windows/x64 enablement — no behavioural change for the existing Android arm64 flow:

| # | File | Patch |
|---|------|-------|
| 1 | `blutter/src/Disassembler.h` | `AsmInstruction` / `A64::Register` / `AddrRange` are only defined in `Disassembler_arm64.h` but referenced by arch-neutral headers. Under `TARGET_ARCH_X64`, provide minimal stand-ins so everything compiles. Real disassembly stays gated by `NO_CODE_ANALYSIS`. |
| 2 | `scripts/CMakeLists.txt` | Add `windows` as a `TARGET_OS` (`DART_TARGET_OS_WINDOWS`); keep `DART_TARGET_OS_WINDOWS_UWP` only for cross builds. |
| 3 | `dartvm_fetch_build.py` | After install, fix up exported `dartvmTarget.cmake` if CMake resolved `ICU` to the Windows SDK `icuuc.lib` (SDK ICU lacks the `icu_73::*` symbols the VM needs → LNK2001 x19). |
| 4 | `blutter/CMakeLists.txt` | Link `dbghelp` (needed by Dart VM's `native_symbol_win.cc` when targeting windows). |
| 5 | `blutter.py` | `--dart-version` now accepts a 4th segment `no-compressed-ptrs`, e.g. `3.3.4_windows_x64_no-compressed-ptrs` (snapshot pointer-compression must match exactly). |

## Build (Windows)

Requirements:

- Visual Studio 2022 with **Desktop development with C++** (cl.exe; CMake/Ninja are *not* required from VS — see below)
- Git, Python 3 (3.10+ tested; `pyelftools`, `requests` via pip, `cmake`, `ninja` via pip)

```bat
:: 1. deps (downloads ICU4C 73.2 + capstone 4.0.2 into external\)
python scripts\init_env_win.py

:: 2. open a VS x64 environment, then from repo root:
python blutter.py --dart-version 3.3.4_windows_x64_no-compressed-ptrs --no-analysis path\to\app.so out_dir
```

Notes:

- The first run clones `dart-lang/sdk` (sparse, tens of MB) and compiles the Dart VM (~263 objects; expect 10–30 min).
- `--no-analysis` is **required for x64 targets** — the code analyzer is arm64-only. You still get the reliable outputs: `pp.txt` (object pool) and `objs.txt` (all classes/objects). The `asm/` folder is not meaningful for x64.
- The snapshot's pointer-compression setting **must match** the VM build. If you see `Snapshot not compatible with the current VM configuration`, toggle `no-compressed-ptrs` accordingly.
- The Dart VM library name does not encode the pointer-compression variant. If you build a different variant of the same version, delete `packages/lib/dartvm*.lib` + `packages/lib/cmake/dartvm*` first.
- Non-ASCII paths break MSVC PCH (`C1083`) — build from an ASCII path.

## Roadmap

- [ ] GitHub Actions matrix builds (Dart 3.x LTS versions × x64) with precompiled `blutter_*.exe` releases
- [ ] `flutter_windows.dll` version auto-detection on Windows (upstream only parses ELF `libflutter.so`)
- [ ] Upstream PRs for patches #2–#4
- [ ] One-click usage docs for common targets (Flutter Windows desktop apps)

## Releases

| Release | Dart VM | Compressed pointers | Artifact |
|---------|---------|--------------------|----------|
| v0.1.0 (planned) | 3.3.4 | no | `blutter_dartvm3.3.4_windows_x64_no-compressed-ptrs_no-analysis.exe` + ICU/capstone DLLs |

> The exe is statically bound to one Dart VM configuration — a new Dart version (or a different pointer-compression setting) needs a separate build. The Actions workflow builds on demand; version matrix releases will follow.

## License

MIT — inherited from upstream blutter (© Worawit Wangwarunyoo). See `LICENSE`.
