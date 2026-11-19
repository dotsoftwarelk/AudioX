# Known limitations (Phase 10)

- **No local Windows verification.** The development sandbox is Linux/GCC. MSVC was not available locally, but GitHub Actions `windows-latest` executed MSVC 19.51/19.52 Debug and Release builds with `/W4 /WX`; all Phase 0 CPU test suites passed (run `44523518416`), Phase 1 in run `44526375198`, and Phase 2 in run `44528029411`. This still does not prove WASAPI or ASIO hardware behaviour.
- **WASAPI is implemented but not hardware-verified.** The Windows backend compiles in CI and the portable device→engine path is simulated; no physical endpoint has been exercised. See `Documentation/hardware/wasapi-test-plan.md`.
- **No ASIO backend.** The `Backend::Asio` enum value exists, but opening it is unsupported.
- **Shared mode uses the device mix format.** A requested sample rate that differs from the mix rate returns `Unsupported`; resampling is future work.
- **One direction per WASAPI device.** Full duplex needs two separate devices and separate engine graphs.
- **Exclusive mode is limited** to two channels and has not been measured for jitter or glitches on hardware.
- **`reportedLatencySeconds()` is a buffer-duration estimate**, not an end-to-end measured latency.
- **The RT audit is diagnostic only.** It detects allocations on threads explicitly marked with `rt::ScopedAudioThread`; it does not prove the absence of locks, I/O or unbounded work, and the global override is test-only.
- **Graph channel counts must match the engine configuration** at compile time; per-port channel negotiation is not implemented. MIDI connections are counted but not processed. Mixing has no clipping stage yet.
- **DSP is limited** to silence, gain, biquad low-pass/high-pass/peaking EQ and constant-power panning. No compressor, limiter, convolution, reverb, resampling, oversampling or FFT yet. Biquad supports at most eight channels and coefficients are immutable; changing a filter requires republishing the graph. Frequency-response tests are numerical, not listening tests.
- **No VST3 host.** Only interfaces and a test double exist. No plugin is scanned or loaded.
- **Device change notifications are not synchronised with engine reconfiguration.** Listeners receive a hint and must re-enumerate on the control thread.
- **File I/O is limited** to 16-bit PCM RIFF/WAVE and SMF format 0. No 24-bit/float WAV, AIFF, FLAC, multi-track SMF export, SysEx or tempo-map export. SMF frames must align with the PPQ/tempo grid for exact round-trips.
- **GUI is not interactively verified.** A Win32/Direct2D/DirectWrite window and a portable scene graph exist, but no window was rendered in the sandbox. There is no input routing, high-DPI scaling, accessibility or animated layout. The application remains a diagnostic shell accepting `--info`, `--self-test`, `--list-devices`, `--help`.
- **Static libraries only.** Shared-library export macros and ABI stability are future work.
- **Benchmark numbers** were produced in a virtualised Linux sandbox and are not performance claims for Windows audio hardware.
- **Sanitizers are CPU-only and Linux/GCC based.** ASan+UBSan with leak
  detection and TSan run on every push/PR and report 12/12 enabled CPU tests
  passing, but there is no Windows sanitizer run, long-duration soak test or
  device underrun measurement under load.
- **The RT allocation override test is skipped under sanitizer runtimes.**
  ASan/TSan replace global `operator new`; the portable assertion calls the
  instrumentation API directly, while the global-override test runs only in
  ordinary builds. This is a runtime limitation, not a relaxed contract.
