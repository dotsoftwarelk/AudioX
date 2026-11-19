# ADR 0012 — Phase 9 hardening pass

- Status: Accepted; first hardening pass complete, hardware/sanitizer reports pending
- Date: 2026-11-19
- Phase: 9
- Evidence: 20,000 concurrent engine callbacks with 40 graph republishes, zero RT allocation violations; 300 deterministic malformed project inputs. CI IDs appended after verification.

## Context

Earlier phases proved correctness on single-threaded paths and simulated
devices. Before calling the framework professionally usable, the atomic graph
publication path and control-thread parsers need stress and malformed-input
coverage.

## Decision

1. Add a hardening test that runs 20,000 engine callbacks on one thread while
   another thread republishes 40 graphs. The test checks finite output,
   successful commits and zero RT allocation violations under the global
   allocation hook.
2. Add deterministic malformed-input testing for `ProjectDocument::parse()`:
   300 pseudo-random strings from a fixed alphabet, with re-parse stability
   whenever parsing succeeds.
3. Keep this pass CPU-only and portable. ThreadSanitizer/AddressSanitizer,
   long-duration leak checks and real-device underrun measurement require a
   physical Windows machine and remain explicit future work.
4. ASIO and VST3 remain unimplemented because their proprietary SDKs are not
   available in this workspace; no stub is presented as a finished backend.

## Consequences

- The graph publication contract has evidence under concurrent control-thread
   changes, not only sequential unit tests.
- Parser robustness is demonstrated against arbitrary bytes, not against a
   security audit or fuzzing engine.
- The framework still cannot be labelled fully hardened until sanitizer and
   hardware reports exist.
