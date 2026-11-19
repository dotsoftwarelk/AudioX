# Coding standards

## General

- C++20, no compiler extensions (`CMAKE_CXX_EXTENSIONS OFF`). C++23 only with toolchain evidence and an ADR.
- RAII, strong types, explicit ownership, `const` correctness, small focused classes.
- No global mutable state or singletons for audio state. `Logger::instance()` is a controlled exception for process-wide logging only.
- Public APIs are documented in headers. Private code lives in `src/`.
- Named types use `PascalCase`; functions/methods `camelCase`; members `trailingUnderscore_`; constants `kPascalCase`; enum values `PascalCase`.
- Namespaces: `axf::<module>`. Platform-specific code stays out of public headers.

## Error handling

- Use `axf::core::Result<T>` at module boundaries; reserve exceptions for allocation failure and truly exceptional control-thread conditions.
- Errors carry an `ErrorCode`, static string message and source location; never allocate when creating an `Error`.
- Always handle or explicitly discard result values (`[[nodiscard]]`).

## Real-time code

- Audio-thread functions are `noexcept`, allocation-free and lock-free.
- Use `SpscRingBuffer` for trivially copyable payloads between one producer and one consumer.
- Never log, format strings, open files or touch UI from the audio thread.
- Document thread affinity on every public API that has one.

## Review checklist

Ownership clear? Bounds checked? Thread affinity stated? Failure path tested? Warning-free under `/W4` and `-Wall -Wextra -Wpedantic -Wshadow -Werror`? Formatting applied?
