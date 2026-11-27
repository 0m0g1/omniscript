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
#
# Other languages (langs\ folder, or the bench folder itself if there is no langs\): node, go, rust, java, dotnet (python is opt-in: it takes minutes to hours at full size).
# Each tool that is installed is built (release/-O3 settings) and run with the same benchmark names and sizes as bench.c;
# its output must match the C reference's output (floating-point lines within 5e-9) or the run is marked WRONG.
# Times are whole-process wall time (best of -Reps), so JVM / .NET / Node startup is included, as it is for the OmniScript JIT.
#   powershell -ExecutionPolicy Bypass -File .\bench\run_bench.ps1 -Langs node,go,rust -SkipAot
#   powershell -ExecutionPolicy Bypass -File .\bench\run_bench.ps1 -Langs python -Only fib -Reps 1 -TimeoutSec 900
#   -SkipOs skips the OmniScript runs entirely (useful for only measuring the other languages).
# Results are saved as bench_results_<machine>.csv and bench_langs_<machine>.csv so several devices can be compared.
param(
    [string]$Engine = ".\bin\Release-windows-x86_64\Osengine.exe",
    [int]$Reps = 3,
    [string[]]$Only = @(),
    [switch]$SkipAot,
    [switch]$SkipC,
    [switch]$SkipOs,
    [switch]$SkipLangs,
    [string[]]$Langs = @("node", "go", "rust", "java", "dotnet"),
    [int]$TimeoutSec = 300
)

# `powershell -File` hands "a,b" over as one string, so split list parameters on commas
$Only  = @($Only  | ForEach-Object { $_ -split "," } | Where-Object { $_ })
$Langs = @($Langs | ForEach-Object { $_ -split "," } | Where-Object { $_ })
$here  = Split-Path -Parent $MyInvocation.MyCommand.Path
$names = @("fib", "sieve", "nbody", "spectral", "fannkuch", "trees")
if ($Only.Count -gt 0) { $names = $names | Where-Object { $Only -contains $_ } }

if (-not $SkipOs -and -not (Test-Path $Engine)) { Write-Host "Engine not found: $Engine (use -Engine, or -SkipOs)" -ForegroundColor Red; exit 1 }

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


# ---- process helper with timeout (used for the other languages) -------------
function Quote-Arg([string]$a) { if ($a -match '[\s"]') { return '"' + ($a -replace '"', '\"') + '"' } return $a }

function Invoke-Proc([string]$exe, [string[]]$argList, [int]$timeoutSec) {
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $exe
    $psi.Arguments = (($argList | ForEach-Object { Quote-Arg $_ }) -join " ")
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $p = [System.Diagnostics.Process]::Start($psi)
    $outTask = $p.StandardOutput.ReadToEndAsync()
    $errTask = $p.StandardError.ReadToEndAsync()
    $done = $p.WaitForExit($timeoutSec * 1000)
    if (-not $done) {
        try { $p.Kill() } catch {}
        $sw.Stop()
        return [pscustomobject]@{ Ms = $sw.Elapsed.TotalMilliseconds; Out = ""; Err = ""; Code = $null; TimedOut = $true }
    }
    $p.WaitForExit()
    $sw.Stop()
    return [pscustomobject]@{ Ms = $sw.Elapsed.TotalMilliseconds; Out = $outTask.Result; Err = $errTask.Result; Code = $p.ExitCode; TimedOut = $false }
}

function Normalize-Lines([string]$text) {
    return @($text -split "`r?`n" | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" })
}

# same output as the C reference? numeric lines may differ by 5e-9 (last printed digit / FMA differences)
function Same-Output([string]$got, [string]$expected) {
    $g = Normalize-Lines $got; $e = Normalize-Lines $expected
    if ($g.Count -ne $e.Count -or $g.Count -eq 0) { return $false }
    for ($i = 0; $i -lt $g.Count; $i++) {
        if ($g[$i] -eq $e[$i]) { continue }
        $a = 0.0; $b = 0.0
        $ci = [System.Globalization.CultureInfo]::InvariantCulture
        if ([double]::TryParse($g[$i], [System.Globalization.NumberStyles]::Float, $ci, [ref]$a) -and
            [double]::TryParse($e[$i], [System.Globalization.NumberStyles]::Float, $ci, [ref]$b) -and
            [math]::Abs($a - $b) -le 5e-9) { continue }
        return $false
    }
    return $true
}

# Builds one language (if its toolchain is installed). Returns @{ Exe; Pre; Version } or $null.
function Prepare-Lang([string]$lang, [string]$dir) {
    function Tool([string[]]$names) { foreach ($n in $names) { $c = Get-Command $n -ErrorAction SilentlyContinue; if ($c) { return $c.Source } } return $null }
    function First-Line($text) { return (($text | Out-String) -split "`r?`n" | Where-Object { $_.Trim() -ne "" } | Select-Object -First 1) }
    switch ($lang) {
        "node" {
            $t = Tool @("node"); if (-not $t) { return $null }
            return @{ Exe = $t; Pre = @((Join-Path $dir "bench.js")); Version = "node " + (First-Line (& $t --version)) }
        }
        "python" {
            $t = Tool @("python", "python3", "py"); if (-not $t) { return $null }
            return @{ Exe = $t; Pre = @((Join-Path $dir "bench.py")); Version = First-Line (& $t --version 2>&1) }
        }
        "go" {
            $t = Tool @("go"); if (-not $t) { return $null }
            $exe = Join-Path $dir "bench_go.exe"
            & $t build -o $exe (Join-Path $dir "bench.go") 2>&1 | Out-Host
            if ($LASTEXITCODE -ne 0 -or -not (Test-Path $exe)) { Write-Host "  go build failed" -ForegroundColor Yellow; return $null }
            return @{ Exe = $exe; Pre = @(); Version = First-Line (& $t version) }
        }
        "rust" {
            $t = Tool @("rustc"); if (-not $t) { return $null }
            $exe = Join-Path $dir "bench_rs.exe"
            & $t -C opt-level=3 -C target-cpu=native -C codegen-units=1 -o $exe (Join-Path $dir "bench.rs") 2>&1 | Out-Host
            if ($LASTEXITCODE -ne 0 -or -not (Test-Path $exe)) { Write-Host "  rustc failed" -ForegroundColor Yellow; return $null }
            return @{ Exe = $exe; Pre = @(); Version = First-Line (& $t --version) }
        }
        "java" {
            $javac = Tool @("javac"); $java = Tool @("java"); if (-not $javac -or -not $java) { return $null }
            $out = Join-Path $dir "java_out"
            & $javac -d $out (Join-Path $dir "Bench.java") 2>&1 | Out-Host
            if ($LASTEXITCODE -ne 0) { Write-Host "  javac failed" -ForegroundColor Yellow; return $null }
            return @{ Exe = $java; Pre = @("-cp", $out, "Bench"); Version = "java " + (First-Line (& $javac -version 2>&1)) }
        }
        "dotnet" {
            $t = Tool @("dotnet"); if (-not $t) { return $null }
            $ver = First-Line (& $t --version)
            $major = ($ver -split "\.")[0]
            $proj = Join-Path $dir "dotnet\bench.csproj"
            $out = Join-Path $dir "dotnet_out"
            & $t build $proj -c Release -o $out "-p:TargetFramework=net$major.0" -nologo -v q 2>&1 | Out-Host
            $dll = Join-Path $out "bench_dotnet.dll"
            if ($LASTEXITCODE -ne 0 -or -not (Test-Path $dll)) { Write-Host "  dotnet build failed" -ForegroundColor Yellow; return $null }
            return @{ Exe = $t; Pre = @($dll); Version = ".NET SDK $ver" }
        }
    }
    return $null
}

# ---- machine info -----------------------------------------------------------
$machine = $env:COMPUTERNAME; if (-not $machine) { $machine = [System.Net.Dns]::GetHostName() }
try { $cpu = (Get-CimInstance Win32_Processor | Select-Object -First 1).Name.Trim() } catch { $cpu = "unknown CPU" }
Write-Host "Machine: $machine  |  $cpu" -ForegroundColor DarkGray

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

# ---- other languages: build what is installed --------------------------------
$langCmd = [ordered]@{}
if (-not $SkipLangs) {
    # Language sources live either in bench\langs or directly in bench\ (bench.js, bench.go, ...).
    $langDir = Join-Path $here "langs"
    if (-not (Test-Path $langDir)) { $langDir = $here }
    foreach ($l in $Langs) {
        $l = $l.ToLower()
        if (-not (Test-Path $langDir)) { Write-Host "No langs folder at $langDir; skipping other languages." -ForegroundColor Yellow; break }
        $cmd = Prepare-Lang $l $langDir
        if ($cmd) { $langCmd[$l] = $cmd; Write-Host "$l ready: $($cmd.Version)" }
        else { Write-Host "$l skipped (toolchain missing or build failed)" -ForegroundColor Yellow }
    }
}
$cmpRows = @()

$rows = @()
foreach ($n in $names) {
    $src = Join-Path $here "bench_$n.os"
    Write-Host "== $n" -ForegroundColor Cyan
    $row = [ordered]@{ Bench = $n; C_ms = $null; Compile_ms = $null; JIT_total_ms = $null; JIT_run_ms = $null; AOT_ms = $null; JIT_vs_C = $null; AOT_vs_C = $null; Notes = "" }

    if ($cExe) {
        $c = Best-Of $cExe @($n) $Reps $false
        if ($c.Ms) { $row.C_ms = [math]::Round($c.Ms) } else { $row.Notes += "C:$($c.Status) " }
    }

    $cExpected = $null
    if ($cExe -and $c -and $c.Ms) { $cExpected = $c.Out }

    if (-not $SkipOs) {
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

    } # end -SkipOs guard

    if ($row.C_ms -and $row.JIT_run_ms) { $row.JIT_vs_C = [math]::Round($row.JIT_run_ms / $row.C_ms, 2) }
    if ($row.C_ms -and $row.AOT_ms)     { $row.AOT_vs_C = [math]::Round($row.AOT_ms / $row.C_ms, 2) }
    $rows += [pscustomobject]$row

    # ---- other languages (same benchmark name + default size, output checked against the C reference) ----
    $cmp = [ordered]@{ Bench = $n; C = $row.C_ms; OS_JIT = $row.JIT_run_ms; OS_AOT = $row.AOT_ms }
    foreach ($l in $langCmd.Keys) {
        $lc = $langCmd[$l]
        $bestMs = $null; $status = "ok"
        for ($i = 0; $i -lt $Reps; $i++) {
            $r = Invoke-Proc $lc.Exe (@($lc.Pre) + @($n)) $TimeoutSec
            if ($r.TimedOut) { $status = "TIMEOUT>$($TimeoutSec)s"; break }
            if ($r.Code -ne 0) { $status = "ERR($($r.Code))"; Write-Host "   $l problem: $((($r.Err + " " + $r.Out) -split "`r?`n" | Select-Object -First 3) -join ' | ')" -ForegroundColor Yellow; break }
            if ($cExpected -and -not (Same-Output $r.Out $cExpected)) { $status = "WRONG"; Write-Host "   $l output differs from C: $(($r.Out -split "`r?`n" | Select-Object -First 3) -join ' | ')" -ForegroundColor Yellow; break }
            if ($null -eq $bestMs -or $r.Ms -lt $bestMs) { $bestMs = $r.Ms }
        }
        if ($status -eq "ok" -and $null -ne $bestMs) {
            $cmp[$l] = [math]::Round($bestMs)
            $tag = if ($cExpected) { "ok" } else { "unchecked (no C reference)" }
            Write-Host "   $l`: $([math]::Round($bestMs)) ms ($tag)"
        } else { $cmp[$l] = $status }
    }
    $cmpRows += [pscustomobject]$cmp
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
$csv = Join-Path $here "bench_results_$machine.csv"
$rows | Select-Object @{n="Machine";e={$machine}}, * | Export-Csv -NoTypeInformation -Path $csv
Write-Host "Saved $csv"

# ---- cross-language comparison -------------------------------------------------
if ($cmpRows.Count -gt 0) {
    $cols = @($cmpRows[0].PSObject.Properties.Name | Where-Object { $_ -ne "Bench" -and $_ -ne "C" })
    Write-Host ""
    Write-Host "Time in ms (best of $Reps, whole process).  C = gcc/clang -O3 -march=native reference" -ForegroundColor Cyan
    $cmpRows | Format-Table -AutoSize

    # ratio to C: numbers only; text (WRONG / TIMEOUT / ERR) is kept so failures stay visible
    $ratioRows = @(); $ratioVals = @{}
    foreach ($c in $cols) { $ratioVals[$c] = @() }
    foreach ($r in $cmpRows) {
        $o = [ordered]@{ Bench = $r.Bench }
        foreach ($c in $cols) {
            $v = $r.$c
            if ($v -is [string] -and $v -ne "") { $o[$c] = $v }
            elseif ($v -and $r.C) { $x = [math]::Round($v / $r.C, 2); $o[$c] = $x; $ratioVals[$c] += $x }
            else { $o[$c] = $null }
        }
        $ratioRows += [pscustomobject]$o
    }
    $g = [ordered]@{ Bench = "GEOMEAN" }
    foreach ($c in $cols) { $gm = Geo-Mean $ratioVals[$c]; $g[$c] = if ($gm) { "$($gm)x" } else { $null } }
    $ratioRows += [pscustomobject]$g
    Write-Host "Slowdown vs C (1.00 = same speed as C; GEOMEAN is over the benchmarks that passed for that language)" -ForegroundColor Cyan
    $ratioRows | Format-Table -AutoSize

    $csv2 = Join-Path $here "bench_langs_$machine.csv"
    $cmpRows | Select-Object @{n="Machine";e={$machine}}, * | Export-Csv -NoTypeInformation -Path $csv2
    Write-Host "Saved $csv2"
}