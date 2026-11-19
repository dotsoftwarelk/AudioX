# Third-party dependencies and licensing

| Component | Version / pin | Licence | Purpose | Redistribution notes |
|---|---|---|---|---|
| GoogleTest | v1.15.2 (FetchContent, git tag) | BSD-3-Clause | Unit/integration tests | Fetched at configure time; test-only, not shipped in the SDK or DAW. Keep the copyright notice if tests are redistributed. |
| CMake | 3.25+ (build tool) | BSD-3-Clause | Build system | Not shipped. |
| Ninja | optional | Apache-2.0 | Build backend | Not shipped. |
| VST3 SDK | 3.8.1 (CI clone, commit `3cdf9ca`) | MIT (since VST 3.8) | Phase 5 plugin hosting | **Never vendored.** CI clones the official repo with `base`, `pluginterfaces`, `public.sdk` and `cmake` submodules into the runner temp dir, then sets `AXF_VST3_SDK_ROOT`. No SDK source or binary is installed with AXF. |
| ASIO SDK | 2.3.4, downloaded by Windows CI from the official Steinberg zip | Dual: Steinberg proprietary **or** GPLv3 | Phase 3b low-latency backend | **Never vendored.** CI fetches into the runner temp dir and sets `AXF_ASIO_SDK_ROOT`. AXF uses the GPLv3 option; ASIO-enabled binaries must comply with GPLv3 unless a signed Steinberg proprietary agreement is obtained. ASIO is a trademark of Steinberg. |
| fmt | not integrated | MIT | Candidate formatting library | Deferred; `std::ostream` used in Phase 0. Re-evaluate before adding. |
| spdlog | not integrated | MIT | Candidate logger | Deferred; AXF owns its `ILogSink` abstraction. |
| KissFFT | not integrated | BSD-3-Clause | Candidate FFT (Phase 4) | Deferred. |
| libsndfile | not integrated | LGPL-2.1+ | Candidate audio file I/O (Phase 6) | If used, prefer dynamic linking; document LGPL obligations before shipping. |
| FLAC | not integrated | BSD-3-Clause | Candidate FLAC codec (Phase 6) | Deferred. |

## Policy

1. Check licence, maintenance status, Windows support, performance and redistribution requirements before adding any dependency.
2. Record the decision in `Documentation/adr/` and update this file in the same commit.
3. No dependency is permitted to leak into AXF public headers without a documented reason.
4. Proprietary SDKs are detected by `Scripts/check_dependencies.py` and must stay outside the repository.
5. Credentials, tokens and private keys are never committed. CI uses secrets, never checked-in files.

## Phase 0 dependency rationale

GoogleTest is the only fetched dependency. It is battle-tested, BSD-3-Clause licensed, works with MSVC 2022, is used only by test targets, and removed from installed SDK artefacts. Everything else in Phase 0 is standard C++20.
