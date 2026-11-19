# Known limitations (Phase 0)

- **No local Windows verification.** The development sandbox is Linux/GCC. MSVC was not available locally, but GitHub Actions `windows-latest` executed MSVC 19.51/19.52 Debug and Release builds with `/W4 /WX`; all 7 CPU test suites passed (run `44523518416`). This still does not prove WASAPI or ASIO hardware behaviour.
- **No audio device backends.** WASAPI/ASIO interfaces exist; implementations are Phase 3. No audio can be recorded or played.
- **No compiled real-time engine.** The graph is a control-thread description only. No callback has been instrumented or measured on real hardware.
- **DSP is limited** to silence and gain utilities. No filters, EQ, dynamics, reverb, resampling or FFT yet.
- **No VST3 host.** Only interfaces and a test double exist. No plugin is scanned or loaded.
- **No file codecs or MIDI I/O.** Only metadata/format detection and event representation exist.
- **No GUI rendering or DAW editing features.** The application is a diagnostic shell accepting `--info`, `--self-test`, `--help`.
- **Static libraries only.** Shared-library export macros and ABI stability are future work.
- **Benchmark numbers** were produced in a virtualised Linux sandbox and are not performance claims for Windows audio hardware.
