# GUI design (Phase 7)

## Layers

```
control thread: Scene / Widget state ──→ ICanvas ──→ D2DCanvas ──→ Direct2D/DirectWrite
audio thread:   never touches GUI objects
```

`ICanvas` is a minimal portable drawing contract: `clear`, `fillRect`,
`drawText`, `present`. It contains no Windows types. `Scene` stores owning
widget pointers and renders only dirty, visible widgets that intersect the
clip rect. After a draw, widgets are marked clean; mutating setters call
`invalidate()`.

The Windows backend creates a Win32 window, a hardware render target, a solid
brush and a DirectWrite factory. UTF-8 labels are converted to wide strings
with `MultiByteToWideChar`. `WM_SIZE` resizes the render target and `WM_PAINT`
repaints the scene.

## Verification boundary

Automated tests record draw calls from a fake canvas. They prove geometry,
text content, culling, invalidation and layout logic. They do **not** prove
Direct2D/DirectWrite rendering, font availability, DPI behaviour or user
input. See `Documentation/hardware/gui-test-plan.md`.

## Not implemented

Input/event routing, resize event forwarding to widgets, high-DPI scaling,
accessibility, animation, retained layout constraints, and cross-platform
backends beyond the Windows implementation.
