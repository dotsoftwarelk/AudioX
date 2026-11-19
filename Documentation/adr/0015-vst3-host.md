# ADR 0015 — Phase 5 VST3 host

- Status: Accepted; host compiled, CPU-tested and smoke-tested with real
  SDK example plug-ins; broad compatibility matrix untested
- Date: 2026-11-19
- Phase: 5
- SDK: Steinberg VST3 SDK 3.8.1 (MIT licence), cloned by CI from
  `https://github.com/steinbergmedia/vst3sdk` with the `base`,
  `pluginterfaces`, `public.sdk` and `cmake` submodules. The SDK is never
  vendored or installed with AXF.
- Evidence: Windows MSVC Debug + Release `44619637185`, portable GCC
  `44619637224`, quality `44619637199`, sanitizers `44619637383` — all green;
  MSVC reported `100% tests passed out of 12`.

## Context

`IPluginHost` existed from Phase 0 but `plugin_host.cpp` contained only a
comment. The user supplied the MIT-licensed VST3 SDK by asking AXF to obtain
the SDK itself. Since 3.8 the VST3 SDK is MIT-licensed, so AXF can consume it
without vendoring proprietary code.

## Decision

1. `AXF_ENABLE_VST3=ON` plus `AXF_VST3_SDK_ROOT` (or the same environment
   variable) enables the host. CMake fails early if the option is set without a
   valid SDK root; Windows and Linux are supported. No SDK file is copied into
   AXF or installed.
2. CI clones the official SDK with the required submodules into the runner temp
   directory and configures both GCC and MSVC jobs with VST3 enabled. SDK
   sources are compiled in a build-only static library with warnings disabled;
   they are not exported.
3. `Vst3Host::scan()` walks a directory for `.vst3` bundles, loads each module
   with `VST3::Hosting::Module`, and records every `kVstAudioEffectClass`
   entry as a `PluginDescriptor` (UID, name, vendor, version, path).
4. `Vst3Host::create()` loads the module, selects the class by UID, uses
   `PlugProvider` with a shared `HostApplication` context, and wraps the
   component/processor/controller in `IPluginInstance`.
5. `process()` maps AXF's planar `AudioBufferView` onto VST3 audio buses using
   preallocated pointer arrays, converts note-on/note-off/aftertouch MIDI into
   a preallocated VST3 `EventList`, and calls `IAudioProcessor::process()`.
   No allocation or parameter work occurs in the audio path.
6. State is read/written through the SDK's `MemoryStream`. Component and
   controller state are stored as separately length-prefixed sections with an
   `AXS1` magic header: a single shared stream made real plug-ins fail to read
   controller state. A missing or unsupported controller section is skipped on
   restore. Latency comes from `getLatencySamples()`. `reset()` toggles
   `setProcessing` because VST3 has no direct reset method.
7. Public factory `createVst3Host()` returns `nullptr` when AXF is built
   without VST3, so consumers never depend on the SDK at compile time.

## Consequences

- AXF has a real VST3 scan/create/process/state path rather than an empty
  interface. It builds and its portable control-thread paths are CPU-tested.
- The current host intentionally limits support: first audio input/output bus,
  32-bit float samples, note-on/off/aftertouch MIDI, no editor UI, no
  parameter automation, no note-expression or MIDI-CC-to-parameter mapping.
  Unsupported descriptors return errors instead of being silently accepted.
- A real-plugin smoke test was executed locally against Steinberg SDK 3.8.1
  binaries: `AGain Sample Accurate` passed unity-gain audio, finite-output and
  state round-trip checks, and `ADelay` passed scan/create/process/state. This
  is not a broad compatibility claim. The remaining matrix, long-duration run
  and third-party plug-ins are still covered by the unexecuted checklist in
  `Documentation/hardware/vst3-test-plan.md`. CI continues to skip the opt-in
  test because no plug-in binary is installed on runners.
- VST3 SDK 3.8+ is MIT-licensed; the SDK is not redistributed by AXF.
