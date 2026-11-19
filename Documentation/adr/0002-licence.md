# ADR 0002 — Apache-2.0 for AXF and AudioX

- Status: Accepted (Phase 0)
- Context: A professional audio SDK needs an explicit patent grant and permissive reuse terms. Proprietary SDKs used by the product have their own redistribution restrictions.
- Decision: License original AXF and AudioX code under Apache-2.0. Keep Steinberg VST3/ASIO SDKs out of the repository. Record every dependency in `THIRD_PARTY.md`.
- Consequences: Contributors grant patent rights; LGPL components must be handled carefully (dynamic linking or equivalent compliance) if added later.
