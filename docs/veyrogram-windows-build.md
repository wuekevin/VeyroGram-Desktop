# VeyroGram Desktop — Windows x64 build

This fork currently inherits **SereinGram / Telegram Desktop's mixed build system**. The supported starting point is the upstream dependency preparation script, **not** an all-vcpkg build.

## Why this matters

- `Telegram/build/prepare/prepare.py` builds custom versions of FFmpeg, `tg_owt`, OpenSSL, Rust components and **RNNoise**.
- RNNoise comes from `desktop-app/rnnoise` at revision `d8ea2b0`; do **not** add the unsupported upstream `rnnoise` vcpkg port to a Windows manifest.
- The preparation script uses `/MT` in several MSVC builds. Standard vcpkg `x64-windows` binaries generally use `/MD`. Mixing them without an ABI audit can cause link errors.
- The checked-in `Telegram/build/qt_version.py` currently selects **Qt 6.11.2** for Windows. A local Qt 6.8.3 installation is **not** interchangeable without validation.
- `DESKTOP_APP_USE_PACKAGED=ON` changes the dependency-discovery path. It does not make the upstream custom builds unnecessary.

## Prerequisites

Windows x64, Git, Python, CMake, a Visual Studio MSVC x64 environment with Windows SDK, adequate disk space and reliable GitHub/download access. The current upstream `docs/building-win.md` describes VS2026 with the 14.44 toolset; VS2022 14.44 is **not yet validated** against this fork's latest preparation scripts.

## Quick start (CMD)

From the repository root:

```bat
git submodule sync --recursive
git submodule update --init --recursive
set "TDESKTOP_API_ID=YOUR_ID"
set "TDESKTOP_API_HASH=YOUR_HASH"
scripts\veyrogram-windows.cmd all
```

Use `prepare` to build only third-party dependencies, or `build` to configure/build with already prepared dependencies:

```bat
scripts\veyrogram-windows.cmd prepare
scripts\veyrogram-windows.cmd build
```

The script preserves the upstream `Telegram/configure.bat x64 qt6` entry point and generates into `out/Release`. It does not package or code-sign a production installer.

**Credentials:** never commit Telegram API credentials to source, build presets, CI logs or screenshots. Use environment variables or CI secrets. If sharing generated build logs, check that command lines do not contain credentials.

## Dependency responsibilities

| Source | Examples | Recommended management |
| --- | --- | --- |
| Git submodules | `lib_base`, `lib_ui`, `lib_webrtc`, `tgcalls`, `widget-sandbox`, `OpenCC`, `libfido2` | Recursive Git checkout, keep pinned revisions |
| Upstream prepare.py | RNNoise, FFmpeg, `tg_owt`, Opus, OpenH264, OpenAL, Rust/wallet engine, Qt and other codecs | Keep pinned build scripts |
| External standard libraries | Boost.Regex, Minizip, JPEG, zlib, OpenSSL and other portable components | Optional vcpkg only after version/ABI testing |
| Developer tools | MSVC, Windows SDK, Python, Git, CMake, MSYS2, Rust | Native setup plus upstream prepare.py |

A **separate experimental** manifest is located at `build-support/vcpkg-packaged/vcpkg.json`. It is **not a verified full dependency list** and is **not wired into the default build**, intentionally. For a future packaged build, audit each `find_package()` and `pkg_check_modules()` branch, align /MT vs /MD, Qt version and library versions, then introduce a dedicated preset. Do not combine two competing copies of `tg_owt`, RNNoise or FFmpeg.

## Known pending work

1. Validate the upstream build on VS2022 MSVC 14.44 versus documented VS2026.
2. Align project identity: root `project(Telegram)`, description and branding still reference SereinGram; rename and package independently **without stripping upstream license notices**.
3. Smoke-test the executable and provide a VeyroGram-specific Inno Setup/signing workflow. Upstream's production `Telegram/build/build.bat` references private signing resources and cannot be used unchanged.
4. Only after a successful native build, migrate vetted libraries to vcpkg Manifest + CMakePresets. Document each approved version and ABI choice.
