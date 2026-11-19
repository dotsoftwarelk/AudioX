# AudioX

**AudioX Framework (AXF)** is an independent, modular C++20 audio SDK.
**AudioX DAW** is a separate application built on AXF's public API.

> Status: **Phase 2 — audio foundation complete**.
> AXF now has a compiled, immutable audio graph, a real-time engine skeleton,
> preallocated buffer pools, sample-accurate event scheduling, latency
> propagation and an allocation-free callback verified by the RT audit.
> This is still **not a DAW**: no device backend, recording, editing, mixer UI,
> VST3 hosting or GUI exists yet. See `Documentation/known-limitations.md`.

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
- `Documentation/design/audio-engine.md`, `Documentation/design/graph.md`
- `Documentation/testing.md`, `Documentation/performance/README.md`
- `Documentation/roadmap.md`, `Documentation/known-limitations.md`
- `Documentation/coding-standards.md`, `THIRD_PARTY.md`

## Licence

Apache-2.0. See `LICENSE`, `NOTICE` and `THIRD_PARTY.md`. Steinberg SDKs are not included.
