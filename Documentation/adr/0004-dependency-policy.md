# ADR 0004 — Dependency evaluation and SDK handling

- Status: Accepted (Phase 0)
- Context: JUCE is not permitted as a foundation. Steinberg SDKs have restrictive redistribution terms.
- Decision: Only standard C++20 plus GoogleTest for Phase 0. Each future dependency must have a documented licence, maintenance status, Windows compatibility, performance rationale and redistribution plan in `THIRD_PARTY.md`. VST3 and ASIO SDKs are user-supplied and never committed.
- Consequences: Some infrastructure is written in-house (logger, error types, ring buffer), keeping public APIs independent and auditable.
