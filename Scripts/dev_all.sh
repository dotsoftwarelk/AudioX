#!/usr/bin/env bash
# One-command AXF development workflow: build, install, consume, test and
# verify as many paths as the host can run. Optional integrations are skipped
# with a printed reason instead of being silently marked as passed.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PATH="$HOME/.local/bin:$PATH"
DEV_ROOT="${AXF_DEV_ROOT:-$HOME/.cache/audiox-dev}"
mkdir -p "$DEV_ROOT/build" "$DEV_ROOT/install" "$DEV_ROOT/wine"

MODE="${1:-full}"
case "$MODE" in
  full) ;;
  quick|native|consumer|vst3|sanitizers|windows) ;;
  *)
    echo "usage: $0 [full|quick|native|consumer|vst3|sanitizers|windows]" >&2
    exit 2
    ;;
esac

declare -A RESULTS
record() { RESULTS["$1"]="$2"; }
run_step() {
    local name="$1"; shift
    echo
    echo "=== $name ==="
    local status=0
    ( set -e; "$@" ) || status=$?
    if [ "$status" -eq 0 ]; then
        record "$name" PASS
    else
        record "$name" FAIL
    fi
    return 0
}

native_build() {
    local build="$DEV_ROOT/build/native-release"
    cmake -S "$ROOT" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DAXF_WERROR=ON
    cmake --build "$build" --parallel 4
    ctest --test-dir "$build" --output-on-failure
}

native_consumer() {
    local build="$DEV_ROOT/build/native-release"
    local prefix="$DEV_ROOT/install/native"
    local consumer="$DEV_ROOT/build/native-consumer"
    cmake --install "$build" --prefix "$prefix"
    rm -rf "$consumer"
    cmake -S "$ROOT/Examples/minimal_app" -B "$consumer" -G Ninja \
        -DCMAKE_PREFIX_PATH="$prefix" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$consumer" --parallel 4
    "$consumer/axf_minimal_app"
}

vst3_build() {
    local sdk="${AXF_VST3_SDK_ROOT:-}"
    if [ -z "$sdk" ]; then
        echo "SKIP vst3: set AXF_VST3_SDK_ROOT to enable the SDK-backed build"
        record vst3 SKIP
        return 0
    fi
    local build="$DEV_ROOT/build/vst3"
    cmake -S "$ROOT" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DAXF_WERROR=ON -DAXF_ENABLE_VST3=ON -DAXF_VST3_SDK_ROOT="$sdk"
    cmake --build "$build" --parallel 4
    ctest --test-dir "$build" --output-on-failure
    if [ -n "${AXF_VST3_PLUGIN_PATH:-}" ]; then
        export AXF_VST3_PLUGIN_PATH
        if [ -n "${AXF_VST3_EXPECT_GAIN:-}" ]; then
            export AXF_VST3_EXPECT_GAIN
        else
            unset AXF_VST3_EXPECT_GAIN
        fi
        "$build/Tests/axf_platform_tests" \
            --gtest_filter=PluginHost.Vst3RealPluginSmoke
    else
        echo "SKIP vst3-plugin-smoke: set AXF_VST3_PLUGIN_PATH for a real .vst3 binary"
        record vst3-plugin-smoke SKIP
    fi
}

sanitizer_builds() {
    AXF_SANITIZER_BUILD_DIR="$DEV_ROOT/build/asan" "$ROOT/Scripts/run_sanitizers.sh" address
    AXF_SANITIZER_BUILD_DIR="$DEV_ROOT/build/tsan" "$ROOT/Scripts/run_sanitizers.sh" thread
}

windows_wine_build() {
    if ! command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1 || ! command -v wine >/dev/null 2>&1; then
        echo "SKIP windows-wine: install wine and mingw-w64 (Scripts/install_dev_deps.sh)"
        record windows-wine SKIP
        return 0
    fi
    export WINEPREFIX="$DEV_ROOT/wine/prefix"
    export XDG_RUNTIME_DIR="$DEV_ROOT/wine/runtime"
    export WINEDEBUG=-all
    mkdir -p "$WINEPREFIX" "$XDG_RUNTIME_DIR"
    chmod 700 "$XDG_RUNTIME_DIR"
    if [ ! -f "$WINEPREFIX/.update-timestamp" ]; then
        wineboot -u
    fi

    local build="$DEV_ROOT/build/mingw-tests"
    local prefix="$DEV_ROOT/install/mingw"
    local consumer="$DEV_ROOT/build/mingw-consumer"
    local toolchain="$ROOT/cmake/mingw-w64-toolchain.cmake"
    local linkflags='-static-libgcc -static-libstdc++ -static'
    local jobs="${MINGW_JOBS:-2}"
    cmake -S "$ROOT" -B "$build" -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$toolchain" -DCMAKE_BUILD_TYPE=Debug \
        -DAXF_WERROR=ON -DAXF_BUILD_APP=OFF \
        -DCMAKE_CROSSCOMPILING_EMULATOR=wine \
        -DCMAKE_EXE_LINKER_FLAGS="$linkflags"
    cmake --build "$build" --parallel "$jobs"
    ctest --test-dir "$build" --output-on-failure

    cmake --install "$build" --prefix "$prefix"
    rm -rf "$consumer"
    cmake -S "$ROOT/Examples/minimal_app" -B "$consumer" -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$toolchain" \
        -DAXF_DIR="$prefix/lib/cmake/AXF" -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_EXE_LINKER_FLAGS="$linkflags"
    cmake --build "$consumer" --parallel "$jobs"
    wine "$consumer/axf_minimal_app.exe"
}

case "$MODE" in
  quick)
    run_step native native_build
    ;;
  native)
    run_step native native_build
    run_step native-consumer native_consumer
    ;;
  consumer)
    run_step native-consumer native_consumer
    ;;
  vst3)
    run_step vst3 vst3_build
    ;;
  sanitizers)
    run_step sanitizers sanitizer_builds
    ;;
  windows)
    run_step windows-wine windows_wine_build
    ;;
  full)
    run_step dependency-policy python3 "$ROOT/Scripts/check_dependencies.py"
    run_step native native_build
    run_step native-consumer native_consumer
    run_step vst3 vst3_build
    run_step sanitizers sanitizer_builds
    run_step windows-wine windows_wine_build
    ;;
esac

echo
echo "=== Summary ==="
failed=0
for name in $(printf '%s\n' "${!RESULTS[@]}" | sort); do
    printf '%-24s %s\n' "$name" "${RESULTS[$name]}"
    [ "${RESULTS[$name]}" = FAIL ] && failed=1
done
exit "$failed"
