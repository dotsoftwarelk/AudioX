# ADR 0003 — Real-time audio callback contract

- Status: Accepted (Phase 0)
- Context: Dropouts and races in professional audio software are usually caused by unbounded work or synchronization in the device callback.
- Decision: The audio callback must not allocate, lock a blocking mutex, perform I/O, touch UI, wait on workers, or run unbounded loops. All buffers are preallocated. UI↔audio communication uses SPSC lock-free queues with trivially copyable payloads. Graph/plugin changes use immutable published snapshots (Phase 2+).
- Consequences: Some operations need two-phase prepare/commit designs. Unit tests alone cannot prove the contract; allocation instrumentation and long-duration hardware tests are required later.
