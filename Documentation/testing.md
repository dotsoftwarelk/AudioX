# Testing procedures

## Automated CPU tests

Run locally:

```bash
Scripts/run_tests.sh
```

CI runs the same suite with MSVC Debug + Release (MSVC 19.51/19.52, `/W4 /WX`, generator-independent Ninja) and portable GCC Debug + Release, with warnings as errors. Verified on 2026-11-19: Phase 9 portable suites pass locally; Windows compilation and the full suite were verified in GitHub Actions on
2026-11-19: Portable GCC `44619637224`, Windows MSVC `44619637185`, Quality
`44619637199`, Sanitizers `44619637383` — all successful, with 12/12 enabled
tests passing in both Release and Debug and in both sanitizer modes. These runs
include the ASIO and VST3 SDK builds. Earlier Phase 10 IDs: GCC `44613628281`,
MSVC `44613628173`, Quality `44613628147`, Sanitizers `44613628203`; Phase 9
IDs: GCC `44549680971`, MSVC `44549681036`, Quality `44549680864`. Earlier run IDs: Phase 8 `44544378901` / `44544379083` /
`44544378963`; Phase 7 `44542472580` / `44542472445` / `44542472569`; Phase 6
`44539857623` / `44539857549` / `44539857581`; Phase 4 `44536414501` /
`44536414626` / `44536414437`; Phase 3 `44532804211` / `44532804371` /
`44532804250`. No real audio device or display was present.

| Suite | Covers |
|---|---|
| `axf_core_tests` | conversions, Result/Error, SPSC FIFO, events, job system, config snapshots/file IO, time/tempo, aligned/pool memory, thread utilities, multi-sink logger and RT logging guard |
| `axf_rt_audit_tests` | global allocation interception on a marked audio thread, including 16 engine callbacks, 32 Phase 4 DSP callbacks, 20k concurrent callbacks with 40 graph republishes, and 300 malformed project inputs |
| `axf_engine_tests` | compiled graph processing, gain accuracy, channel mismatch rejection, graph republishing, 2000-block stability, buffer pool, sample-accurate scheduler, latency propagation |
| `axf_audio_tests` | clock, transport state and position |
| `axf_dsp_tests` | gain accuracy, channel integrity, denormal/NaN handling, biquad coefficient validation, Goertzel frequency response for LP/HP/peaking EQ, reset tail, constant-power panning |
| `axf_midi_tests` | event encoding and bounded event buffer |
| `axf_graph_tests` | validation, duplicate ports, cycle rejection, ordering, MIDI ports |
| `axf_platform_tests` | version, media extensions, project format, plugin contract, VST3 empty-directory scan and invalid descriptor rejection, track/clip model, serialisation round-trip and corrupt-input rejection |
| `axf_file_io_tests` | WAV 16-bit PCM stereo round-trip and invalid input; SMF format 0 event round-trip with VLQ deltas and invalid input |
| `axf_gui_tests` | widget draw geometry, text, meter scaling, dirty invalidation, clip culling, vertical layout, non-Windows window stub |
| `axf_devices_tests` | device config, ASIO validation/factory availability and WASAPI restrictions, backend aggregation, default selection, listener notification, fake device→engine callback driving 0.8 gain with correct channel mapping, simulated disconnect |
| `axf_benchmarks` | SPSC throughput and gain throughput sanity check |

## Sanitizer jobs

Run both locally before pushing:

```bash
Scripts/run_sanitizers.sh address   # ASan + UBSan + leak detection
Scripts/run_sanitizers.sh thread    # TSan
```

The jobs build fresh Ninja trees with `AXF_WERROR=ON` and run the same 12
enabled CPU tests. The hardware placeholder stays disabled. Sanitizer runtimes
replace global `operator new`; therefore the RT audit test that asserts
violation counting calls `rt::noteAllocation()` directly, and the separate
global-override test is skipped with an explicit reason under ASan/TSan. Normal
Debug/Release builds still execute the global-override test.

The first local sanitizer pass found a real UBSan defect: `ScheduledEvent::payload`
was read through a 4-byte-aligned `Payload*` while the byte array had only 1-byte
alignment. `payload` is now a naturally aligned `std::uint64_t[4]` (an explicit
`alignas` would trigger MSVC C4324), and the scheduler test passes under UBSan. No address, leak or data-race reports remain in the exercised paths.

Verified on 2026-11-19: Portable GCC `44613628281`, Windows MSVC
`44613628173`, Quality `44613628147`, Sanitizers `44613628203` — all
successful. MSVC reported `100% tests passed out of 12` in both Debug and
Release; the sanitizer matrix reported the same 12/12 for both address and
thread modes.

## Hardware tests (not in cloud CI)

The Phase 3 plan is `Documentation/hardware/wasapi-test-plan.md`. The Phase 7
interactive GUI checklist is `Documentation/hardware/gui-test-plan.md`. Both are
unexecuted here. It is
**untagged as executed**: HW-01…HW-09 are procedures to run on a physical
Windows 10/11 machine, not results. Use `audiox --list-devices` to capture
endpoint IDs and mix rates, then open a device in shared mode at that rate and
drive the compiled engine. Record device name, driver version, OS build, sample
rate, buffer size, shared/exclusive mode, callback duration statistics and
underruns. Mark a report **hardware-verified** only after the checklist has been
executed; otherwise mark it **simulated only**. Cloud CI compiling the WASAPI
source is not proof of device behaviour.

## Rules

- Never weaken a test to obtain a green build.
- Every bug fix gets a regression test where practical.
- Never claim that cloud CI proves device behaviour.
