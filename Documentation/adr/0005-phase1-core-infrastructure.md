# ADR 0005 — Phase 1 core infrastructure

- Status: Accepted (Phase 1)
- Context: AXF needs events, background work, configuration and time/memory utilities without compromising the real-time contract.

## Decisions

1. **Events are fixed-size, trivially copyable records.** `EventBus::tryPost()` copies an event into a preallocated 256-slot SPSC queue; `dispatch()` runs handlers on the control thread. Payloads are capped at 64 bytes and must expose a static `typeId`. This keeps posting allocation-free and avoids global mutable type registries.
2. **Jobs run on a fixed control-thread pool.** `JobSystem` has three priority deques and `waitForAll()`. Audio threads must never submit or wait. Worker exceptions are contained.
3. **Configuration uses immutable published snapshots.** Mutators operate on a control-thread map and publish an `atomic<std::shared_ptr<const ConfigSnapshot>>`. Audio threads read through `withSnapshot()` with no lock and no allocation; numeric access uses `from_chars` rather than locale-dependent parsing.
4. **Memory utilities are explicit.** `AlignedBuffer` and `MonotonicPool` allocate once at construction. `allocate()` is a bump pointer and cannot allocate system memory.
5. **Real-time auditing is diagnostic, not enforcement.** Threads used by device callbacks are marked with `rt::ScopedAudioThread`. An optional, test-only global `operator new` override reports allocations on marked threads through a hook. It is never installed with the SDK.
6. **Logging never blocks an audio thread.** `Logger` supports multiple sinks and a minimum level, but if called from a marked audio thread it drops the record and increments the audit counter instead of taking its mutex.

## Consequences

Event payloads and counts are bounded by design; larger events must travel as control-thread messages. Configuration reads are cheap but snapshots are immutable, so readers must not retain views beyond the snapshot lifetime. The global allocation override is linked only into a dedicated test target.
