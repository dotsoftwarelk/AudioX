#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODE="${1:-address}"

case "$MODE" in
  address)
    BUILD="${AXF_SANITIZER_BUILD_DIR:-$ROOT/build-asan}"
    SANITIZERS="address,undefined"
    ;;
  thread)
    BUILD="${AXF_SANITIZER_BUILD_DIR:-$ROOT/build-tsan}"
    SANITIZERS="thread"
    ;;
  *)
    echo "usage: run_sanitizers.sh [address|thread]" >&2
    exit 2
    ;;
esac

export PATH="$HOME/.local/bin:$PATH"
FLAGS="-fsanitize=${SANITIZERS} -fno-omit-frame-pointer"
cmake -S "$ROOT" -B "$BUILD" -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DAXF_WERROR=ON \
  -DCMAKE_CXX_FLAGS="$FLAGS" \
  -DCMAKE_EXE_LINKER_FLAGS="$FLAGS"
cmake --build "$BUILD" --parallel 2

if [ "$MODE" = "address" ]; then
  export ASAN_OPTIONS="detect_leaks=1:halt_on_error=1"
  export UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"
else
  export TSAN_OPTIONS="halt_on_error=1"
fi
ctest --test-dir "$BUILD" --output-on-failure
