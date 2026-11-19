# Development roadmap

| Phase | Scope | Exit evidence |
|---|---|---|
| 0 | Repo, CMake, core skeleton, CI, docs, tests | **Current**: builds and 7 CPU test suites pass on portable GCC; MSVC CI configured but not run locally |
| 1 | Full AXF.Core implementation (job system, events, config) | Unit tests, documented API |
| 2 | Audio foundation: buffers, clock, graph compiler, RT engine skeleton | Integration tests, allocation audit |
| 3 | WASAPI backend, then ASIO | Hardware test report on physical Windows machine |
| 4 | DSP catalogue with frequency-response/numerical tests | DSP verification report |
| 5 | VST3 host | Compatibility matrix on real plugins |
| 6 | MIDI and media | Import/export round-trip tests |
| 7 | GUI framework (Direct2D/DirectWrite) | Rendering and interaction tests |
| 8 | AudioX DAW subsystems | Feature-level integration tests |
| 9 | Professional hardening | Long-duration, leak, thread-safety, hardware and performance reports |

No phase is marked complete until its acceptance criteria are demonstrated by evidence.
