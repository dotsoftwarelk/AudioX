# Performance records

## Phase 0 portable sanity benchmark

Environment: Linux virtualised sandbox, GCC 14.2, `-O3`, `AXF_WERROR=ON`, Release build. CPU model not asserted. These numbers are **sanity checks**, not Windows audio performance claims.

Command: `./build/Tests/axf_benchmarks`

Example result:

```json
{"spsc_ops_per_sec": 806087030, "gain_frames_per_sec": 2872257471, "checksum": 49.500}
```

What it measures:
- `spsc_ops_per_sec`: combined push/pop operations on a 1024-slot SPSC queue with monotonically increasing indices.
- `gain_frames_per_sec`: stereo gain processing throughput with a volatile checksum that prevents the loop from being optimised away.

What it does **not** measure: audio callback duration, device underruns, plugin overhead, cache behaviour in a large graph, or Windows/MSVC performance.

Future reports must include MSVC version, CPU, sample rate, buffer size, channel count, graph size, plugin chain, underrun count, and p50/p95/p99 callback duration on physical hardware.
