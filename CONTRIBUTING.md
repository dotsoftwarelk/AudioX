# Contributing to AudioX

## Branches and commits

- `main` is stable. Work on `feature/<short-name>` branches.
- Keep commits small and focused. Every commit must build and pass tests.
- Open a pull request; CI (MSVC Debug/Release, portable GCC, quality gates) must pass before merge.
- Never force-push shared branches. Never commit credentials, SDK sources or private keys.

## Code style

See `Documentation/coding-standards.md`. Format with clang-format 18 before committing:

```bash
clang-format -i $(git ls-files '*.hpp' '*.cpp')
```

## Definition of done for a module

Documented API, implemented, builds on supported configurations, unit tests pass, relevant integration tests pass, errors handled, ownership explicit, thread-safety documented, performance measured where relevant, limitations recorded.

## Testing

```bash
Scripts/run_tests.sh          # portable GCC/Clang
./Scripts/run_tests.ps1       # Windows MSVC
```

Hardware-dependent tests are tagged/disabled by default and must be run on a physical Windows machine with a real audio device. Cloud CI does not prove device behaviour.
