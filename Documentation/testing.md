# Testing procedures

## Automated CPU tests

Run locally:

```bash
Scripts/run_tests.sh
```

CI runs the same suite with MSVC Debug + Release (MSVC 19.51/19.52, `/W4 /WX`, generator-independent Ninja) and portable GCC Debug + Release, with warnings as errors. Verified on 2026-11-19: all 7 CPU test suites passed on both toolchains.

| Suite | Covers |
|---|---|
| `axf_core_tests` | conversions, Result/Error, SPSC FIFO, events, job system, config snapshots/file IO, time/tempo, aligned/pool memory, thread utilities, multi-sink logger and RT logging guard |
| `axf_rt_audit_tests` | global allocation interception on a marked audio thread |
| `axf_audio_tests` | clock, transport state and position |
| `axf_dsp_tests` | gain accuracy, channel integrity, denormal and NaN handling |
| `axf_midi_tests` | event encoding and bounded event buffer |
| `axf_graph_tests` | validation, duplicate ports, cycle rejection, ordering, MIDI ports |
| `axf_platform_tests` | version, media extensions, project format, plugin contract |
| `axf_benchmarks` | SPSC throughput and gain throughput sanity check |

## Hardware tests (not in cloud CI)

Create tagged tests when real device support lands. Run on a physical Windows 10/11 machine with WASAPI and, if available, an ASIO driver. Record device name, driver version, sample rate, buffer size, shared/exclusive mode, underruns and measured callback duration. Mark results clearly as hardware-verified.

## Rules

- Never weaken a test to obtain a green build.
- Every bug fix gets a regression test where practical.
- Never claim that cloud CI proves device behaviour.
