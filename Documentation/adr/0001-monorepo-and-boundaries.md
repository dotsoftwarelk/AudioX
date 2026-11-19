# ADR 0001 — Monorepo with two independent products

- Status: Accepted (Phase 0)
- Context: AXF must be reusable outside AudioX, while the DAW must be developed alongside it.
- Decision: One Git repository containing `AXF/` and `AudioX/` directories. AXF is a set of independent CMake static libraries with an installable `AXFConfig.cmake`. The DAW is a separate executable target that links only AXF public targets. AXF sources contain no reference to `AudioX/`.
- Consequences: Single CI pipeline and versioned history; a future split is a directory move plus independent CMake install. Boundary violations are detectable by review and by include-path checks.
- Alternatives rejected: two repositories (harder coordinated API changes), single monolithic library (violates modularity).
