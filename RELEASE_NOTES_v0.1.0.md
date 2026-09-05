# First Windows-ready build of blutter

Prebuilt `blutter` for **Windows x64 hosts**, targeting **Flutter Windows desktop app** snapshots (`data/app.so`).

## Contents

- `blutter_dartvm3.3.4_windows_x64_no-compressed-ptrs_no-analysis.exe`
- `capstone.dll`, `icudt73.dll`, `icuuc73.dll` (required runtime DLLs)

## Target configuration

| Setting | Value |
|---|---|
| Dart VM | 3.3.4 |
| Snapshot pointer compression | **no** |
| Code analysis | disabled (`--no-analysis`, arm64-only upstream; x64 targets must use this) |

Check your target's configuration: the error message of a mismatched build prints what the snapshot requires, e.g.

```
Snapshot not compatible with the current VM configuration: the snapshot requires '... no-compressed-pointers ...' but the VM has '... compressed-pointers ...'
```

## Usage

```bat
blutter_dartvm3.3.4_windows_x64_no-compressed-ptrs_no-analysis.exe -i path\to\app.so -o out_dir
```

Outputs:

- `out_dir\pp.txt` — full object pool (strings/constants + owning context)
- `out_dir\objs.txt` — every class & object in the snapshot
- `out_dir\asm\` — not meaningful for x64 targets (upstream disassembler is arm64-only)
- `out_dir\ida_script\` — IDA name/func scripts (function boundaries are correct for x64)

Requires the VC++ 2022 redistributable on machines without VS installed.

## Source

Built by GitHub Actions from [luotianyiismywife/blutter-windows](https://github.com/luotianyiismywife/blutter-windows) (upstream: [worawit/blutter](https://github.com/worawit/blutter), MIT).
