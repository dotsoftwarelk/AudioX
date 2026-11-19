# Architecture overview

## Products

| Product | Target | Depends on |
|---|---|---|
| AXF SDK | Static libraries, installable via `find_package(AXF)` | Standard C++20 + Threads |
| AudioX DAW | Windows application shell | AXF public targets only |

AXF public headers live at `AXF/<Module>/include/axf/<module>/...`.
Private implementation lives at `AXF/<Module>/src/...` and is not installed.
Nothing under `AudioX/` may include a header from an AXF `src/` directory.

## Dependency layers

```
AXF.Platform
    ↓
AXF.Core ──→ AXF.Audio ──→ AXF.Graph
    │            │             ↑
    │            ├──→ AXF.DSP ─┘
    │            ├──→ AXF.Media
    │            └──→ AXF.Plugins ←── AXF.MIDI
    ├──→ AXF.Project (Core + Audio + MIDI + Graph)
    └──→ AXF.GUI      (Core + Audio)
                    ↓
                 AudioX DAW
AXF.Devices (Core) ──→ AXF.Audio engine (device callback drives it)
```

Rules:
1. Dependencies point downward only. No cycles.
2. Platform-specific headers (`windows.h`, ASIO headers) never appear in portable public APIs.
3. GUI does not own audio state; it observes snapshots.
4. The DAW consumes AXF through `AXF::*` CMake targets and public headers.

## Threading model

| Thread | Responsibility | Allowed operations |
|---|---|---|
| Audio device callback | Pull graph, process DSP/plugins, fill output | Preallocated memory only; atomics and lock-free queues; no I/O/UI/locks/allocation |
| UI thread | Draw, handle input, edit project | Communicates via lock-free queues and control messages |
| Control thread | Project edits, graph rebuild, plugin scan/load | May allocate, lock and do I/O; never blocks the audio callback |
| Worker pool (future) | Analysis, waveform peaks, scanning | No direct audio graph access |

Graph and plugin topology changes follow: control thread builds a new immutable snapshot → publishes an atomic pointer at a buffer boundary → audio thread uses the old snapshot until done → control thread retires the old snapshot. This avoids locks and use-after-free in the callback.

## Status (Phase 3)

Implemented: Core types/errors/results/SPSC queue, typed events, job system, published configuration snapshots, time/tempo conversions, aligned buffer and pool allocator, real-time audit hooks, multi-sink logger, Audio buffers/clock/transport, compiled graph and allocation-free engine, preallocated buffer pools, sample-accurate scheduler, latency propagation, Graph description and validation, DSP gain processor, MIDI event primitives, plugin/media/project/GUI interfaces, portable device manager, WASAPI shared/exclusive backends (Windows only), device→engine callback wiring, disconnect state, enumeration via `audiox --list-devices`, installable CMake package, CI.

Not implemented: hardware-verified device operation, ASIO, sample-rate conversion, full duplex within one device, DSP catalogue, VST3 host, file codecs, GUI rendering, DAW subsystems. See `Documentation/adr/0007-wasapi-device-layer.md`.
