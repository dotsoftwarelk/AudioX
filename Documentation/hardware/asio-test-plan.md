# ASIO hardware test plan (unexecuted)

This is a procedure, not a result. It has not been run in the development
sandbox or in cloud CI. Cloud CI compiling the ASIO backend proves only that
the code builds; it does not prove driver or interface behaviour.

## Environment to record

- Windows 10/11 build, machine, audio interface model and driver version.
- ASIO driver name as shown by `asio:` device IDs from `DeviceManager::enumerate()`.
- Sample rate, buffer size, clock source, and whether the interface is internally
  or externally clocked.
- ASIO SDK version used for the build (2.3.4 in CI).

## Procedure

1. Build with `-DAXF_ENABLE_ASIO=ON -DAXF_ASIO_SDK_ROOT=<SDK>` in Debug first.
2. Enumerate devices and record every `asio:` entry, channel count and rate list.
3. Open one device with `Backend::Asio`, 32-bit float channels, a supported rate
   and supported buffer size. Confirm a second open is rejected.
4. Start a compiled graph through `EngineDeviceCallback`; verify finite output,
   stable channel mapping and correct stream-frame progression for 10 minutes.
5. Test full duplex with input monitoring disabled, then enabled if the driver
   exposes it. Record round-trip delay observed with a loopback cable.
6. Open the driver control panel, switch sample rate and clock source while
   running; confirm the backend reports `Disconnected` on reset requests and
   does not allocate or log from `bufferSwitch`.
7. Stress the system with concurrent graph republishes and record
   `DeviceDiagnostics` callback count, underruns, overruns and callback time.
8. Repeat in Release. Record any driver crash, glitch or popup.

## Acceptance

Mark a report **hardware-verified** only after all steps above are executed and
the observations are recorded with the environment details. Until then the
backend is **simulated/compile-verified only**.
