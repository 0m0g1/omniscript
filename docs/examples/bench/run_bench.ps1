# docs/examples/bench/run_bench.ps1
# OmniScript benchmark runner.
# Run from the PROJECT ROOT (relative paths in OmniScript resolve from the launch dir):
#   powershell -ExecutionPolicy Bypass -File .\bench\run_bench.ps1
#   powershell -ExecutionPolicy Bypass -File .\bench\run_bench.ps1 -Only fib,nbody -Reps 5
#   powershell -ExecutionPolicy Bypass -File .\bench\run_bench.ps1 -Engine .\bin\Debug-windows-x86_64\Osengine.exe
# For each benchmark it reports the best of -Reps runs for:
#   C        gcc/clang -O3 -march=native reference (skipped if no compiler is on PATH)
#   JIT      the whole `os file.os` run, i.e. compile + run
#   compile  `os file.os --dry` (compile only); JIT-run = JIT - compile, an estimate of pure run time
#   AOT      `os file.os --make -o x.exe`, then the produced exe (run time only)
# A run only counts if the program printed PASS (its own correctness check against known values).
param(
    [string]$Engine = ".\bin\Release-windows-x86_64\Osengine.exe",
    [int]$Reps = 3,
    [string[]]$Only = @(),
    [switch]$SkipAot,
    [switch]$SkipC
)

$here  = Split-Path -Parent $MyInvocation.MyCommand.Path
$names = @("fib", "sieve", "nbody", "spectral", "fannkuch", "trees")
if ($Only.Count -gt 0) { $names = $names | Where-Object { $Only -contains $_ } }

if (-not (Test-Path $Engine)) { Write-Host "Engine not found: $Engine (use -Engine)" -ForegroundColor Red; exit 1 }

function Invoke-Timed([string]$exe, [string[]]$argList) {
    $sw  = [System.Diagnostics.Stopwatch]::StartNew()
    $out = (& $exe @argList 2>&1 | Out-String)
    $sw.Stop()
    return [pscustomobject]@{ Ms = $sw.Elapsed.TotalMilliseconds; Out = $out; Code = $LASTEXITCODE }
}

function Best-Of([string]$exe, [string[]]$argList, [int]$reps, [bool]$needPass) {
    $best = $null; $lastOut = ""
    for ($i = 0; $i -lt $reps; $i++) {
        $r = Invoke-Timed $exe $argList
        $lastOut = $r.Out
        if ($r.Code -ne 0) { return [pscustomobject]@{ Ms = $null; Out = $r.Out; Status = "ERR($($r.Code))" } }
        if ($needPass -and $r.Out -notmatch "PASS") { return [pscustomobject]@{ Ms = $null; Out = $r.Out; Status = "WRONG" } }
        if ($null -eq $best -or $r.Ms -lt $best) { $best = $r.Ms }
    }
    return [pscustomobject]@{ Ms = $best; Out = $lastOut; Status = "ok" }
}

# ---- C reference build ------------------------------------------------------
$cExe = $null
if (-not $SkipC) {
    $cc = Get-Command gcc -ErrorAction SilentlyContinue
    if (-not $cc) { $cc = Get-Command clang -ErrorAction SilentlyContinue }
    if ($cc) {
        $cExe = Join-Path $here "bench_c.exe"
        & $cc.Source -O3 -march=native (Join-Path $here "bench.c") -o $cExe -lm 2>$null
        if ($LASTEXITCODE -ne 0) { & $cc.Source -O3 -march=native (Join-Path $here "bench.c") -o $cExe 2>$null }
        if ($LASTEXITCODE -ne 0) { Write-Host "C reference failed to build; skipping C column." -ForegroundColor Yellow; $cExe = $null }
        else { Write-Host "C reference built with $($cc.Name)" }
    } else { Write-Host "No gcc/clang on PATH: skipping C column (ratios will be blank)." -ForegroundColor Yellow }
}

$rows = @()
foreach ($n in $names) {
    $src = Join-Path $here "bench_$n.os"
    Write-Host "== $n" -ForegroundColor Cyan
    $row = [ordered]@{ Bench = $n; C_ms = $null; Compile_ms = $null; JIT_total_ms = $null; JIT_run_ms = $null; AOT_ms = $null; JIT_vs_C = $null; AOT_vs_C = $null; Notes = "" }

    if ($cExe) {
        $c = Best-Of $cExe @($n) $Reps $false
        if ($c.Ms) { $row.C_ms = [math]::Round($c.Ms) } else { $row.Notes += "C:$($c.Status) " }
    }

    # compile only (best effort: if --dry is rejected the column stays empty)
    $d = Invoke-Timed $Engine @($src, "--dry")
    if ($d.Code -eq 0) { $row.Compile_ms = [math]::Round($d.Ms) }

    $j = Best-Of $Engine @($src) $Reps $true
    if ($j.Ms) {
        $row.JIT_total_ms = [math]::Round($j.Ms)
        if ($row.Compile_ms) { $row.JIT_run_ms = [math]::Round([math]::Max($j.Ms - $row.Compile_ms, 1)) }
        $first = ($j.Out -split "`r?`n" | Where-Object { $_ -match "PASS|FAIL|N/A" } | Select-Object -First 1)
        $nums = ($j.Out -split "`r?`n" | Where-Object { $_ -match "^\s*-?\d+(\.\d+)?\s*$" }) -join " "
        if ($first) { Write-Host "   JIT: $first $nums" }
    } else {
        $row.Notes += "JIT:$($j.Status) "
        Write-Host "   JIT problem ($($j.Status)); first lines of output:" -ForegroundColor Yellow
        ($j.Out -split "`r?`n" | Select-Object -First 12) | ForEach-Object { Write-Host "     $_" }
    }

    if (-not $SkipAot) {
        $exe = Join-Path $here "bench_$n.exe"
        if (Test-Path $exe) { Remove-Item $exe -Force }
        $b = Invoke-Timed $Engine @($src, "--make", "-o", $exe)
        if ($b.Code -eq 0 -and (Test-Path $exe)) {
            $a = Best-Of $exe @() $Reps $true
            if ($a.Ms) { $row.AOT_ms = [math]::Round($a.Ms) } else { $row.Notes += "AOT:$($a.Status) " }
        } else {
            $row.Notes += "AOT:build-failed "
            Write-Host "   AOT build failed (exit $($b.Code)); first lines of output:" -ForegroundColor Yellow
            ($b.Out -split "`r?`n" | Select-Object -First 12) | ForEach-Object { Write-Host "     $_" }
        }
    }

    if ($row.C_ms -and $row.JIT_run_ms) { $row.JIT_vs_C = [math]::Round($row.JIT_run_ms / $row.C_ms, 2) }
    if ($row.C_ms -and $row.AOT_ms)     { $row.AOT_vs_C = [math]::Round($row.AOT_ms / $row.C_ms, 2) }
    $rows += [pscustomobject]$row
}

Write-Host ""
$rows | Format-Table -AutoSize

function Geo-Mean($vals) {
    $v = @($vals | Where-Object { $_ })
    if ($v.Count -eq 0) { return $null }
    $s = 0.0; foreach ($x in $v) { $s += [math]::Log($x) }
    return [math]::Round([math]::Exp($s / $v.Count), 2)
}
$gj = Geo-Mean ($rows | ForEach-Object { $_.JIT_vs_C })
$ga = Geo-Mean ($rows | ForEach-Object { $_.AOT_vs_C })
if ($gj) { Write-Host "Geometric mean slowdown vs C  (JIT run): ${gj}x" }
if ($ga) { Write-Host "Geometric mean slowdown vs C  (AOT):     ${ga}x" }
$csv = Join-Path $here "bench_results.csv"
$rows | Export-Csv -NoTypeInformation -Path $csv
Write-Host "Saved $csv"
