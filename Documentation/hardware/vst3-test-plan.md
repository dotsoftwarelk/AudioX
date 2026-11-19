# VST3 compatibility test plan

The full matrix below is still unexecuted. A limited real-plugin smoke test was
executed locally on 2026-11-19, as recorded at the end of this document. Cloud
CI compiling the host proves only that the host builds; CI skips the opt-in
smoke test because no plug-in binary is installed there.

## Environment to record

- Windows 10/11 or Linux build, OS version, compiler, AXF commit.
- VST3 SDK version (3.8.1 in CI).
- Plugin name, vendor, version, UID, path and architecture (x64/ARM64).

## Procedure

1. Build with `-DAXF_ENABLE_VST3=ON -DAXF_VST3_SDK_ROOT=<SDK>` in Debug first.
2. `scan()` a directory containing at least one VST3 effect and one instrument.
   Record every returned descriptor; verify UIDs are stable across rescans.
3. `create()` each descriptor and record success/failure with the plugin's own
   log. Confirm a second host instance can load the same plugin.
4. `prepare()` at 44.1/48/96 kHz and 128/256/512 frames. Feed a known signal
   through a gain/EQ plugin and verify finite output and expected channel count.
5. Feed note-on/off and aftertouch events to an instrument and verify audible
   or measurable output; record which other MIDI messages are ignored.
6. `getState()`, mutate control state, `setState()` the saved bytes and verify
   the plugin returns to the prior state. Restart the host and repeat.
7. Run for 30 minutes with repeated graph republishes; watch for allocations
   in `process()` via the RT audit hook and record any crash or assertion.
8. Repeat in Release. Record latency reported by the plugin.

## Acceptance

Mark a report **plugin-verified** only after the steps above are executed with
at least three independently developed plugins and the observations recorded.
The local smoke test below is **limited plugin-verified**, not a completed
compatibility matrix.

## Executed local smoke test (2026-11-19)

- AXF commit: `bf2f668`; Release `AXF_WERROR=ON`, GCC, Linux.
- SDK: Steinberg VST3 SDK 3.8.1 at `/tmp/axf-sdks/vst3sdk`; plug-ins built from
  SDK examples in `/tmp/axf-vst3-plugins` with `SMTG_ENABLE_VSTGUI_SUPPORT=OFF`.
- Command shape:

  ```bash
  AXF_VST3_PLUGIN_PATH=/path/to/again-sample-accurate.vst3 \
  AXF_VST3_EXPECT_GAIN=1 \
  /tmp/axf-vst3-local/Tests/axf_platform_tests \
    --gtest_filter=PluginHost.Vst3RealPluginSmoke
  ```

  Set `AXF_VST3_PLUGIN_PATH` to a `.vst3` bundle or a directory of bundles.
  `AXF_VST3_EXPECT_GAIN=1` additionally asserts unity-gain output at 48 kHz.
- Result: `AGain Sample Accurate` scanned, loaded, prepared, processed a
  stereo 128-frame block with the expected 0.25 output, returned `getState()`
  bytes that `setState()` accepted, reset cleanly, and passed the finite-output
  check. `ADelay` scanned, loaded, processed and round-tripped state without
  error.
- Defect found and fixed: the original single shared `MemoryStream` for
  component + controller state made `AGain Sample Accurate` return
  "VST3 controller state read failed". State is now framed into two independent
  sections; the same smoke test passes after the fix.
- Not covered by this run: three independent vendors, instruments, MIDI note
  output, 44.1/96 kHz, 256/512-frame blocks, editor UI, 30-minute soak or
  RT-audit sampling during `process()`.
