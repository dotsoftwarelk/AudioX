# Build instructions

## Prerequisites

- Windows 10/11: Visual Studio 2022 with Desktop C++ workload and Windows 10/11 SDK.
- CMake 3.25+; Git; Ninja optional.
- Network access on first configure for GoogleTest FetchContent.
- VST3/ASIO SDKs are not needed for Phase 0.

## Commands

See the root `README.md`. Key options:

| Option | Default | Meaning |
|---|---|---|
| `AXF_BUILD_TESTS` | ON | Fetch GoogleTest and build test targets |
| `AXF_BUILD_APP` | ON | Build the `audiox` diagnostic shell |
| `AXF_WERROR` | OFF | Warnings as errors (CI enables it) |
| `AXF_ENABLE_VST3` | OFF | Reserved for Phase 5 |
| `AXF_ENABLE_ASIO` | OFF | Reserved for Phase 3 |

## Installed artefacts

`cmake --install` places headers under `include/axf/`, static libraries under `lib/`, the `audiox` shell under `bin/`, and CMake package files under `lib/cmake/AXF/`. `Examples/minimal_app` demonstrates standalone consumption.
