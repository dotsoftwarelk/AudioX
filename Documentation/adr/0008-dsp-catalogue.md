# ADR 0008 — Phase 4 DSP catalogue

- Status: Accepted
- Date: 2026-11-19
- Phase: 4
- Evidence: Numerical Goertzel response tests, RT allocation audit; GCC `44536414501`, MSVC `44536414626`, Quality `44536414437` (all green)

## Context

Phase 2/3 shipped gain and the real-time engine, but no frequency-selective
processing. A DAW needs at minimum stable filters/EQ and panning before mixer
work can begin. The implementation must remain allocation-free in callbacks,
portable, and verifiable without audio hardware.

## Decision

1. Add `axf::dsp::BiquadProcessor`, implementing both `audio::INodeProcessor`
   and `dsp::IProcessor`. Coefficients are immutable and designed on the
   control thread with `designBiquad()` (RBJ cookbook low-pass, high-pass and
   peaking EQ). State is a fixed eight-channel array, so `process()` performs
   no allocation and supports up to eight channels.
2. Add `axf::dsp::PannerProcessor`, a constant-power mono→stereo panner with an
   atomic pan control. Filter/EQ changes are made by constructing a new
   processor and republishing the compiled graph, matching the Phase 2
   atomic-publication model; no coefficient double-buffering is needed in the
   callback.
3. Denormals are flushed to zero. Non-finite pan is treated as centre.
   Coefficient design validates sample rate, Nyquist, Q and finite gain.
4. Numerical verification uses a Goertzel measurement of steady-state sine
   response (after warm-up) rather than claims of "audiophile quality":
   low-pass/high-pass attenuation and +6 dB peaking boost are asserted with
   explicit tolerances.
5. A regression test caught a missing `a0 = 1 + alpha` normalisation in the
   original low-pass/high-pass paths, which made those filters unstable. The
   coefficient assertions and response tests now guard that bug.
6. Compressor, limiter, convolution, resampling and oversampling remain out
   of scope for this phase and are recorded as limitations.

## Consequences

- The engine can place real filters and a panner in a compiled graph through
  the existing public `INodeProcessor` interface; the DAW will use only AXF
  public APIs.
- Frequency-response evidence is numerical and toolchain-independent, but it
  is not a listening test or a hardware measurement.
- The eight-channel cap and immutable-coefficient rule must be documented for
  consumers; dynamic filter automation needs a future lock-free parameter
  snapshot design.
