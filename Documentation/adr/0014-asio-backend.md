# ADR 0014 — Phase 3b ASIO backend

- Status: Accepted; backend compiled and CPU-tested, hardware untested
- Date: 2026-11-19
- Phase: 3b
- SDK: Steinberg ASIO SDK 2.3.4, downloaded from
  `https://download.steinberg.net/sdk_downloads/ASIO-SDK_2.3.4_2025-10-15.zip`
  by Windows CI into the runner temp directory. The SDK is never vendored.
- Evidence: Windows MSVC Debug + Release `44617004902`, portable GCC
  `44617005191`, quality `44617004831`, sanitizers `44617004948` — all green;
  MSVC reported `100% tests passed out of 12`.

## Context

Phase 3 shipped WASAPI shared/exclusive with a simulated device→engine path,
but the `Backend::Asio` enum value was still unsupported. The user asked for
the SDK to be obtained and the backend implemented rather than left as a stub.
Since Steinberg's 2025 licensing update the ASIO SDK offers a GPLv3 option in
addition to its proprietary agreement, so an open-source build can consume it
without committing proprietary source.

## Decision

1. `AXF_ENABLE_ASIO=ON` plus `AXF_ASIO_SDK_ROOT` (or the same environment
   variable) enables the backend on Windows. CMake fails early if the option
   is set without a valid SDK root, and disables it with a status message on
   non-Windows platforms. No SDK file is copied into AXF or installed.
2. Windows CI downloads the official SDK zip, verifies `common/asio.h`, and
   configures with `-DAXF_ENABLE_ASIO=ON`. The SDK sources are compiled into a
   build-only static library with warnings disabled; they are not exported or
   installed.
3. `AsioBackend` enumerates registered ASIO drivers via the SDK's
   `AsioDrivers`, queries channel counts and common sample rates, and opens a
   driver by name. IDs are prefixed `asio:` to avoid collisions with WASAPI.
4. `AsioDevice` supports full duplex, 32-bit float channels only, the generic
   power-of-two buffer-frame contract, and one open ASIO device at a time.
   It rejects unsupported sample rates, channel counts or buffer sizes rather
   than silently resampling or changing the requested format.
5. The ASIO `bufferSwitch` callback marks the thread with
   `rt::ScopedAudioThread`, fills preallocated planar pointer arrays, invokes
   the existing `IAudioDeviceCallback`, advances an atomic frame counter, and
   calls `outputReady()`. No allocation, locking, logging or UI work occurs on
   the audio thread. Driver messages update atomic diagnostic counters only.
6. `createAsioBackend()` and `validateAsioConfig()` are part of the public
   device API. Portable tests verify validation and the nullptr factory when
   the backend is not compiled; Windows CI verifies the non-null factory and
   backend tag. No test claims real driver or device behaviour.

## Consequences

- AXF now has a real Windows ASIO backend rather than an unsupported enum
  value; it compiles and its control-thread/validation paths are CPU-tested.
- ASIO-enabled binaries use the SDK under its GPLv3 option unless a separate
  proprietary Steinberg agreement is obtained. The Apache-2.0 AXF source is
  unchanged; only the enabled binary configuration inherits the additional
  obligations.
- The SDK's global driver state allows only one open ASIO device. Full-duplex
  support is per device, unlike WASAPI's one-direction-per-device rule.
- No physical ASIO interface was present. Driver enumeration, clock modes,
  actual latency, glitch/underrun behaviour and sample-accurate timestamps
  remain hardware-verification items in
  `Documentation/hardware/asio-test-plan.md`.
- Format conversion beyond 32-bit float is deliberately unsupported; the
  backend returns `Unsupported` instead of adding a lossy conversion path.
