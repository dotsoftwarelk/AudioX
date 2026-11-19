# Testing procedures

## Automated CPU tests

Run locally:

```bash
Scripts/run_tests.sh
```

CI runs the same suite with MSVC Debug + Release (MSVC 19.51/19.52, `/W4 /WX`, generator-independent Ninja) and portable GCC Debug + Release, with warnings as errors. Verified on 2026-11-19: Phase 3 portable suites pass locally; Windows compilation and the full suite are verified in GitHub Actions.

| Suite | Covers |
|---|---|
| `axf_core_tests` | conversions, Result/Error, SPSC FIFO, events, job system, config snapshots/file IO, time/tempo, aligned/pool memory, thread utilities, multi-sink logger and RT logging guard |
| `axf_rt_audit_tests` | global allocation interception on a marked audio thread, including 16 engine callbacks with zero allocations |
| `axf_engine_tests` | compiled graph processing, gain accuracy, channel mismatch rejection, graph republishing, 2000-block stability, buffer pool, sample-accurate scheduler, latency propagation |
| `axf_audio_tests` | clock, transport state and position |
| `axf_dsp_tests` | gain accuracy, channel integrity, denormal and NaN handling |
| `axf_midi_tests` | event encoding and bounded event buffer |
| `axf_graph_tests` | validation, duplicate ports, cycle rejection, ordering, MIDI ports |
| `axf_platform_tests` | version, media extensions, project format, plugin contract |
| `axf_devices_tests` | device config and WASAPI restrictions, backend aggregation, default selection, listener notification, fake device→engine callback driving 0.8 gain with correct channel mapping, simulated disconnect |
| `axf_benchmarks` | SPSC throughput and gain throughput sanity check |

## Hardware tests (not in cloud CI)

The Phase 3 plan is `Documentation/hardware/wasapi-test-plan.md`. It is
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
