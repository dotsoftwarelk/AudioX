# Known limitations (Phase 0)

- **No local Windows verification.** The development sandbox is Linux/GCC. MSVC was not available locally, but GitHub Actions `windows-latest` executed MSVC 19.51/19.52 Debug and Release builds with `/W4 /WX`; all Phase 0 CPU test suites passed (run `44523518416`), and all Phase 1 suites passed in run `44526375198`. This still does not prove WASAPI or ASIO hardware behaviour.
- **No audio device backends.** WASAPI/ASIO interfaces exist; implementations are Phase 3. No audio can be recorded or played.
- **The engine is a callback skeleton, not a device-driven engine.** `AudioEngine::process()` is called manually by tests; no WASAPI/ASIO device drives it yet. It has been audited for zero allocations in 16 consecutive callbacks, but not measured on hardware.
- **The RT audit is diagnostic only.** It detects allocations on threads explicitly marked with `rt::ScopedAudioThread`; it does not prove the absence of locks, I/O or unbounded work, and the global override is test-only.
- **Graph channel counts must match the engine configuration** at compile time; per-port channel negotiation is not implemented. MIDI connections are counted but not processed. Mixing has no clipping stage yet.
- **DSP is limited** to silence, gain and a gain node processor. No filters, EQ, dynamics, reverb, resampling or FFT yet.
- **No VST3 host.** Only interfaces and a test double exist. No plugin is scanned or loaded.
- **No file codecs or MIDI I/O.** Only metadata/format detection and event representation exist.
- **No GUI rendering or DAW editing features.** The application is a diagnostic shell accepting `--info`, `--self-test`, `--help`.
- **Static libraries only.** Shared-library export macros and ABI stability are future work.
- **Benchmark numbers** were produced in a virtualised Linux sandbox and are not performance claims for Windows audio hardware.
