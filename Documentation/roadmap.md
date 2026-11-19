# Development roadmap

| Phase | Scope | Exit evidence |
|---|---|---|
| 0 | Repo, CMake, core skeleton, CI, docs, tests | Done: 7 CPU test suites passed on GCC and MSVC Debug/Release; quality gates passed |
| 1 | Full AXF.Core implementation (events, jobs, config, time, memory, RT audit) | Done: 8 CPU test suites passed on GCC and MSVC Debug/Release (run `44526375198`); ADR 0005 |
| 2 | Audio foundation: compiled graph, RT engine skeleton, buffers, scheduling | Done: 9 CPU test suites pass on GCC and MSVC Debug/Release (Actions run `44528029411`); ADR 0006 |
| 3 | WASAPI device abstraction, shared/exclusive backends, enumeration, device→engine callbacks, disconnect recovery | Simulated tests and MSVC/GCC Debug+Release green (Actions `44532804371`); ADR 0007. **Awaiting** the hardware report in `Documentation/hardware/wasapi-test-plan.md` |
| 3b | ASIO backend (32-bit float, full duplex, one open device) | Real backend compiled with Steinberg SDK 2.3.4 on MSVC Debug/Release (`44617004902`); ADR 0014. Compatibility report with real ASIO drivers still pending; plan in `Documentation/hardware/asio-test-plan.md` |
| 4 | DSP catalogue: biquad LP/HP/peaking EQ, constant-power panner, numerical response tests | Simulated DSP verification report; ADR 0008. No listening or hardware claim |
| 5 | VST3 host | Compatibility matrix on real plugins |
| 6 | MIDI and media file I/O: 16-bit WAV, SMF format 0 | Portable WAV sample and SMF event round-trip tests; ADR 0009 |
| 7 | GUI framework: portable scene graph + Win32/Direct2D/DirectWrite window | Simulated draw-call tests; ADR 0010. Interactive checklist in `Documentation/hardware/gui-test-plan.md` still pending |
| 8 | DAW control model: tracks, clips, mixer state, versioned serialisation | Round-trip and validation tests; ADR 0011 |
| 9 | Hardening pass: concurrent graph publication, malformed-input robustness | 20k concurrent callbacks + 40 republishes, 300 parser inputs; ADR 0012. Hardware report still pending |
| 10 | Sanitizer pass: ASan/UBSan/leak and TSan in CI, alignment/audit fixes | Local + CI sanitizer jobs green, 12/12 CPU tests in each mode; ADR 0013. Hardware/GUI and Windows-specific sanitizer reports still pending |

No phase is marked complete until its acceptance criteria are demonstrated by evidence.
