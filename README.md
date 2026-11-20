# AudioX

**AudioX Framework (AXF)** is an independent, modular C++20 audio SDK.
**AudioX DAW** is a separate application built on AXF's public API.

> Status: **Phase 5 VST3 host and limited local plugin smoke test; hardware/full-plugin/GUI verification still pending**.
> AXF has a compiled, immutable audio graph, an allocation-free real-time
> engine, preallocated buffer pools, sample-accurate scheduling and latency
> propagation. Phase 3 adds a portable device abstraction, a real WASAPI
> shared-mode backend (plus event-driven exclusive mode), device enumeration,
> disconnect detection, a public device→engine callback adapter and a hardware test plan.
> Cloud CI verifies compilation and simulated tests only; no physical audio
> device has been tested. Phase 4 adds stable biquad low-pass/high-pass/peaking
> filters and a constant-power panner with numerical frequency-response and
> RT-allocation tests. Phase 6 adds real 16-bit PCM WAV read/write and
> Standard MIDI File format 0 import/export with round-trip tests. Phase 7 adds
> a portable widget scene graph and a real Win32/Direct2D/DirectWrite window
> with simulated draw-call tests. Phase 8 adds a control-thread track/clip
> document with deterministic serialisation and round-trip tests. A Phase 9
> hardening pass adds concurrent graph-republish and malformed-input tests. A
> Phase 10 pass adds ASan/UBSan/leak and TSan CI jobs and fixes a scheduler
> payload alignment defect found by UBSan. Phase 3b adds a Windows ASIO backend
> built against the Steinberg SDK, downloaded by CI and never vendored. Phase 5
> adds a VST3 scan/create/process/state host built against the MIT-licensed SDK,
> also fetched by CI and never vendored. Local Phase 8/11 work adds linear
> gain/pan automation, bounded undo/redo, atomic autosave/recovery and a
> preallocated real-time-safe recorder with WAV output, all on control-thread
> boundaries. This is still **not a DAW**: there is no full editing timeline,
> mixer UI or live-device recording session yet. A portable GUI scene graph and
> Win32 window exist but are not interactively verified.
> See `Documentation/known-limitations.md`.

## Design goals

- Real-time safety: no allocation, locking, I/O or UI work in the audio callback.
- Strict module boundaries: AXF never depends on the DAW application.
- Test-first: every module has unit tests; hardware tests are explicit and opt-in.
- Portable public APIs, Windows-specific code confined to private implementations.

## Repository map

```
AXF/        Core, Audio, Devices, DSP, Graph, Plugins, MIDI, Media, Project, GUI, Platform
AudioX/     Application shell and future DAW subsystems
Tests/      Unit, Integration, Audio, DSP, Plugins, Platform, Benchmarks
Documentation/  Architecture, ADRs, design, testing, performance, roadmap
Examples/   Standalone AXF consumer
Scripts/    Build/test/dependency tooling
```

## Build

Requirements: CMake >= 3.25, C++20 compiler (MSVC 2022 on Windows), Git.
Tests use GoogleTest v1.15.2 fetched by CMake at configure time (network required once).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DAXF_WERROR=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/AudioX/audiox --self-test
./build/AudioX/audiox --list-devices
```

Windows:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DAXF_WERROR=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Install and consume AXF standalone:

```bash
cmake --install build --prefix /path/to/prefix
cmake -S Examples/minimal_app -B /tmp/app -DCMAKE_PREFIX_PATH=/path/to/prefix
cmake --build /tmp/app
```

## One-command development workflow

From a fresh Debian/Ubuntu machine:

```bash
Scripts/install_dev_deps.sh   # sudo apt: wine64, wine32, mingw-w64
Scripts/dev_all.sh full
```

`full` runs the dependency-policy check, native Release build/test, install
plus standalone-consumer run, VST3 build/test when `AXF_VST3_SDK_ROOT` is set,
the opt-in real plug-in smoke when `AXF_VST3_PLUGIN_PATH` is set, ASan+UBSan
and TSan, and a MinGW-w64 Windows cross-build whose tests and installed
consumer run under Wine. Individual passes are available as `quick`, `native`,
`consumer`, `vst3`, `sanitizers` and `windows`. Builds live in
`AXF_DEV_ROOT` (default `~/.cache/audiox-dev`). Optional steps print `SKIP`
with a reason instead of silently pretending to pass. Wine/MinGW verifies
Windows code paths locally but does not replace MSVC CI or hardware tests.

## Documentation

- `Documentation/architecture/overview.md`
- `Documentation/adr/` — architecture decision records
- `Documentation/design/audio-engine.md`, `Documentation/design/graph.md`,
  `Documentation/design/dsp.md`, `Documentation/design/file-formats.md`,
  `Documentation/design/gui.md`
- `Documentation/testing.md`, `Documentation/hardware/wasapi-test-plan.md`,
  `Documentation/hardware/gui-test-plan.md`,
  `Documentation/performance/README.md`
- `Documentation/roadmap.md`, `Documentation/known-limitations.md`
- `Documentation/coding-standards.md`, `THIRD_PARTY.md`

## Licence

Apache-2.0. See `LICENSE`, `NOTICE` and `THIRD_PARTY.md`. Steinberg SDKs are not included.
