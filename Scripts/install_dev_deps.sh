#!/usr/bin/env bash
# Install the extra local toolchain used by Scripts/dev_all.sh on
# Debian/Ubuntu. AXF itself needs only CMake, Ninja and a C++20 compiler;
# wine + MinGW enable the local Windows-path test pass.
set -euo pipefail

if ! command -v apt-get >/dev/null 2>&1; then
    echo "install_dev_deps.sh supports Debian/Ubuntu apt only" >&2
    exit 1
fi

export DEBIAN_FRONTEND=noninteractive
sudo dpkg --add-architecture i386
sudo apt-get update -qq
sudo apt-get install -y -qq wine64 wine32 mingw-w64
echo "Development dependencies installed. Run Scripts/dev_all.sh."
