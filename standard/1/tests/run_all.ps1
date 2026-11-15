# Runs every test_*.os in this folder with the Omniscript engine.
# Works from any location: finds the repo root by walking up until the engine exe is found.
param([string]$Config = "Debug-windows-x86_64")

$root = $PSScriptRoot
while ($root -and -not (Test-Path (Join-Path $root "bin\$Config\Osengine.exe"))) {
    $parent = Split-Path $root -Parent
    if ($parent -eq $root) { $root = $null; break }
    $root = $parent
}
if (-not $root) {
    Write-Host "Could not find bin\$Config\Osengine.exe above $PSScriptRoot" -ForegroundColor Red
    exit 2
}

$omni = Join-Path $root "bin\$Config\Osengine.exe"
Set-Location $root   # the engine resolves "standard/1/..." imports from the repo root

$failed = 0
Get-ChildItem -Path $PSScriptRoot -Filter "test_*.os" | Sort-Object Name | ForEach-Object {
    Write-Host "=== $($_.Name) ==="
    $rel = Resolve-Path -Relative $_.FullName
    & $omni $rel
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FAILED: $($_.Name)" -ForegroundColor Red
        $failed++
    }
}

if ($failed -eq 0) { Write-Host "All tests passed" -ForegroundColor Green }
else { Write-Host "$failed file(s) failed" -ForegroundColor Red }
exit $failed