param([string]$BuildDir = (Join-Path $PSScriptRoot '..' 'build'))
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
cmake -S $root -B $BuildDir -G 'Visual Studio 17 2022' -A x64 -DAXF_WERROR=ON
cmake --build $BuildDir --config Release --parallel
ctest --test-dir $BuildDir -C Release --output-on-failure
