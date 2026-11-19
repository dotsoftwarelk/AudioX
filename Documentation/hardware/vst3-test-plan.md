# VST3 compatibility test plan (unexecuted)

This is a procedure, not a result. No real plugin was loaded in the sandbox or
in cloud CI. Cloud CI compiling the host proves only that the host builds.

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
Until then the host is **compile/CPU-verified only**.
