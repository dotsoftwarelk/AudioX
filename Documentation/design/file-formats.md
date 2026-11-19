# File format design (Phase 6)

## WAV

`WavReader` and `WavWriter` implement the existing `IAudioFileReader` /
`IAudioFileWriter` contracts. Supported subset: RIFF/WAVE, PCM format tag 1,
16 bits per sample, any channel count accepted by `validate`-style limits
(1–512). Unknown chunks are skipped with RIFF padding. Files are opened on the
control thread; `read()` returns the number of frames actually decoded.

Encoding is explicit little-endian. Positive samples use `/32767`, negative
samples `/32768`. Clipping and non-finite values are handled before conversion.

## Standard MIDI File

`SmfDocument` holds a PPQ division, one tempo (microseconds per quarter note)
and events with absolute sample frames. `writeSmf()` sorts events, converts
frames to ticks, emits variable-length deltas, a tempo meta event and
end-of-track. `readSmf()` accepts format 0/1, reads only the first track,
supports running status, and converts delta ticks into frames incrementally so
tempo changes affect later events.

Round-trip tests use 48 kHz, PPQ 960 and 120 BPM, where one tick is exactly 25
frames. This makes the test deterministic without hiding real conversion
behaviour: deltas above 127 exercise multi-byte VLQ encoding.

## Not implemented

24-bit/float WAV, AIFF and FLAC codecs, multi-track SMF export, SysEx,
arbitrary tempo maps, SMF time signatures, and streaming from network sources.
