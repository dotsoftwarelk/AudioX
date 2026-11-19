# GUI interactive verification plan (Phase 7)

**Status: not executed.** The sandbox has no display and cloud CI has no
interactive desktop session. MSVC compilation does not prove visual output.

## Manual checklist (record results; do not assume)

| ID | Procedure | Pass condition |
|---|---|---|
| GUI-01 | Build the Windows target with MSVC Debug and Release | Both configurations compile and link |
| GUI-02 | Create a window through `axf::gui::createWindowsWindow()` | Window appears with the dark theme background |
| GUI-03 | Add a `Panel`, `Label` and `Meter` to the scene | All three render at their bounds |
| GUI-04 | Change the meter level and repaint | Bar length changes; no stale pixels |
| GUI-05 | Resize the window | Render target resizes without distortion or crash |
| GUI-06 | Close the window | Message loop exits, resources released |
| GUI-07 | Run for 10 minutes with a scene | No visible leak or unbounded CPU usage |

Record OS build, DPI scaling, GPU/driver, font availability and screenshots.
Label the report **GUI-verified** only after these steps run interactively;
otherwise label it **compiled/simulated only**.
