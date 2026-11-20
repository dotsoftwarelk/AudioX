# ADR 0018 — Unified local verification workflow

- Status: Accepted; executed locally with all steps passing
- Date: 2026-11-19
- Phase: tooling
- Evidence: `Scripts/dev_all.sh full` reported PASS for dependency policy,
  native build/test, installed-consumer build/run, VST3 build/test plus real
  plug-in smoke, ASan+UBSan and TSan, and MinGW/Wine Windows build/test plus
  installed-consumer run.

## Context

Development previously required several separate commands and relied on
GitHub Actions for every Windows check. The sandbox is Linux, so MSVC and
physical audio hardware are not available locally. A single reproducible
entry point was needed to build, install, consume and test AXF without
silently skipping optional integrations.

## Decision

1. `Scripts/dev_all.sh` is the one-command entry point. `full` runs every
   step the host can execute; `quick`, `native`, `consumer`, `vst3`,
   `sanitizers` and `windows` run individual passes.
2. Build outputs, install prefixes and the Wine prefix live under
   `AXF_DEV_ROOT` (default `~/.cache/audiox-dev`) so they do not pollute the
   repository or the small `/tmp` tmpfs.
3. Optional integrations are skipped only with a printed reason: VST3 needs
   `AXF_VST3_SDK_ROOT`; the real plug-in smoke needs `AXF_VST3_PLUGIN_PATH`.
   Skips are reported by name in the final summary.
4. A MinGW-w64 toolchain file cross-compiles the Windows code paths, including
   WASAPI and Win32/Direct2D sources. `CMAKE_CROSSCOMPILING_EMULATOR=wine`
   lets CTest execute the Windows test binaries under Wine. The installed
   consumer is also rebuilt for Windows and executed under Wine.
5. Windows test executables are statically linked against the MinGW runtime
   so they do not depend on DLLs being copied into a Wine prefix.
6. `Scripts/install_dev_deps.sh` installs the extra Debian/Ubuntu packages
   (`wine64`, `wine32`, `mingw-w64`) when a fresh machine needs them.

## Consequences

- A single command now verifies native Release, install/consumer, VST3 (when
  an SDK is supplied), both sanitizer modes, and Windows code paths under
  Wine. All 12 enabled CTest suites pass in each test target.
- MinGW + Wine is an additional local gate, not a replacement for MSVC CI.
  It does not prove MSVC-specific warnings, real WASAPI/ASIO driver behaviour,
  interactive GUI rendering or plug-in/GUI UI compatibility.
- The MinGW pass uncovered dead helper functions in `wasapi_backend.cpp` that
  failed with `-Werror` only on MinGW. They are now marked `[[maybe_unused]]`,
  and shared-mode format detection now uses the stricter IEEE-float helper
  instead of treating every `WAVE_FORMAT_EXTENSIBLE` format as float.
- A Wine prefix is large; keeping it under the cache directory avoids filling
  `/tmp`. First-run Wine initialisation needs a writable `XDG_RUNTIME_DIR`.
