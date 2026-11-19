# ADR 0007 — Windows device layer (WASAPI shared mode)

- Status: Accepted
- Date: 2026-11-19
- Phase: 3

## Context

Phase 2 delivered an allocation-free `AudioEngine` that is driven manually by
tests. To turn AXF into a usable SDK on Windows, the engine must be driven by
real audio device callbacks, and the application must be able to discover and
configure endpoints. The implementation must not leak Windows headers into the
portable public API and must keep GCC and MSVC Debug/Release builds green.

## Decision

1. Extend `axf/devices/device.hpp` with state (`DeviceState`), diagnostics,
   WASAPI-specific validation and a change-listener contract. Add
   `axf/devices/device_manager.hpp` with a `DeviceManager` that aggregates
   backends, opens devices, chooses default endpoints and broadcasts change
   notifications.
2. Keep all Windows types inside the private source `wasapi_backend.cpp`,
   compiled only when `WIN32` and `AXF_HAS_WASAPI` are set. The public
   `createWasapiBackend()` returns `nullptr` on unsupported platforms.
3. Use WASAPI **shared mode** as the primary backend. Exclusive mode with an
   event-driven callback is included as a second backend for measured
   comparisons, but limited to two channels in this phase. ASIO is not
   implemented.
4. WASAPI callbacks are converted to the engine's planar `float` buffers. The
   endpoint format is detected (32-bit IEEE float or 16-bit PCM) and converted
   in preallocated storage. Shared mode uses the device mix format and rejects
   a sample-rate mismatch rather than silently resampling.
5. `axf/devices/engine_callback.hpp` provides `EngineDeviceCallback`, a public
   adapter that forwards device planar buffers straight into
   `AudioEngine::process()`. The device→engine path is therefore part of the
   SDK, not only a test fixture.
6. A device is one direction only. Full duplex requires two devices because the
   first implementation uses one `IAudioClient` per device. Disconnects are
   detected from WASAPI error HRESULTs; the pump exits and reports
   `DeviceState::Disconnected`.
7. Change notifications use `IMMNotificationClient` and are forwarded through
   `DeviceManager` to application listeners. Listener callbacks are `noexcept`
   and must not block.
8. `audiox --list-devices` exposes enumeration to operators for hardware test
   reports. Hardware tests remain opt-in and are never claimed as CI evidence.

## Consequences

- The public API stays portable; Linux consumers compile with no Windows SDK.
- `AudioEngine` gains a real device driver without changing its RT contract:
  the backend only calls `onAudioCallback()` with preallocated planar buffers.
- No device underrun/overrun guarantee is claimed without hardware measurement.
- Latency reported by `reportedLatencySeconds()` is the device buffer duration,
  an estimate — not an end-to-end measured latency.
- Exclusive mode, sample-rate conversion, duplex and ASIO remain explicit
  limitations until later phases.
