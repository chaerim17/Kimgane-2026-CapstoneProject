$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ tools not found' }
$dev = Join-Path $vs 'Common7\Tools\VsDevCmd.bat'
$build = Join-Path $repo 'bin\LunarMapTests'
New-Item -ItemType Directory -Force -Path $build | Out-Null
$sources = @('Tools\LunarMapSmokeTest.cpp','Shared\Terrain\TerrainHeightMap.cpp',
 'Server\KimganeServer\src\Terrain\TerrainHeightMap.cpp','Shared\IO\AssetPathResolver.cpp',
 'Shared\Physics\CollisionWorld.cpp','Shared\Physics\CollisionQueries.cpp',
 'Shared\Physics\CollisionResolver.cpp','Shared\Physics\CharacterMovement.cpp')
$quoted = ($sources | ForEach-Object { '"' + (Join-Path $repo $_) + '"' }) -join ' '
Push-Location $build
try {
    $batch = Join-Path $build 'compile.cmd'
    $lines = @('@echo off', "call `"$dev`" -arch=x64 >nul", 'if errorlevel 1 exit /b 1')
    $objects = @()
    for ($i=0; $i -lt $sources.Count; $i++) {
        $source = Join-Path $repo $sources[$i]
        $objects += "part$i.obj"
        $lines += "cl /nologo /c /std:c++20 /EHsc /utf-8 /DNOMINMAX /DWIN32_LEAN_AND_MEAN `"$source`" /Fo:part$i.obj"
        $lines += 'if errorlevel 1 exit /b 1'
    }
    $lines += "link /nologo $($objects -join ' ') /out:LunarMapSmokeTest.exe"
    $lines += 'exit /b %errorlevel%'
    $lines | Set-Content -LiteralPath $batch -Encoding ascii
    & cmd.exe /d /c $batch
    if ($LASTEXITCODE -ne 0) { throw 'Lunar smoke test compilation failed' }
    Set-Location $repo
    & (Join-Path $build 'LunarMapSmokeTest.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Lunar smoke test failed' }
} finally { Pop-Location }
