#pragma once

// Phase 0 ships AXF as static libraries. A single empty API macro keeps public
// headers portable; per-module dllimport/dllexport macros will be introduced
// together with shared-library support in a future ADR.
#define AXF_API
