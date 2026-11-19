# WASAPI hardware verification plan (Phase 3)

**Status: not executed.** No audio hardware is available in the development or
cloud-CI environment. The automated tests are simulated and only prove the
portable device API, the device→engine callback contract and compilation of the
Windows backend. Nothing in this document is a hardware test result.

## Prerequisites

- Physical Windows 10/11 machine with at least one render endpoint.
- Visual Studio 2022 build tools, CMake >= 3.25, Ninja or the VS generator.
- Optional: a second capture endpoint and headphones for loopback.
- Disable system audio enhancements if deterministic measurements are needed.

## Build and simulated checks

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DAXF_WERROR=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
.\build\AudioX\audiox.exe --self-test
.\build\AudioX\audiox.exe --list-devices
```

The last command must print at least one render endpoint on a real machine. An
empty list means the device was not found; do not treat it as a pass.

## Manual hardware checklist (record results, do not assume)

| ID | Procedure | Evidence to record | Pass condition |
|---|---|---|---|
| HW-01 | Run `--list-devices` with speakers and headphones connected | Endpoint IDs, names, mix rates, channel counts | Both endpoints appear |
| HW-02 | Open each endpoint in shared mode at its advertised mix rate and 256 frames | Device ID, config, open result | Opens without error |
| HW-03 | Drive `AudioEngine` with a gain=0.5 graph for 10 s through a render callback | Audible tone level, callback count, underruns | Stable audio, no underruns |
| HW-04 | Request a mix rate different from the device mix rate | Error message | Unsupported returned, no silent resampling |
| HW-05 | Open an exclusive-mode endpoint, 48 kHz/2 ch/256 frames | Buffer size, observed callback jitter | Event-driven callbacks advance steadily |
| HW-06 | Unplug/disable the active endpoint during playback | State transition and log | `Disconnected` reported, pump exits |
| HW-07 | Add/remove a device while the app listens for changes | Notification count | Listener notified |
| HW-08 | Capture-only device, 16-bit PCM format | Sample values, channel order | Planar conversion correct |
| HW-09 | Run 30 minutes with the RT audit enabled | Allocation violations, underruns | Zero RT allocations, no growth |

Record the device name, driver version, OS build, sample rate, buffer size,
mode, callback duration statistics and underrun counts. Label the final report
**hardware-verified** only after these rows have been executed on physical
hardware; otherwise label it **simulated only**.
