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

What it does **not** measure: audio callback duration, device underruns, plugin overhead, cache behaviour in a large graph, or Windows performance. MSVC Debug/Release builds and all CPU tests were verified in CI, but no MSVC performance numbers have been recorded yet.

Future reports must include MSVC version, CPU, sample rate, buffer size, channel count, graph size, plugin chain, underrun count, and p50/p95/p99 callback duration on physical hardware.

## Phase 4 portable DSP sanity benchmark

Environment: Linux virtualised sandbox, GCC 14.2, `-O3`, `AXF_WERROR=ON`, Release build. CPU model not asserted. These numbers are **sanity checks**, not audio-quality or Windows hardware claims.

Example result:

```json
{"spsc_ops_per_sec": 800473491, "gain_frames_per_sec": 3018494172, "biquad_frames_per_sec": 240840250, "checksum": 49.500, "biquad_checksum": 979.517}
```

`biquad_frames_per_sec` measures one mono biquad processing 512-frame blocks
with a volatile checksum. It is not a measure of musical quality, filter
accuracy (see the Goertzel tests), device callback duration or underruns.
