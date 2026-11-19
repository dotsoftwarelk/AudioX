# Graph design

## Phase 0 implementation

`axf::graph::Graph` is a control-thread description of nodes and typed connections. It supports:

- node creation with declared audio/MIDI input/output port counts;
- connection validation (existence, direction, port bounds, duplicates, self-connections);
- cycle rejection at connection time;
- Kahn topological ordering and structural validation.

`processingOrder()` returns node IDs in dependency order. `hasCycle()` and `validate()` report invalid structures. The description is not thread-safe and is never touched from the callback.

## Planned compiled graph

Phase 2 adds an immutable compiled representation: resolved buffers, fixed processing order, preallocated scratch space, latency propagation and a diagnostics report. Updates are published atomically as described in ADR 0003.
