# ADR 0009 — MIDI and media file import/export (Phase 6)

- Status: Accepted
- Date: 2026-11-19
- Phase: 6
- Evidence: portable round-trip tests for WAV and SMF; CI IDs appended after verification.

## Context

Phase 0–5 left `AXF.Media` with format-name/extension helpers and reader/writer
interfaces, and `AXF.MIDI` with in-memory events, but no real file I/O. A DAW
needs to import/export at least one uncompressed audio format and one
 interchange format before recording and editing can be built.

## Decision

1. Implement `WavReader`/`WavWriter` in `axf/media/wav_file.hpp` for RIFF/WAVE
   16-bit PCM. Byte order is parsed/written explicitly, so no platform struct
   packing is assumed. Reader supports streaming `read()`; writer patches RIFF
   and data sizes in `close()`. Samples are converted with asymmetric 16-bit
   scaling and clipped to [-1, 1].
2. Implement `readSmf()`/`writeSmf()` in `axf/midi/smf.hpp` for Standard MIDI
   File format 0 with one track and PPQ division. Events carry absolute sample
   frames; conversion uses tempo and PPQ. The reader accepts format 0/1,
   running status, meta tempo and end-of-track; the writer emits one tempo
   event, channel voice messages, variable-length deltas and end-of-track.
3. All file I/O happens on the control thread. No file handle or allocation is
   used in the audio callback; the existing `MidiEventBufferView` remains the
   RT representation.
4. Round-trip tests verify WAV sample accuracy within 16-bit quantisation and
   SMF event identity at frame-aligned tick boundaries. Invalid headers and
   unsupported formats are regression-tested.
5. FLAC/AIFF decoding, SMF format 1 multi-track import, tempo-map export,
   SysEx, and compressed codecs remain out of scope and are documented as
   limitations.

## Consequences

- The DAW can persist and reload a 16-bit PCM mixdown and a simple MIDI
  sequence through AXF public APIs only.
- Numerical fidelity is limited by 16-bit PCM; 24-bit/float WAV and high-level
  codecs need a later milestone.
- SMF round-tripping is exact only when event frames align with the PPQ/tempo
  grid; the API documents conversion through `double` and rounding to ticks.
