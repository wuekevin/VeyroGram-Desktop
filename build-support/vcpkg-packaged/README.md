# Experimental vcpkg packaged dependencies

This is **not** a complete nor validated Windows build manifest. It records standard package candidates from the existing packaged-mode experiments. **Do not** move it to the repository root or enable vcpkg manifest mode for the regular upstream build.

Not included: RNNoise (upstream prepare.py pins desktop-app/rnnoise), tg_owt, tgcalls, Qt, custom FFmpeg patches, Rust/wallet engine and Git submodules.

The upstream Windows preparation uses /MT for several components. A migration must select a compatible triplet and audit binary compatibility; simply using `x64-windows` is unsafe.

Suggested verification sequence: run the native upstream build first; then create a new separate build directory, use the vcpkg toolchain and this manifest directory, compare each detected package with upstream-required versions, and record test results before promoting any package.
