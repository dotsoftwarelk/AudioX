# ADR 0017 — Preallocated recording path

- Status: Accepted; locally tested, CI verification pending
- Date: 2026-11-19
- Phase: 6 extension
- Evidence: local Release `AXF_WERROR=ON` build and `ctest` reporting
  `100% tests passed out of 12` suites; the new recorder cases pass and the RT
  audit confirms no allocation in `capture()`.

## Context

The media module could read and write WAV files, but there was no way to
capture a live device callback. Recording must never allocate, lock or perform
file I/O on the audio thread, and overrun must be visible rather than silently
corrupting audio.

## Decision

1. `AudioRecorder::start()` allocates a fixed interleaved PCM float buffer on
   the control thread. `capture()` only copies preallocated input blocks into
   that buffer and is real-time safe.
2. Frames beyond capacity are counted as dropped, not wrapped or truncated.
   `capturedFrames()`, `droppedFrames()` and `capacityFrames()` expose the
   result for diagnostics.
3. `stopAndWrite()` stops capture and writes the recorded frames as 16-bit PCM
   WAV from the control thread. The existing deterministic WAV writer is reused.
4. `RecordingDeviceCallback` records the input block and then forwards the
   callback to the engine. It owns no state beyond references and performs no
   allocation.
5. The recorder does not resample, dither or compress. Input must already be at
   the configured sample rate and channel count.

## Consequences

- Recording an unbounded session is not supported; the caller must choose a
  maximum duration and can inspect dropped frames.
- Buffer allocation can fail before recording starts, which is reported
  through `core::Result` instead of throwing into the audio path.
- 16-bit PCM matches the current WAV writer; 24-bit/float output remains future
  work.
- CI results are not yet available for these changes; the local Werror build
  and tests are the current evidence.
