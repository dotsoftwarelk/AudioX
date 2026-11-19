# ADR 0016 — Project control features

- Status: Accepted; locally tested and CI-verified
- Date: 2026-11-19
- Phase: 8 extension
- Evidence: local Release `AXF_WERROR=ON` build and `ctest` reporting
  `100% tests passed out of 12` suites (hardware placeholder disabled); CI
  `bf2f668` Windows MSVC `44625636487`, Portable GCC `44625636835`,
  Sanitizers `44625636593`, Quality `44625636510` — all successful.

## Context

Phase 8 delivered deterministic project serialisation but left three control
features missing: parameter automation, bounded undo/redo, and recovery after a
crash. These are control-thread concerns and must not affect the real-time
audio path.

## Decision

1. `AutomationLane` stores sorted `(frame, value)` points for gain and pan.
   Evaluation clamps outside the range and linearly interpolates between
   points. Values are finite-checked on insertion; malformed serialized lanes
   are rejected.
2. `ProjectDocument` serializes lanes as `AUTOMATION` records with a version.
   Parsing rejects duplicate, unsorted, non-finite or out-of-range data instead
   of silently repairing it.
3. `UndoStack` stores serialized before/after document snapshots with a fixed
   depth. Descriptions are static strings; all allocation happens on the
   control thread. Push prunes the oldest entry, so memory is bounded.
4. `ProjectAutosave` writes bounded numbered revisions and a manifest using
   temporary files plus atomic renames. Recovery selects the newest complete
   revision and falls back to the previous one if the newest is corrupt. There
   is a fixed maximum revision count.
5. None of these components is referenced by the audio engine. They share the
   existing project model boundary; no locks, I/O or allocation enter
   `process()`.

## Consequences

- The DAW can now represent automation, undo edits and recover an interrupted
  project without risking the audio callback.
- Undo and autosave operate on whole-document snapshots. This is simple and
  deterministic but not memory-optimal for very large projects; the depth and
  revision caps make the worst case explicit.
- Automation is linear only. Curve shapes and parameter modulation beyond
  gain/pan are future work.
