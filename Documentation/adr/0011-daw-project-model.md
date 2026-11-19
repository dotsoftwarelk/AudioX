# ADR 0011 — Phase 8 DAW project model

- Status: Accepted
- Date: 2026-11-19
- Phase: 8
- Evidence: portable track/clip/serialisation tests; CI IDs appended after verification.

## Context

Phase 7 produced a usable AXF GUI layer, but `AXF.Project` only had manifest
version helpers. The DAW needs a control-thread document model for tracks,
clips and mixer state that consumes AXF public APIs and never touches the
audio callback.

## Decision

1. Add `axf/project/document.hpp` with `Track`, `Clip` and `ProjectDocument`.
   The document owns monotonically increasing IDs, track gain/pan/mute/solo,
   and ordered clips with start/length/source.
2. All mutations are control-thread only and validate ranges: tempo 20–300 BPM,
   gain 0–4, pan −1…1, non-zero clip length and no integer overflow in ranges.
3. `serialize()` / `parse()` use a small tab-delimited, escaped text format
   with a version header and `END` sentinel. It is deterministic and has no
   third-party dependency. Unknown records, corrupt numbers and missing
   sections are errors, not silently ignored.
4. Parsed documents continue ID allocation above the largest stored ID, so new
   tracks/clips cannot collide with loaded ones.
5. Audio playback, recording, automation lanes and undo/redo are out of scope
   for this phase.

## Consequences

- The DAW can create/save/reload a basic session through AXF public APIs.
- Text serialisation is human-readable but not a final interchange format;
  binary/JSON formats, autosave and recovery need later work.
- GUI binding to the document remains a later integration step.
