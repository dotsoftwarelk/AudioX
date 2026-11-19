# Audio engine design (Phase 2 implemented)

## Ownership

- `AudioBuffer` owns preallocated planar float storage, created on a control thread.
- `AudioBufferView` is a non-owning view passed through processors.
- `AudioClock` and `Transport` use atomics for cross-thread observation; the audio thread advances the clock.
- The future engine owns a compiled graph snapshot; nodes are owned by the snapshot, never by the callback.

## Implemented in Phase 2

`compileGraph()` produces an immutable `CompiledGraph` from a validated description.
`AudioEngine::commit()` publishes it atomically; superseded graphs are retained
for safe reclamation. `CompiledGraph::process()` clears preallocated buffers,
copies device input into the input node, mixes resolved connections in
topological order, invokes node processors and copies the output node to device
buffers. The whole callback is wrapped in `rt::ScopedAudioThread` and verified
allocation-free by the audit test.

## Callback outline (current)

1. Drain a bounded, preallocated control-event queue.
2. If a new graph snapshot was published, swap the audio thread's local pointer.
3. Clear output buffers, pull input buffers from the device into preallocated storage.
4. Traverse the compiled graph in validated topological order.
5. Run DSP and plugin processors with `noexcept` process methods.
6. Copy/wet outputs to the device, update meters through lock-free atomics, advance the clock.

## Latency

No maximum track count or latency figure is claimed. Latency must be measured per device, buffer size and session using real hardware. Reported latency will come from the device backend plus plugin/graph delay estimates.
