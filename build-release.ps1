$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$solution = Join-Path $root "build_x64\mouse-overlay-gg.sln"
$msbuild = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
$isccCandidates = @(
    "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
    "C:\Program Files\Inno Setup 6\ISCC.exe",
    "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
)
$iscc = $isccCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1

if (!(Test-Path -LiteralPath $msbuild)) {
    throw "MSBuild not found: $msbuild"
}
if (!$iscc) {
	throw "Inno Setup is not installed"
}

& $msbuild $solution -t:Build -p:Configuration=RelWithDebInfo -m -v:minimal
if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE"
}

$tests = @(
    "build_x64\tests\RelWithDebInfo\test_keyboard_layout.exe",
    "build_x64\tests\RelWithDebInfo\test_keyboard_keys.exe",
    "build_x64\tests\RelWithDebInfo\test_mouse_click.exe",
    "build_x64\tests\RelWithDebInfo\test_mouse_trail.exe"
)
$obsTestBin = "C:\Program Files\obs-studio\bin\64bit"
if (!(Test-Path -LiteralPath $obsTestBin)) {
    throw "OBS test binary directory not found: $obsTestBin"
}
$env:Path = "$obsTestBin;$env:Path"
foreach ($test in $tests) {
    $testPath = Join-Path $root $test
    if (!(Test-Path -LiteralPath $testPath)) {
        throw "Test executable not found: $testPath"
    }
    & $testPath
    if ($LASTEXITCODE -ne 0) {
        throw "Test failed: $test"
    }
}

$installer = Join-Path $root "installer.iss"
& $iscc $installer
if ($LASTEXITCODE -ne 0) {
    throw "Installer build failed with exit code $LASTEXITCODE"
}

Write-Host "Installer created: $(Join-Path $root 'release\Input_overlay_gg_v1.1.0_Setup.exe')"
