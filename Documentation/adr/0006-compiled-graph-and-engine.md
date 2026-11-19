# ADR 0006 — Compiled graph and Phase 2 engine

- Status: Accepted (Phase 2)
- Context: The Phase 0 graph is a mutable control-thread description. The audio callback needs an immutable, preallocated plan with deterministic order and no locking.

## Decisions

1. **Compile, then publish.** `compileGraph()` turns a validated `graph::Graph` into `CompiledGraph`. `AudioEngine::commit()` publishes a pointer with release semantics; the audio thread loads it with acquire semantics. Superseded graphs are retained in an engine-owned history vector and reclaimed at engine destruction, avoiding use-after-free without locks.
2. **All buffers are preallocated.** Each node port gets an `AudioBuffer` from a pre-sized vector at compile time. `process()` clears and reuses them; no allocation occurs on the audio thread.
3. **Topological mixing.** Connections are resolved to indices in the compiled order. At each node's turn its input buffers are mixed from already-processed upstream nodes, then its processor runs. Multiple connections to one port sum into the preallocated input buffer.
4. **Terminal nodes are engine-owned.** `AudioInput` and `AudioOutput` nodes are copied directly to and from device buffers; they have no user processor. This keeps channel mapping explicit and avoids accidental overwrites.
5. **Latency is propagated** as `own latency + max(upstream latency)` along audio edges; the reported engine latency is the maximum across output nodes.
6. **Events are sample-accurate.** `EventScheduler` uses a fixed-capacity, allocation-free sorted array keyed by frame and insertion sequence. It is independent of the graph until Phase 6 integration.

## Consequences

Graph edits are control-thread operations and can be arbitrarily slow; they never block audio. Channel counts must match the engine configuration at compile time (documented limitation). MIDI connections are counted in diagnostics but not processed until Phase 6. Mixing has no built-in clipping stage yet; output must stay in range or be handled by a downstream processor.
