# AudioX

**AudioX Framework (AXF)** is an independent, modular C++20 audio SDK.
**AudioX DAW** is a separate application built on AXF's public API.

> Status: **Phase 7 — GUI framework added; Phase 3 hardware/GUI verification still pending**.
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
> with simulated draw-call tests. This is still **not a DAW**: no recording, editing,
> mixer UI, VST3 hosting or GUI exists yet. See `Documentation/known-limitations.md`.

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
