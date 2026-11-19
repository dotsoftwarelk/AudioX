# DSP design

## Phase 4 processors

| Processor | Contract | RT state | Configuration |
|---|---|---|---|
| `GainProcessor` | `INodeProcessor` | one atomic float | `setGain()` from control thread |
| `BiquadProcessor` | `INodeProcessor` + `IProcessor` | fixed 8-channel IIR state | immutable coefficients from `designBiquad()` |
| `PannerProcessor` | `INodeProcessor` | atomic pan | `setPan()` from control thread |

`designBiquad()` uses the RBJ cookbook forms for low-pass, high-pass and
peaking EQ. It normalises every coefficient by `a0`; a missing normalisation
was found and fixed during Phase 4 with a regression test.

`BiquadProcessor::prepare()` allocates nothing. A filter change is a control
thread operation: create a new processor, install it with
`AudioEngine::setProcessor()`, build/republish the graph; the old graph remains
alive until no callback can reference it. This avoids lock-free coefficient
snapshots in this phase.

## Numerical verification

Tests generate a sine at a known frequency, discard a warm-up period, then
measure the output amplitude with a Goertzel bin at the same frequency. The
suite checks:

- low-pass passes 200 Hz and attenuates 8 kHz;
- high-pass does the inverse;
- +6 dB peaking boosts 1 kHz and leaves 100 Hz near unity;
- reset clears the filter tail;
- panner centre is equal-power and hard pans reach the expected extremes;
- all Phase 4 processors produce zero allocations on a marked audio thread.

These are numerical tests, not listening tests or hardware measurements.

## Not yet implemented

Compressor, limiter, effects, convolution, resampling, oversampling, dynamic
filter automation and per-sample parameter smoothing.
