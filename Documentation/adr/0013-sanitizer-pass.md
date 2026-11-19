# ADR 0013 — Phase 10 sanitizer and alignment pass

- Status: Accepted
- Date: 2026-11-19
- Phase: 10
- Evidence: local `Scripts/run_sanitizers.sh address` (ASan + UBSan) and
  `Scripts/run_sanitizers.sh thread` (TSan) both reported `100% tests passed
  out of 12`; the hardware placeholder remains disabled. CI workflow
  `.github/workflows/sanitizers.yml` runs both modes on every push and PR.

## Context

Phase 9 added concurrency and malformed-input stress tests, but every result
came from ordinary Debug/Release builds. Two classes of defect were still
possible: undefined behaviour that does not reliably crash, and races that
only a ThreadSanitizer build can report. A long-term professional codebase
needs repeatable sanitizer jobs, not a one-off local command.

## Decision

1. Add `Scripts/run_sanitizers.sh`, which configures a fresh Ninja build with
   `-fsanitize=address,undefined` (plus leak detection) or
   `-fsanitize=thread`, builds with warnings as errors, and runs the full CPU
   suite. The build directory is overridable with `AXF_SANITIZER_BUILD_DIR`.
2. Add `.github/workflows/sanitizers.yml` with `address` and `thread` jobs.
3. Fix the misaligned `ScheduledEvent::payload` access found by UBSan:
   `payload` is now `alignas(8) std::byte[32]`, matching the alignment required
   by event payload structs such as `Payload`.
4. Make the RT audit allocation test independent of global operator new.
   Sanitizer runtimes legitimately replace the global allocation functions,
   so the core assertion now calls `rt::noteAllocation()` and
   `rt::noteRealtimeViolation()` directly. A separate normal-build test still
   verifies the global allocation override; it is skipped under ASan/TSan with
   an explicit reason instead of failing for the wrong reason.

## Consequences

- The CPU suite now has repeatable undefined-behaviour, address, leak and race
  coverage in both local and cloud builds.
- The scheduler payload alignment bug has a regression test: the existing
  sample-accurate scheduler test now passes under UBSan.
- Sanitizers are still Linux/GCC only. They do not prove WASAPI/ASIO hardware
  behaviour, Windows-specific memory layout, or GUI/DPI interaction.
- ThreadSanitizer reports no data race in the exercised paths; this is not a
  formal proof of race freedom for all untested code.
