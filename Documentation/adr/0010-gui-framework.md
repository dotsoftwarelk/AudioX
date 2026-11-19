# ADR 0010 — Phase 7 GUI framework

- Status: Accepted; simulated scene tests complete, interactive Windows verification pending
- Date: 2026-11-19
- Phase: 7
- Evidence: portable scene/widget tests; GCC `44542472580`, MSVC `44542472445`, Quality `44542472569` (all green). No window was rendered in the sandbox.

## Context

Phase 6 completed file I/O, but `AXF.GUI` contained only a theme struct and no
drawable widgets or native window. The DAW will need a UI, but AXF must stay a
framework: the application may only consume public APIs, and Windows-specific
headers must not leak into portable headers.

## Decision

1. Add a portable control-thread scene graph: `ICanvas`, `Widget`, `Panel`,
   `Label`, `Meter` and `Scene`. Widgets own bounds, visibility and a dirty
   flag; `Scene::render()` draws only visible dirty widgets intersecting the
   clip rectangle. `Scene::layout()` provides a simple vertical stack.
2. Add `IWindow` and a real Win32/Direct2D/DirectWrite implementation in the
   private source `win32_window.cpp`, compiled only for Windows. The public
   `createWindowsWindow()` returns `nullptr` on other platforms.
3. The native window owns the render target, brush and DirectWrite factory;
   painting clears with the theme background and delegates to `Scene::render`.
   Text is drawn with Segoe UI via DirectWrite.
4. Tests use a `RecordingCanvas` to verify draw geometry, text, culling,
   invalidation and layout without a display. Interactive rendering must be
   checked manually on Windows and is recorded as a hardware/GUI test plan,
   not as CI evidence.
5. GUI work is strictly control/UI-thread; no GUI state is read by the audio
   callback. Meters must be fed from lock-free snapshots in a later DAW phase.

## Consequences

- A real Windows window can be created and painted, but appearance, DPI
  scaling and input behaviour are not verified here.
- The scene graph is intentionally small; no retained-mode layout engine,
  theming engine, accessibility tree or GPU effects are claimed.
- Event routing, drag/drop, IME and high-DPI scaling remain future work.
