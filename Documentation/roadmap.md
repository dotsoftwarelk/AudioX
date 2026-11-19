# Development roadmap

| Phase | Scope | Exit evidence |
|---|---|---|
| 0 | Repo, CMake, core skeleton, CI, docs, tests | Done: 7 CPU test suites passed on GCC and MSVC Debug/Release; quality gates passed |
| 1 | Full AXF.Core implementation (events, jobs, config, time, memory, RT audit) | Done: 8 CPU test suites passed on GCC and MSVC Debug/Release (run `44526375198`); ADR 0005 |
| 2 | Audio foundation: compiled graph, RT engine skeleton, buffers, scheduling | Done: 9 CPU test suites pass on GCC and MSVC Debug/Release (Actions run `44528029411`); ADR 0006 |
| 3 | WASAPI device abstraction, shared/exclusive backends, enumeration, device→engine callbacks, disconnect recovery | Simulated tests and MSVC/GCC Debug+Release green (Actions `44532804371`); ADR 0007. **Awaiting** the hardware report in `Documentation/hardware/wasapi-test-plan.md` |
| 3b | ASIO backend and device-format negotiation | Compatibility report with real ASIO drivers |
| 4 | DSP catalogue: biquad LP/HP/peaking EQ, constant-power panner, numerical response tests | Simulated DSP verification report; ADR 0008. No listening or hardware claim |
| 5 | VST3 host | Compatibility matrix on real plugins |
| 6 | MIDI and media file I/O: 16-bit WAV, SMF format 0 | Portable WAV sample and SMF event round-trip tests; ADR 0009 |
| 7 | GUI framework: portable scene graph + Win32/Direct2D/DirectWrite window | Simulated draw-call tests; ADR 0010. Interactive checklist in `Documentation/hardware/gui-test-plan.md` still pending |
| 8 | DAW control model: tracks, clips, mixer state, versioned serialisation | Round-trip and validation tests; ADR 0011 |
| 9 | Professional hardening | Long-duration, leak, thread-safety, hardware and performance reports |

No phase is marked complete until its acceptance criteria are demonstrated by evidence.
