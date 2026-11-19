param([string]$BuildDir = (Join-Path $PSScriptRoot '..' 'build'))
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
cmake -S $root -B $BuildDir -G Ninja -DCMAKE_BUILD_TYPE=Release -DAXF_WERROR=ON
cmake --build $BuildDir --parallel
ctest --test-dir $BuildDir --output-on-failure
