<#
.SYNOPSIS
    Test suite for the OmniScript grep example (docs\examples\grep\main.os).

.DESCRIPTION
    Builds a set of fixture files in a "fixtures" folder next to this script, runs the grep
    example through Osengine.exe once per test, and compares stdout (and optionally the exit
    code / stderr) with what GNU grep prints for the same command line.

    Test tiers
      Core     options listed in the grep usage text (-e -i -v -n -c -l -r -R -E -F -H -h --color --help)
      Edge     behaviour real grep has that the usage text does not mention (empty pattern, "-" as stdin,
               option order, error handling, regex corner cases, big/long/UTF-8 files ...)
      Exit     exit status: 0 = match, 1 = no match, 2 = error
      Missing  grep features the example does not implement yet (-w -x -o -q -m -A/-B/-C -L -s, several -e,
               -f, --include/--exclude, long option names, \b \w \s back-references).
               A failing Missing test is reported as TODO and does not fail the run (use -Strict to change that).
               A Missing test that passes is reported as NEW.

    Lives in docs\examples\grep\ next to main.os. Run it from the repository root (the folder you
    normally start Osengine.exe from), because Osengine.exe is looked up relative to the current folder.

.EXAMPLE
    .\docs\examples\grep\run-tests.ps1
.EXAMPLE
    .\docs\examples\grep\run-tests.ps1 -Tier Core,Edge
.EXAMPLE
    .\docs\examples\grep\run-tests.ps1 -Filter '*-r*' -Verbose
.EXAMPLE
    .\docs\examples\grep\run-tests.ps1 -List
#>
[CmdletBinding()]
param(
    [string]$Engine = '.\bin\Release-windows-x86_64\Osengine.exe',
    [string]$Script = (Join-Path $PSScriptRoot 'main.os'),
    [ValidateSet('Core','Edge','Exit','Missing')][string[]]$Tier = @('Core','Edge','Exit','Missing'),
    [string]$Filter = '*',            # wildcard on the test name
    [int]$TimeoutSec = 30,            # per test; a hung test is killed and reported as FAIL
    [switch]$Strict,                  # count failing Missing tests as failures too
    [switch]$Quiet,                   # do not print details for TODO tests
    [switch]$List,                    # only list the test names
    [switch]$FixturesOnly             # only (re)create the fixtures folder
)

$ErrorActionPreference = 'Stop'
$EnginePath = (Resolve-Path -LiteralPath $Engine).Path
$ScriptPath = (Resolve-Path -LiteralPath $Script).Path
$WorkDir    = (Get-Location).Path
$FixDir     = Join-Path $PSScriptRoot 'fixtures'
$FixPrefixRx = '^' + [regex]::Escape($FixDir) + '[\\/]'
$BannerRx   = '^(Starting Omniscript engine execution\.\.\.|Engine execution completed successfully|Application completed successfully)$'

# ---------------------------------------------------------------- fixtures
function Add-Bytes([string]$Name, [byte[]]$Bytes) {
    $path = Join-Path $FixDir $Name
    $dir  = Split-Path -Parent $path
    if (-not (Test-Path -LiteralPath $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }
    [IO.File]::WriteAllBytes($path, $Bytes)
}
function Add-Text([string]$Name, [string[]]$Lines, [string]$Eol = "`n", [switch]$NoFinalEol) {
    $text = $Lines -join $Eol
    if (-not $NoFinalEol -and $Lines.Count -gt 0) { $text += $Eol }
    Add-Bytes $Name ((New-Object System.Text.UTF8Encoding($false)).GetBytes($text))   # UTF-8, no BOM
}
function New-Fixtures {
    if ((Split-Path -Leaf $FixDir) -ne 'fixtures') { throw "refusing to clean '$FixDir'" }
    if (Test-Path -LiteralPath $FixDir) { Remove-Item -LiteralPath $FixDir -Recurse -Force }
    New-Item -ItemType Directory -Path $FixDir -Force | Out-Null
    Add-Text 'a.txt' @('apple pie','Banana split','cherry tart','APPLE crumble','banana bread','date square','apple, apple, apple')
    Add-Text 'b.txt' @('alpha','beta','gamma','delta','apple orchard','epsilon')
    Add-Text 'words.txt' @('cat','cats','concat','the cat sat','cat-like','bobcat')
    Add-Text 'regex.txt' @('abc123','foo42bar','no digits here','2024-11-27','a.b','axb','test@example.com','  indented line','x','aaa','color','colour','ababab','abcd','abc')
    Add-Text 'special.txt' @('price is $5.00','a+b=c','(parens) here','[brackets] here','back\slash','star*star','question?','caret^top','pipe|char','brace{1}')
    Add-Text 'dash.txt' @('-n is an option','--verbose flag','plain line','a -- b','-')
    Add-Text 'blank.txt' @('a','','b','')
    Add-Text 'ctx.txt' @('L1','L2','L3','L4','L5','L6','L7','L8','L9','L10')
    Add-Text 'uni.txt' @('naïve café','plain','日本語 text')
    Add-Text 'pats.txt' @('cherry','date')
    Add-Text 'noeol.txt' @('first line','last line without newline') -NoFinalEol
    Add-Text 'crlf.txt' @('one','two','three') -Eol "`r`n"
    Add-Text 'empty.txt' @()
    Add-Bytes 'binary.bin' ([byte[]](97,112,112,108,101,0,98,105,110,97,114,121,10,97,112,112,108,101,32,97,103,97,105,110,10))
    Add-Bytes 'late-nul.bin' ([byte[]](@([Text.Encoding]::ASCII.GetBytes('apple start' + "`n" + (("filler line`n") * 800) + 'tail')) + @(0) + @([Text.Encoding]::ASCII.GetBytes("end`n"))))
    Add-Text 'long.txt' @((('x' * 100000) + 'needle'), 'short line')
    Add-Text 'big.txt' @(1..20000 | ForEach-Object { "line $_" })
    Add-Text 'tree/top.txt' @('needle top','hay')
    Add-Text 'tree/sub/deep.txt' @('needle deep')
    Add-Text 'tree/sub/more/deeper.txt' @('hay only','needle deeper')
    Add-Text 'tree/sub/note.md' @('needle md')
    Add-Bytes 'tree/sub/skip.bin' ([byte[]](110,101,101,100,108,101,0,98,105,110,10))
    New-Item -ItemType Directory -Path (Join-Path $FixDir 'tree\emptydir') -Force | Out-Null
}
New-Fixtures
if ($FixturesOnly) { Write-Host "Fixtures written to $FixDir"; return }

# ---------------------------------------------------------------- harness
$script:Results = New-Object System.Collections.ArrayList

function F([string]$Name) { Join-Path $FixDir $Name }          # absolute path of a fixture

function ConvertTo-WinArg([string]$a) {                          # CommandLineToArgvW-compatible quoting
    if ($a.Length -eq 0) { return '""' }
    if ($a -notmatch '[\s"]') { return $a }
    $sb = New-Object System.Text.StringBuilder
    [void]$sb.Append('"')
    $bs = 0
    foreach ($c in $a.ToCharArray()) {
        if ($c -eq '\') { $bs++ }
        elseif ($c -eq '"') { [void]$sb.Append('\' * ($bs * 2 + 1)); [void]$sb.Append('"'); $bs = 0 }
        else { if ($bs -gt 0) { [void]$sb.Append('\' * $bs); $bs = 0 }; [void]$sb.Append($c) }
    }
    if ($bs -gt 0) { [void]$sb.Append('\' * ($bs * 2)) }
    [void]$sb.Append('"')
    $sb.ToString()
}

function Invoke-Grep([string[]]$Cmd, [string]$Stdin) {
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $EnginePath
    $psi.Arguments = ((@($ScriptPath) + @($Cmd)) | ForEach-Object { ConvertTo-WinArg $_ }) -join ' '
    $psi.WorkingDirectory = $WorkDir
    $psi.UseShellExecute = $false
    $psi.CreateNoWindow = $true
    $psi.RedirectStandardInput = $true
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.StandardOutputEncoding = [System.Text.Encoding]::UTF8
    $psi.StandardErrorEncoding  = [System.Text.Encoding]::UTF8
    $p = [System.Diagnostics.Process]::Start($psi)
    $outTask = $p.StandardOutput.ReadToEndAsync()
    $errTask = $p.StandardError.ReadToEndAsync()
    if (-not [string]::IsNullOrEmpty($Stdin)) {
        $bytes = (New-Object System.Text.UTF8Encoding($false)).GetBytes($Stdin)
        try { $p.StandardInput.BaseStream.Write($bytes, 0, $bytes.Length); $p.StandardInput.BaseStream.Flush() } catch { }
    }
    try { $p.StandardInput.Close() } catch { }                    # always send EOF so "read stdin" cannot hang
    $timedOut = $false
    if (-not $p.WaitForExit($TimeoutSec * 1000)) { $timedOut = $true; try { $p.Kill() } catch { } }
    $p.WaitForExit()
    [pscustomobject]@{
        Stdout = $outTask.Result; Stderr = $errTask.Result
        ExitCode = $p.ExitCode;   TimedOut = $timedOut
    }
}

# Split output into lines; drop the engine banner; strip the fixtures folder from file names;
# strip ANSI colour codes (unless -KeepAnsi); -Paths also turns \ into / and removes a leading ./
function Get-Lines([string]$Text, [switch]$Paths, [switch]$KeepAnsi) {
    $result = New-Object System.Collections.Generic.List[string]
    if ([string]::IsNullOrEmpty($Text)) { return $result.ToArray() }
    $parts = $Text.Replace("`r`n", "`n").Split([char]10)
    $list = New-Object System.Collections.Generic.List[string]
    foreach ($p in $parts) { $list.Add($p) }
    if ($list[$list.Count - 1] -eq '') { $list.RemoveAt($list.Count - 1) }
    foreach ($l in $list) {
        if ($l -match $BannerRx) {
            if ($l -like 'Engine execution*' -and $result.Count -gt 0 -and $result[$result.Count - 1] -eq '') { $result.RemoveAt($result.Count - 1) }
            continue
        }
        if (-not $KeepAnsi) { $l = [regex]::Replace($l, '\x1b\[[0-9;]*[A-Za-z]', '') }
        $l = [regex]::Replace($l, $FixPrefixRx, '')
        if ($Paths) {
            $l = $l.Replace('\', '/')
            if ($l.StartsWith('./')) { $l = $l.Substring(2) }
        }
        $result.Add($l)
    }
    return $result.ToArray()
}

function Sort-Ordinal([string[]]$Lines) {
    $copy = [string[]]@($Lines)
    [Array]::Sort($copy, [System.StringComparer]::Ordinal)
    return $copy
}

function Write-Block([string]$Label, [string[]]$Lines) {
    Write-Host ("        {0} ({1} line(s))" -f $Label, @($Lines).Count) -ForegroundColor DarkGray
    $n = 0
    foreach ($l in @($Lines)) {
        if ($n -ge 12) { Write-Host '          ...' -ForegroundColor DarkGray; break }
        if ($l.Length -gt 120) { $l = $l.Substring(0, 120) + '...' }
        Write-Host ("          | " + $l)
        $n++
    }
}

function T {
    param(
        [Parameter(Mandatory, Position = 0)][string]$TierName,
        [Parameter(Mandatory, Position = 1)][string]$Name,
        [Parameter(Position = 2)][string[]]$Cmd = @(),       # grep arguments
        [Parameter(Position = 3)][string[]]$Expect = @(),    # expected stdout lines
        [string]$Stdin,
        [int]$Exit = -1,                                     # expected exit code (-1 = not checked)
        [switch]$SkipOut,                                    # do not compare stdout
        [switch]$Sort,                                       # compare stdout ignoring line order
        [switch]$Paths,                                      # normalise path separators in stdout
        [string]$Match,                                      # regex that stdout+stderr must match
        [switch]$ErrNotEmpty,
        [switch]$ErrEmpty,
        [scriptblock]$Check                                  # custom check, returns problem strings
    )
    if ($Tier -notcontains $TierName) { return }
    if ($Name -notlike $Filter) { return }
    if ($List) { Write-Host ("{0,-8} {1}" -f $TierName, $Name); return }

    $r = Invoke-Grep $Cmd $Stdin
    $problems = New-Object System.Collections.Generic.List[string]
    $actual = @(Get-Lines $r.Stdout -Paths:$Paths)
    $errLines = @(Get-Lines $r.Stderr)
    $expected = @($Expect)
    if ($Sort) { $actual = @(Sort-Ordinal $actual); $expected = @(Sort-Ordinal $expected) }

    if ($r.TimedOut) { $problems.Add("timed out after $TimeoutSec s (process killed)") }
    else {
        if (-not $SkipOut -and (($actual -join "`n") -cne ($expected -join "`n") -or $actual.Count -ne $expected.Count)) { $problems.Add('stdout differs') }
        if ($Exit -ge 0 -and $r.ExitCode -ne $Exit) { $problems.Add("exit code is $($r.ExitCode), expected $Exit") }
        if ($ErrNotEmpty -and $errLines.Count -eq 0) { $problems.Add('expected a message on stderr, got none') }
        if ($ErrEmpty -and $errLines.Count -gt 0) { $problems.Add('expected an empty stderr') }
        if ($Match -and (($r.Stdout + "`n" + $r.Stderr) -notmatch $Match)) { $problems.Add("output does not match /$Match/") }
        if ($Check) { foreach ($m in @(& $Check $r)) { if ($m) { $problems.Add([string]$m) } } }
    }

    if ($problems.Count -eq 0) { $status = if ($TierName -eq 'Missing') { 'NEW' } else { 'PASS' } }
    else { $status = if ($TierName -eq 'Missing' -and -not $Strict) { 'TODO' } else { 'FAIL' } }
    [void]$script:Results.Add([pscustomobject]@{ Tier = $TierName; Name = $Name; Status = $status })

    $color = @{ PASS = 'Green'; NEW = 'Cyan'; TODO = 'Yellow'; FAIL = 'Red' }[$status]
    Write-Host ("[{0}] {1,-7} {2}" -f $status, $TierName, $Name) -ForegroundColor $color
    if ($problems.Count -gt 0 -and -not ($status -eq 'TODO' -and $Quiet)) {
        $shown = (@($Cmd) | ForEach-Object {
            $s = [string]$_
            if ($s.StartsWith($FixDir)) { $s = $s.Substring($FixDir.Length + 1) }
            if ($s -eq '' -or $s -match '\s') { '"' + $s + '"' } else { $s }
        }) -join ' '
        Write-Host "        grep $shown" -ForegroundColor DarkGray
        foreach ($p in $problems) { Write-Host "        - $p" -ForegroundColor DarkYellow }
        if ($problems -contains 'stdout differs') {
            Write-Block 'expected' $expected
            Write-Block 'actual  ' $actual
        }
        if ($errLines.Count -gt 0) { Write-Block 'stderr  ' $errLines }
    }
}

# ---------------------------------------------------------------- tests
Write-Host "Engine : $EnginePath"
Write-Host "Script : $ScriptPath"
Write-Host "Fixtures: $FixDir"
Write-Host ''
$clock = [System.Diagnostics.Stopwatch]::StartNew()

# ----------------------------------------------------------------------
# CORE: literal matching (default mode is literal text)
# ----------------------------------------------------------------------
T Core 'literal: simple match' @('apple',(F 'a.txt')) @('apple pie','apple, apple, apple') 
T Core 'literal: no match prints nothing' @('zzz',(F 'a.txt')) @() 
T Core 'literal: case-sensitive by default' @('APPLE',(F 'a.txt')) @('APPLE crumble') 
T Core 'literal: substring inside a word' @('rry',(F 'a.txt')) @('cherry tart') 
T Core 'literal: pattern with a space' @('apple pie',(F 'a.txt')) @('apple pie') 
T Core 'literal: whole line' @('date square',(F 'a.txt')) @('date square') 
T Core 'literal: at end of line' @('crumble',(F 'a.txt')) @('APPLE crumble') 
T Core 'literal: dot is not a wildcard' @('a.b',(F 'regex.txt')) @('a.b') 
T Core 'literal: plus sign' @('a+b',(F 'special.txt')) @('a+b=c') 
T Core 'literal: dollar and dot' @('$5.00',(F 'special.txt')) @('price is $5.00') 
T Core 'literal: parentheses' @('(parens)',(F 'special.txt')) @('(parens) here') 
T Core 'literal: square brackets' @('[brackets]',(F 'special.txt')) @('[brackets] here') 
T Core 'literal: backslash' @('back\slash',(F 'special.txt')) @('back\slash') 
T Core 'literal: asterisk' @('star*star',(F 'special.txt')) @('star*star') 
T Core 'literal: question mark' @('question?',(F 'special.txt')) @('question?') 
T Core 'literal: caret in the middle' @('^top',(F 'special.txt')) @('caret^top') 
T Core 'literal: pipe' @('pipe|char',(F 'special.txt')) @('pipe|char') 
T Core 'literal: braces' @('{1}',(F 'special.txt')) @('brace{1}') 
T Core 'literal: file without trailing newline (first line)' @('first',(F 'noeol.txt')) @('first line') 
T Core 'literal: file without trailing newline (last line)' @('last line',(F 'noeol.txt')) @('last line without newline') 
T Core 'literal: CRLF file' @('two',(F 'crlf.txt')) @('two') 
T Core 'literal: empty file' @('x',(F 'empty.txt')) @() 

# ----------------------------------------------------------------------
# CORE: -i  ignore case
# ----------------------------------------------------------------------
T Core '-i lowercase pattern' @('-i','apple',(F 'a.txt')) @('apple pie','APPLE crumble','apple, apple, apple') 
T Core '-i uppercase pattern' @('-i','APPLE',(F 'a.txt')) @('apple pie','APPLE crumble','apple, apple, apple') 
T Core '-i mixed-case pattern' @('-i','aPpLe',(F 'a.txt')) @('apple pie','APPLE crumble','apple, apple, apple') 
T Core '-i with no letters in the pattern' @('-i','5.00',(F 'special.txt')) @('price is $5.00') 
T Core '-i no match' @('-i','zzz',(F 'a.txt')) @() 

# ----------------------------------------------------------------------
# CORE: -v  invert
# ----------------------------------------------------------------------
T Core '-v apple' @('-v','apple',(F 'a.txt')) @('Banana split','cherry tart','APPLE crumble','banana bread','date square') 
T Core '-v -i apple' @('-v','-i','apple',(F 'a.txt')) @('Banana split','cherry tart','banana bread','date square') 
T Core '-v matches nothing -> every line' @('-v','zzz',(F 'a.txt')) @('apple pie','Banana split','cherry tart','APPLE crumble','banana bread','date square','apple, apple, apple') 
T Core '-v matches everything -> nothing' @('-v','-E','.',(F 'b.txt')) @() 
T Core '-vi bundled' @('-vi','apple',(F 'a.txt')) @('Banana split','cherry tart','banana bread','date square') 

# ----------------------------------------------------------------------
# CORE: -n  line numbers
# ----------------------------------------------------------------------
T Core '-n apple' @('-n','apple',(F 'a.txt')) @('1:apple pie','7:apple, apple, apple') 
T Core '-n no match' @('-n','zzz',(F 'a.txt')) @() 
T Core '-n large line number (big.txt)' @('-n','line 20000',(F 'big.txt')) @('20000:line 20000') 
T Core '-n -v' @('-n','-v','apple',(F 'a.txt')) @('2:Banana split','3:cherry tart','4:APPLE crumble','5:banana bread','6:date square') 
T Core '-ni bundled' @('-ni','apple',(F 'a.txt')) @('1:apple pie','4:APPLE crumble','7:apple, apple, apple') 
T Core '-inv bundled' @('-inv','apple',(F 'a.txt')) @('2:Banana split','3:cherry tart','5:banana bread','6:date square') 

# ----------------------------------------------------------------------
# CORE: -c  count
# ----------------------------------------------------------------------
T Core '-c apple' @('-c','apple',(F 'a.txt')) @('2') 
T Core '-c -i' @('-c','-i','apple',(F 'a.txt')) @('3') 
T Core '-c -v' @('-c','-v','apple',(F 'a.txt')) @('5') 
T Core '-c no match prints 0' @('-c','zzz',(F 'a.txt')) @('0') 
T Core '-c empty file prints 0' @('-c','x',(F 'empty.txt')) @('0') 
T Core '-c ignores -n' @('-c','-n','apple',(F 'a.txt')) @('2') 
T Core '-cv bundled' @('-cv','apple',(F 'a.txt')) @('5') 
T Core '-c every line of big.txt' @('-c','line',(F 'big.txt')) @('20000') 
T Core '-c prefix match in big.txt' @('-c','line 1',(F 'big.txt')) @('11111') 

# ----------------------------------------------------------------------
# CORE: -l  files with matches
# ----------------------------------------------------------------------
T Core '-l two files match' @('-l','apple',(F 'a.txt'),(F 'b.txt')) @('a.txt','b.txt') 
T Core '-l no file matches' @('-l','zzz',(F 'a.txt'),(F 'b.txt')) @() 
T Core '-l only one file matches' @('-l','first',(F 'a.txt'),(F 'b.txt'),(F 'noeol.txt')) @('noeol.txt') 
T Core '-l single file' @('-l','apple',(F 'a.txt')) @('a.txt') 
T Core '-l prints each file once (many matching lines)' @('-l','apple',(F 'a.txt')) @('a.txt') 
T Core '-l -n still just names' @('-l','-n','apple',(F 'a.txt'),(F 'b.txt')) @('a.txt','b.txt') 

# ----------------------------------------------------------------------
# CORE: multiple files, -H / -h
# ----------------------------------------------------------------------
T Core 'multiple files get a file-name prefix' @('apple',(F 'a.txt'),(F 'b.txt')) @('a.txt:apple pie','a.txt:apple, apple, apple','b.txt:apple orchard') 
T Core 'multiple files with -n' @('-n','apple',(F 'a.txt'),(F 'b.txt')) @('a.txt:1:apple pie','a.txt:7:apple, apple, apple','b.txt:5:apple orchard') 
T Core 'multiple files with -c' @('-c','apple',(F 'a.txt'),(F 'b.txt')) @('a.txt:2','b.txt:1') 
T Core 'multiple files with -i -n' @('-in','apple',(F 'a.txt'),(F 'b.txt')) @('a.txt:1:apple pie','a.txt:4:APPLE crumble','a.txt:7:apple, apple, apple','b.txt:5:apple orchard') 
T Core 'multiple files with -v -c' @('-vc','apple',(F 'a.txt'),(F 'b.txt')) @('a.txt:5','b.txt:5') 
T Core '-H forces prefix on a single file' @('-H','apple',(F 'b.txt')) @('b.txt:apple orchard') 
T Core '-H -n' @('-H','-n','apple',(F 'a.txt')) @('a.txt:1:apple pie','a.txt:7:apple, apple, apple') 
T Core '-nH bundled' @('-nH','apple',(F 'a.txt')) @('a.txt:1:apple pie','a.txt:7:apple, apple, apple') 
T Core '-H -c' @('-H','-c','apple',(F 'a.txt')) @('a.txt:2') 
T Core '-h removes prefix with several files' @('-h','apple',(F 'a.txt'),(F 'b.txt')) @('apple pie','apple, apple, apple','apple orchard') 
T Core '-h -n several files' @('-h','-n','apple',(F 'a.txt'),(F 'b.txt')) @('1:apple pie','7:apple, apple, apple','5:apple orchard') 
T Core '-h -c several files' @('-h','-c','apple',(F 'a.txt'),(F 'b.txt')) @('2','1') 
T Core 'last of -H/-h wins' @('-H','-h','apple',(F 'a.txt'),(F 'b.txt')) @('apple pie','apple, apple, apple','apple orchard') 
T Core 'match in only one of several files' @('orchard',(F 'a.txt'),(F 'b.txt')) @('b.txt:apple orchard') 

# ----------------------------------------------------------------------
# CORE: -e  explicit pattern, patterns starting with a dash, "--"
# ----------------------------------------------------------------------
T Core '-e simple' @('-e','apple',(F 'a.txt')) @('apple pie','apple, apple, apple') 
T Core '-e with a space' @('-e','apple pie',(F 'a.txt')) @('apple pie') 
T Core '-e pattern that looks like an option (-n)' @('-e','-n',(F 'dash.txt')) @('-n is an option') 
T Core '-e pattern that looks like an option (--verbose)' @('-e','--verbose',(F 'dash.txt')) @('--verbose flag') 
T Core '-e pattern "--"' @('-e','--',(F 'dash.txt')) @('--verbose flag','a -- b') 
T Core '"--" ends option parsing, pattern -n' @('--','-n',(F 'dash.txt')) @('-n is an option') 
T Core '"--" ends option parsing, normal pattern' @('--','apple',(F 'a.txt')) @('apple pie','apple, apple, apple') 
T Core '-e combined with other options' @('-n','-i','-e','APPLE',(F 'a.txt')) @('1:apple pie','4:APPLE crumble','7:apple, apple, apple') 

# ----------------------------------------------------------------------
# CORE: -F  fixed strings
# ----------------------------------------------------------------------
T Core '-F dot is literal' @('-F','a.b',(F 'regex.txt')) @('a.b') 
T Core '-F plus is literal' @('-F','a+b',(F 'special.txt')) @('a+b=c') 
T Core '-F with -i' @('-F','-i','APPLE',(F 'a.txt')) @('apple pie','APPLE crumble','apple, apple, apple') 
T Core '-F brackets' @('-F','[brackets]',(F 'special.txt')) @('[brackets] here') 

# ----------------------------------------------------------------------
# CORE: -E  regular expressions
# ----------------------------------------------------------------------
T Core '-E digits' @('-E','[0-9]+',(F 'regex.txt')) @('abc123','foo42bar','2024-11-27') 
T Core '-E . matches one char' @('-E','^a.b$',(F 'regex.txt')) @('a.b','axb') 
T Core '-E escaped dot' @('-E','a\.b',(F 'regex.txt')) @('a.b') 
T Core '-E whole line of lowercase letters' @('-E','^[a-z]+$',(F 'regex.txt')) @('axb','x','aaa','color','colour','ababab','abcd','abc') 
T Core '-E negated class' @('-E','[^a-z ]',(F 'a.txt')) @('Banana split','APPLE crumble','apple, apple, apple') 
T Core '-E alternation' @('-E','foo|aaa',(F 'regex.txt')) @('foo42bar','aaa') 
T Core '-E group + alternation + plus' @('-E','^(abc|foo)[0-9]+',(F 'regex.txt')) @('abc123','foo42bar') 
T Core '-E optional (colou?r)' @('-E','colou?r',(F 'regex.txt')) @('color','colour') 
T Core '-E star' @('-E','^a*$',(F 'regex.txt')) @('aaa') 
T Core '-E star matches empty -> every line' @('-c','-E','x*',(F 'regex.txt')) @('15') 
T Core '-E anchor ^' @('-E','^apple',(F 'a.txt')) @('apple pie','apple, apple, apple') 
T Core '-E anchor $' @('-E','pie$',(F 'a.txt')) @('apple pie') 
T Core '-E ^$ finds blank lines (-n)' @('-n','-E','^$',(F 'blank.txt')) @('2:','4:') 
T Core '-E ^$ finds blank lines' @('-E','^$',(F 'blank.txt')) @('','') 
T Core '-E -v ^$ drops blank lines' @('-v','-E','^$',(F 'blank.txt')) @('a','b') 
T Core '-E -c ^$' @('-c','-E','^$',(F 'blank.txt')) @('2') 
T Core '-E repeated group' @('-E','(an)+a',(F 'a.txt')) @('Banana split','banana bread') 
T Core '-E nested group repeat' @('-E','^(ab)+$',(F 'regex.txt')) @('ababab') 
T Core '-E backtracking (a|ab)(c|bcd)' @('-E','^(a|ab)(c|bcd)$',(F 'regex.txt')) @('abcd','abc') 
T Core '-E a.*b.*c' @('-E','a.*b.*c',(F 'regex.txt')) @('abc123','abcd','abc') 
T Core '-E with -i' @('-i','-E','APPLE|BANANA',(F 'a.txt')) @('apple pie','Banana split','APPLE crumble','banana bread','apple, apple, apple') 
T Core '-iE bundled, anchored' @('-iE','^APPLE',(F 'a.txt')) @('apple pie','APPLE crumble','apple, apple, apple') 
T Core '-E with -n' @('-n','-E','^[a-z]+$',(F 'regex.txt')) @('6:axb','9:x','10:aaa','11:color','12:colour','13:ababab','14:abcd','15:abc') 
T Core '-E with -c' @('-c','-E','[0-9]+',(F 'regex.txt')) @('3') 
T Core '-E with -v' @('-v','-E','[0-9]',(F 'regex.txt')) @('no digits here','a.b','axb','test@example.com','  indented line','x','aaa','color','colour','ababab','abcd','abc') 
T Core '-E escaped plus' @('-E','a\+b',(F 'special.txt')) @('a+b=c') 
T Core '-E escaped dollar and dot' @('-E','\$5\.00',(F 'special.txt')) @('price is $5.00') 
T Core '-E escaped parentheses' @('-E','\(parens\)',(F 'special.txt')) @('(parens) here') 
T Core '-E escaped brackets' @('-E','\[brackets\]',(F 'special.txt')) @('[brackets] here') 
T Core '-E escaped backslash' @('-E','back\\slash',(F 'special.txt')) @('back\slash') 
T Core '-E escaped star' @('-E','star\*star',(F 'special.txt')) @('star*star') 
T Core '-E escaped question mark' @('-E','question\?',(F 'special.txt')) @('question?') 
T Core '-E escaped pipe' @('-E','pipe\|char',(F 'special.txt')) @('pipe|char') 
T Core '-E range in class' @('-E','^[a-c]+[0-9]',(F 'regex.txt')) @('abc123') 
T Core '-E class with dot and dash' @('-E','[.-]',(F 'regex.txt')) @('2024-11-27','a.b','test@example.com') 
T Core '-E multiple files' @('-E','^(apple|alpha)',(F 'a.txt'),(F 'b.txt')) @('a.txt:apple pie','a.txt:apple, apple, apple','b.txt:alpha','b.txt:apple orchard') 
T Core '-E -F: last one wins (-F)' @('-E','-F','a.b',(F 'regex.txt')) @('a.b') 
T Core '-F -E: last one wins (-E)' @('-F','-E','^a.b$',(F 'regex.txt')) @('a.b','axb') 

# ----------------------------------------------------------------------
# CORE: standard input
# ----------------------------------------------------------------------
T Core 'stdin: no FILE reads stdin' @('apple') @('apple','apple pie') -Stdin "apple`nbanana`napple pie`nCherry`n"
T Core 'stdin: -n' @('-n','apple') @('1:apple','3:apple pie') -Stdin "apple`nbanana`napple pie`nCherry`n"
T Core 'stdin: -c' @('-c','apple') @('2') -Stdin "apple`nbanana`napple pie`nCherry`n"
T Core 'stdin: -v' @('-v','apple') @('banana','Cherry') -Stdin "apple`nbanana`napple pie`nCherry`n"
T Core 'stdin: -i' @('-i','cherry') @('Cherry') -Stdin "apple`nbanana`napple pie`nCherry`n"
T Core 'stdin: -E' @('-E','^(apple|banana)$') @('apple','banana') -Stdin "apple`nbanana`napple pie`nCherry`n"
T Core 'stdin: no match' @('zzz') @() -Stdin "apple`nbanana`napple pie`nCherry`n"
T Core 'stdin: empty input' @('apple') @() -Stdin ''
T Core 'stdin: no trailing newline' @('pie') @('apple pie') -Stdin 'apple pie'
T Core 'stdin: lines with spaces only' @('-c','-E','^ +$') @('2') -Stdin "a`n  `n `nb`n"

# ----------------------------------------------------------------------
# CORE: recursive search (-r / -R)
# ----------------------------------------------------------------------
T Core '-r finds matches in nested folders' @('-r','needle',(F 'tree')) @('tree/sub/deep.txt:needle deep','tree/sub/more/deeper.txt:needle deeper','tree/sub/note.md:needle md','tree/top.txt:needle top') -Sort -Paths
T Core '-R is the same as -r' @('-R','needle',(F 'tree')) @('tree/sub/deep.txt:needle deep','tree/sub/more/deeper.txt:needle deeper','tree/sub/note.md:needle md','tree/top.txt:needle top') -Sort -Paths
T Core '-rl lists files only' @('-rl','needle',(F 'tree')) @('tree/sub/deep.txt','tree/sub/more/deeper.txt','tree/sub/note.md','tree/top.txt') -Sort -Paths
T Core '-rn shows line numbers' @('-rn','needle',(F 'tree')) @('tree/sub/deep.txt:1:needle deep','tree/sub/more/deeper.txt:2:needle deeper','tree/sub/note.md:1:needle md','tree/top.txt:1:needle top') -Sort -Paths
T Core '-ri ignores case' @('-ri','NEEDLE',(F 'tree')) @('tree/sub/deep.txt:needle deep','tree/sub/more/deeper.txt:needle deeper','tree/sub/note.md:needle md','tree/top.txt:needle top') -Sort -Paths
T Core '-rh drops file names' @('-rh','needle',(F 'tree')) @('needle deep','needle deeper','needle md','needle top') -Sort -Paths
T Core '-rv hides matching lines' @('-rv','needle',(F 'tree')) @('tree/sub/more/deeper.txt:hay only','tree/top.txt:hay') -Sort -Paths
T Core '-rE regex' @('-rE','needle (top|deep)',(F 'tree')) @('tree/sub/deep.txt:needle deep','tree/sub/more/deeper.txt:needle deeper','tree/top.txt:needle top') -Sort -Paths
T Core '-r on a sub folder' @('-r','needle',(F 'tree/sub')) @('tree/sub/deep.txt:needle deep','tree/sub/more/deeper.txt:needle deeper','tree/sub/note.md:needle md') -Sort -Paths
T Core '-r no match prints nothing' @('-r','zzz',(F 'tree')) @() -Sort -Paths
T Core '-r skips binary files' @('-rl','needle',(F 'tree')) @('tree/sub/deep.txt','tree/sub/more/deeper.txt','tree/sub/note.md','tree/top.txt') -Sort -Paths
T Core '-r with several roots' @('-rl','needle',(F 'tree/sub/more'),(F 'tree/top.txt')) @('tree/sub/more/deeper.txt','tree/top.txt') -Sort -Paths
T Edge '-r given a plain file works' @('-r','needle',(F 'tree/top.txt')) @('needle top') -Paths

# ----------------------------------------------------------------------
# CORE: binary files
# ----------------------------------------------------------------------
T Core 'binary file (NUL early) is skipped' @('apple',(F 'binary.bin')) @() 
T Core 'binary file skipped, text file still searched' @('-l','apple',(F 'binary.bin'),(F 'a.txt')) @('a.txt') 

# ----------------------------------------------------------------------
# CORE: --color
# ----------------------------------------------------------------------
T Core '--color: the match is wrapped in an ANSI sequence' @('--color','apple',(F 'b.txt')) -SkipOut -Check {
    param($r)
    $raw = @(Get-Lines $r.Stdout -KeepAnsi)
    $plain = @(Get-Lines $r.Stdout)
    if ($raw.Count -eq 0) { 'no output' }
    foreach ($l in $raw) { if ($l.IndexOf([char]27) -lt 0) { "line without an ANSI escape: $l" } }
    if (($plain -join "`n") -cne 'apple orchard') { "text without ANSI was '$($plain -join '|')', expected 'apple orchard'" }
}
T Core '--color: every match on a line is highlighted' @('--color','apple',(F 'a.txt')) -SkipOut -Check {
    param($r)
    $esc = ([regex]::Matches($r.Stdout, '\x1b\[')).Count
    if ($esc -lt 4) { "only $esc ANSI sequence(s) for 4 matches" }
    $plain = @(Get-Lines $r.Stdout)
    if (($plain -join "`n") -cne "apple pie`napple, apple, apple") { "text without ANSI was '$($plain -join '|')'" }
}
T Core '--color: no escapes when --color is not given' @('apple',(F 'b.txt')) -SkipOut -Check {
    param($r)
    if ($r.Stdout.IndexOf([char]27) -ge 0) { 'ANSI escape found without --color' }
}
T Core '--color: no match prints nothing' @('--color','zzz',(F 'b.txt')) -SkipOut -Check {
    param($r)
    $l = @(Get-Lines $r.Stdout -KeepAnsi)
    if ($l.Count -ne 0) { "expected no output, got $($l.Count) line(s)" }
}
T Core '--color -i keeps the original letter case' @('--color','-i','apple',(F 'a.txt')) -SkipOut -Check {
    param($r)
    $plain = @(Get-Lines $r.Stdout)
    if (($plain -join "`n") -cne "apple pie`nAPPLE crumble`napple, apple, apple") { "text without ANSI was '$($plain -join '|')'" }
    if ($r.Stdout.IndexOf([char]27) -lt 0) { 'no ANSI escape in the output' }
}
T Core '--color -n keeps line numbers' @('--color','-n','apple',(F 'a.txt')) -SkipOut -Check {
    param($r)
    $plain = @(Get-Lines $r.Stdout)
    if (($plain -join "`n") -cne "1:apple pie`n7:apple, apple, apple") { "text without ANSI was '$($plain -join '|')'" }
}
T Core '--color with several files keeps file-name prefixes' @('--color','apple',(F 'a.txt'),(F 'b.txt')) -SkipOut -Check {
    param($r)
    $plain = @(Get-Lines $r.Stdout)
    if (($plain -join "`n") -cne "a.txt:apple pie`na.txt:apple, apple, apple`nb.txt:apple orchard") { "text without ANSI was '$($plain -join '|')'" }
}
T Core '--color -E highlights regex matches' @('--color','-E','[0-9]+',(F 'regex.txt')) -SkipOut -Check {
    param($r)
    $raw = @(Get-Lines $r.Stdout -KeepAnsi)
    foreach ($l in $raw) { if ($l.IndexOf([char]27) -lt 0) { "line without an ANSI escape: $l" } }
    $plain = @(Get-Lines $r.Stdout)
    if (($plain -join "`n") -cne "abc123`nfoo42bar`n2024-11-27") { "text without ANSI was '$($plain -join '|')'" }
}
T Core '--color -c prints a plain count' @('--color','-c','apple',(F 'a.txt')) -Expect @('2')

# ----------------------------------------------------------------------
# CORE: usage and help
# ----------------------------------------------------------------------
T Core '--help prints the usage text' @('--help') @() -SkipOut -Match 'Usage: grep'
T Core 'no arguments prints the usage text' @() @() -SkipOut -Match 'Usage: grep'
T Core 'options but no pattern prints the usage text' @('-n') @() -SkipOut -Match 'Usage: grep'

# ----------------------------------------------------------------------
# EDGE: behaviour real grep has, not spelled out in the usage text
# ----------------------------------------------------------------------
T Edge 'empty pattern matches every line' @('',(F 'b.txt')) @('alpha','beta','gamma','delta','apple orchard','epsilon') 
T Edge 'empty pattern with -c' @('-c','',(F 'a.txt')) @('7') 
T Edge 'empty pattern with -v matches nothing' @('-v','',(F 'a.txt')) @() 
T Edge '"-" as FILE means stdin' @('apple','-') @('apple','apple pie') -Stdin "apple`nbanana`napple pie`nCherry`n"
T Edge 'options after the pattern (permutation)' @('apple','-n',(F 'a.txt')) @('1:apple pie','7:apple, apple, apple') 
T Edge 'options after the file' @('apple',(F 'a.txt'),'-n') @('1:apple pie','7:apple, apple, apple') 
T Edge '-e glued: -ne apple' @('-ne','apple',(F 'a.txt')) @('1:apple pie','7:apple, apple, apple') 
T Edge '-e after the file' @((F 'a.txt'),'-e','apple') @('apple pie','apple, apple, apple') 
T Edge 'pattern is the only line content' @('plain line',(F 'dash.txt')) @('plain line') 
T Edge 'pattern "-" (single dash) via -e' @('-e','-',(F 'dash.txt')) @('-n is an option','--verbose flag','a -- b','-') 
T Edge 'pattern " -- " in the middle of a line' @('a -- b',(F 'dash.txt')) @('a -- b') 
T Edge 'long line (100 KB)' @('-c','needle',(F 'long.txt')) @('1') 
T Edge 'long line: print it and measure length' @('short',(F 'long.txt')) @('short line') 
T Edge 'big file: last line' @('line 20000',(F 'big.txt')) @('line 20000') 
T Edge 'big file: -v -c' @('-v','-c','line',(F 'big.txt')) @('0') 
T Edge 'big file: count of lines ending in 7' @('-c','-E','7$',(F 'big.txt')) @('2000') 
T Edge 'NUL after the first 8 KB: file still searched' @('apple',(F 'late-nul.bin')) @('apple start') 
T Edge 'UTF-8 text: literal' @('café',(F 'uni.txt')) @('naïve café') 
T Edge 'UTF-8 text: CJK literal' @('日本語',(F 'uni.txt')) @('日本語 text') 
T Edge 'UTF-8 text: untouched ASCII neighbours' @('plain',(F 'uni.txt')) @('plain') 
T Edge '-E escaped specials in a single class' @('-E','[$^*?]',(F 'special.txt')) @('price is $5.00','star*star','question?','caret^top') 
T Edge '-E interval {n}' @('-E','p{2}',(F 'a.txt')) @('apple pie','apple, apple, apple') 
T Edge '-E interval {n,m}' @('-E','^.{1,3}$',(F 'regex.txt')) @('a.b','axb','x','aaa','abc') 
T Edge '-E interval {n,}' @('-E','^a{3,}$',(F 'regex.txt')) @('aaa') 
T Edge '-E POSIX class [[:digit:]]' @('-E','[[:digit:]]+',(F 'regex.txt')) @('abc123','foo42bar','2024-11-27') 
T Edge '-E POSIX class [[:upper:]]' @('-E','[[:upper:]]',(F 'a.txt')) @('Banana split','APPLE crumble') 
T Edge '-E POSIX class [[:alpha:]]' @('-c','-E','^[[:alpha:]]+$',(F 'regex.txt')) @('8') 
T Edge '-E POSIX class [[:space:]]' @('-E','[[:space:]]',(F 'dash.txt')) @('-n is an option','--verbose flag','plain line','a -- b') 
T Edge '-iE with a range' @('-iE','^[a-b]anana',(F 'a.txt')) @('Banana split','banana bread') 
T Edge '-E dot does not match across lines' @('-c','-E','a.*e',(F 'a.txt')) @('4') 
T Edge '-E a*b (star after a literal)' @('-E','a*b',(F 'regex.txt')) @('abc123','foo42bar','a.b','axb','ababab','abcd','abc') 
T Edge '-E empty alternative branch' @('-c','-E','^(a|)$',(F 'blank.txt')) @('3') 
T Edge '-E . on empty lines does not match' @('-c','-E','.',(F 'blank.txt')) @('2') 
T Edge 'file named on the command line twice' @('apple',(F 'b.txt'),(F 'b.txt')) @('b.txt:apple orchard','b.txt:apple orchard') 
T Edge 'directory without -r prints no matches' @('needle',(F 'tree')) @() 
T Edge 'missing file prints nothing to stdout' @('apple',(F 'nope.txt')) @() 
T Edge 'good file + missing file: good output still printed' @('apple',(F 'a.txt'),(F 'nope.txt')) @('a.txt:apple pie','a.txt:apple, apple, apple') 
T Edge 'missing file reports an error on stderr' @('apple',(F 'nope.txt')) @() -SkipOut -ErrNotEmpty
T Edge 'unknown option is rejected' @('--bogus','apple',(F 'a.txt')) @() -Match '(?i)unknown|invalid|usage|unrecognized'
T Edge '-e without a value is rejected' @('-e') @() -Match '(?i)requires|missing|usage|argument'
T Edge 'invalid regex reports an error' @('-E','(ab',(F 'a.txt')) @() -SkipOut -ErrNotEmpty
T Edge 'invalid regex (unclosed bracket) reports an error' @('-E','[a',(F 'a.txt')) @() -SkipOut -ErrNotEmpty
T Edge 'invalid regex prints nothing to stdout' @('-E','(ab',(F 'a.txt')) @() 

# ----------------------------------------------------------------------
# EXIT STATUS: 0 = match, 1 = no match, 2 = error
# ----------------------------------------------------------------------
T Exit 'exit 0 when a line matches' @('apple',(F 'a.txt')) @('apple pie','apple, apple, apple') -Exit 0
T Exit 'exit 1 when nothing matches' @('zzz',(F 'a.txt')) @() -Exit 1
T Exit 'exit 1 for an empty file' @('x',(F 'empty.txt')) @() -Exit 1
T Exit 'exit 0 for -c when lines match' @('-c','apple',(F 'a.txt')) @('2') -Exit 0
T Exit 'exit 1 for -c with count 0' @('-c','zzz',(F 'a.txt')) @('0') -Exit 1
T Exit 'exit 0 for -v when lines remain' @('-v','apple',(F 'a.txt')) @('Banana split','cherry tart','APPLE crumble','banana bread','date square') -Exit 0
T Exit 'exit 1 for -v when nothing remains' @('-v','-E','.',(F 'b.txt')) @() -Exit 1
T Exit 'exit 0 for -l with a match' @('-l','apple',(F 'a.txt')) @('a.txt') -Exit 0
T Exit 'exit 1 for -l without a match' @('-l','zzz',(F 'a.txt')) @() -Exit 1
T Exit 'exit 0 on stdin match' @('apple') @('apple','apple pie') -Stdin "apple`nbanana`napple pie`nCherry`n" -Exit 0
T Exit 'exit 1 on stdin no match' @('zzz') @() -Stdin "apple`nbanana`napple pie`nCherry`n" -Exit 1
T Exit 'exit 0 when only one of several files matches' @('orchard',(F 'a.txt'),(F 'b.txt')) @('b.txt:apple orchard') -Exit 0
T Exit 'exit 2 for a missing file' @('apple',(F 'nope.txt')) @() -Exit 2 -SkipOut
T Exit 'exit 2 for missing file even if another file matches' @('apple',(F 'a.txt'),(F 'nope.txt')) @('a.txt:apple pie','a.txt:apple, apple, apple') -Exit 2
T Exit 'exit 2 for an invalid regex' @('-E','(ab',(F 'a.txt')) @() -Exit 2 -SkipOut
T Exit 'exit 2 for a directory without -r' @('needle',(F 'tree')) @() -Exit 2 -SkipOut
T Exit 'exit 0 for -r with a match' @('-r','needle',(F 'tree')) @('tree/top.txt:needle top','tree/sub/more/deeper.txt:needle deeper','tree/sub/note.md:needle md','tree/sub/deep.txt:needle deep') -Exit 0 -SkipOut
T Exit 'exit 1 for -r without a match' @('-r','zzz',(F 'tree')) @() -Exit 1 -SkipOut
T Exit 'exit 2 when no arguments are given' @() @() -Exit 2 -SkipOut
T Exit 'exit 2 when there is no pattern' @('-n') @() -Exit 2 -SkipOut
T Exit 'exit 0 for --help' @('--help') @() -Exit 0 -SkipOut
T Exit 'exit 2 for an unknown option' @('--bogus','apple',(F 'a.txt')) @() -Exit 2 -SkipOut

# ----------------------------------------------------------------------
# MISSING (not implemented yet): matching options
# ----------------------------------------------------------------------
T Missing '-w whole words' @('-w','cat',(F 'words.txt')) @('cat','the cat sat','cat-like') 
T Missing '-w with -i' @('-wi','CAT',(F 'words.txt')) @('cat','the cat sat','cat-like') 
T Missing '-w with -c' @('-wc','cat',(F 'words.txt')) @('3') 
T Missing '-w with a regex' @('-wE','ca(t|ts)',(F 'words.txt')) @('cat','cats','the cat sat','cat-like') 
T Missing '-x whole lines' @('-x','cat',(F 'words.txt')) @('cat') 
T Missing '-x with -i' @('-xi','CAT',(F 'words.txt')) @('cat') 
T Missing '-x with a regex' @('-xE','cats?',(F 'words.txt')) @('cat','cats') 
T Missing '-x -v' @('-xv','cat',(F 'words.txt')) @('cats','concat','the cat sat','cat-like','bobcat') 
T Missing 'two -e patterns' @('-e','apple','-e','cherry',(F 'a.txt')) @('apple pie','cherry tart','apple, apple, apple') 
T Missing 'three -e patterns with -n' @('-n','-e','apple','-e','cherry','-e','date',(F 'a.txt')) @('1:apple pie','3:cherry tart','6:date square','7:apple, apple, apple') 
T Missing '-e patterns with -i' @('-i','-e','BANANA','-e','CHERRY',(F 'a.txt')) @('Banana split','cherry tart','banana bread') 
T Missing '-e patterns with -c' @('-c','-e','apple','-e','cherry',(F 'a.txt')) @('3') 
T Missing '-e patterns with -v' @('-v','-e','apple','-e','banana',(F 'a.txt')) @('Banana split','cherry tart','APPLE crumble','date square') 
T Missing '-f reads patterns from a file' @('-f',(F 'pats.txt'),(F 'a.txt')) @('cherry tart','date square') 
T Missing '-f combined with -n' @('-n','-f',(F 'pats.txt'),(F 'a.txt')) @('3:cherry tart','6:date square') 
T Missing '-f with -F' @('-F','-f',(F 'pats.txt'),(F 'a.txt')) @('cherry tart','date square') 
T Missing '-E with \b word boundary' @('-E','\bcat\b',(F 'words.txt')) @('cat','the cat sat','cat-like') 
T Missing '-E with \w+' @('-E','^\w+$',(F 'words.txt')) @('cat','cats','concat','bobcat') 
T Missing '-E with \s' @('-E','^\s+indented',(F 'regex.txt')) @('  indented line') 
T Missing '-E back-reference' @('-E','(a)\1',(F 'regex.txt')) @('aaa') 
T Missing '-E \< \> word anchors' @('-E','\<cat\>',(F 'words.txt')) @('cat','the cat sat','cat-like') 

# ----------------------------------------------------------------------
# MISSING: output options
# ----------------------------------------------------------------------
T Missing '-o prints only the match' @('-o','apple',(F 'a.txt')) @('apple','apple','apple','apple') 
T Missing '-on prints line numbers with each match' @('-on','apple',(F 'a.txt')) @('1:apple','7:apple','7:apple','7:apple') 
T Missing '-oE digits' @('-oE','[0-9]+',(F 'regex.txt')) @('123','42','2024','11','27') 
T Missing '-oi' @('-oi','apple',(F 'a.txt')) @('apple','APPLE','apple','apple','apple') 
T Missing '-o with several files' @('-o','apple',(F 'a.txt'),(F 'b.txt')) @('a.txt:apple','a.txt:apple','a.txt:apple','a.txt:apple','b.txt:apple') 
T Missing '-oc counts lines, not matches' @('-oc','apple',(F 'a.txt')) @('2') 
T Missing '-q is silent, exit 0 on match' @('-q','apple',(F 'a.txt')) @() -Exit 0
T Missing '-q is silent, exit 1 on no match' @('-q','zzz',(F 'a.txt')) @() -Exit 1
T Missing '-q stops at the first match even if a file is missing' @('-q','apple',(F 'a.txt'),(F 'nope.txt')) @() -Exit 0
T Missing '-m 1 stops after one match' @('-m','1','apple',(F 'a.txt')) @('apple pie') 
T Missing '-m 2 with -i' @('-m','2','-i','apple',(F 'a.txt')) @('apple pie','APPLE crumble') 
T Missing '-m with -c' @('-m','2','-c','-i','apple',(F 'a.txt')) @('2') 
T Missing '-m with several files' @('-m','1','apple',(F 'a.txt'),(F 'b.txt')) @('a.txt:apple pie','b.txt:apple orchard') 
T Missing '--max-count=1' @('--max-count=1','apple',(F 'a.txt')) @('apple pie') 
T Missing '-L lists files without a match' @('-L','apple',(F 'a.txt'),(F 'b.txt'),(F 'noeol.txt')) @('noeol.txt') 
T Missing '-L when every file matches' @('-L','apple',(F 'a.txt'),(F 'b.txt')) @() 
T Missing '-s hides missing-file errors' @('-s','apple',(F 'nope.txt')) @() -Exit 2 -SkipOut -ErrEmpty
T Missing '-s keeps normal output' @('-s','apple',(F 'a.txt'),(F 'nope.txt')) @('a.txt:apple pie','a.txt:apple, apple, apple') -Exit 2
T Missing '-b byte offset' @('-b','apple',(F 'b.txt')) @('23:apple orchard') 

# ----------------------------------------------------------------------
# MISSING: context lines
# ----------------------------------------------------------------------
T Missing '-A 1 (after)' @('-A','1','L5',(F 'ctx.txt')) @('L5','L6') 
T Missing '-A1 glued' @('-A1','L5',(F 'ctx.txt')) @('L5','L6') 
T Missing '-B 1 (before)' @('-B','1','L5',(F 'ctx.txt')) @('L4','L5') 
T Missing '-B2 glued' @('-B2','L5',(F 'ctx.txt')) @('L3','L4','L5') 
T Missing '-C 1 (both)' @('-C','1','L5',(F 'ctx.txt')) @('L4','L5','L6') 
T Missing '-C1 glued' @('-C1','L5',(F 'ctx.txt')) @('L4','L5','L6') 
T Missing '-A 2 near the end of the file' @('-A','2','L9',(F 'ctx.txt')) @('L9','L10') 
T Missing '-B 3 near the start of the file' @('-B','3','L2',(F 'ctx.txt')) @('L1','L2') 
T Missing '-n -C 1 uses : and - separators' @('-n','-C','1','L5',(F 'ctx.txt')) @('4-L4','5:L5','6-L6') 
T Missing 'context groups separated by "--"' @('-C','1','-E','L(2|8)$',(F 'ctx.txt')) @('L1','L2','L3','--','L7','L8','L9') 
T Missing 'overlapping context groups merge' @('-C','2','-E','L(4|6)$',(F 'ctx.txt')) @('L2','L3','L4','L5','L6','L7','L8') 
T Missing 'adjacent context groups merge, no separator' @('-A','1','-E','L(2|4)$',(F 'ctx.txt')) @('L2','L3','L4','L5') 
T Missing '-A 1 with -v' @('-v','-A','1','-E','L[0-9]$',(F 'ctx.txt')) @('L10') 
T Missing '-c ignores context' @('-c','-C','1','L5',(F 'ctx.txt')) @('1') 
T Missing '-C with several files' @('-C','1','apple',(F 'a.txt'),(F 'b.txt')) @('a.txt:apple pie','a.txt-Banana split','--','a.txt-date square','a.txt:apple, apple, apple','--','b.txt-delta','b.txt:apple orchard','b.txt-epsilon') 
T Missing '--context=1' @('--context=1','L5',(F 'ctx.txt')) @('L4','L5','L6') 
T Missing '--after-context=1' @('--after-context=1','L5',(F 'ctx.txt')) @('L5','L6') 
T Missing '--before-context=1' @('--before-context=1','L5',(F 'ctx.txt')) @('L4','L5') 

# ----------------------------------------------------------------------
# MISSING: recursion filters
# ----------------------------------------------------------------------
T Missing '--include limits files by glob' @('-r','--include=*.txt','needle',(F 'tree')) @('tree/sub/deep.txt:needle deep','tree/sub/more/deeper.txt:needle deeper','tree/top.txt:needle top') -Sort -Paths
T Missing '--include as two arguments' @('-r','--include','*.md','needle',(F 'tree')) @('tree/sub/note.md:needle md') -Sort -Paths
T Missing '--exclude removes files by glob' @('-r','--exclude=*.md','needle',(F 'tree')) @('tree/sub/deep.txt:needle deep','tree/sub/more/deeper.txt:needle deeper','tree/top.txt:needle top') -Sort -Paths
T Missing '--exclude-dir removes folders' @('-r','--exclude-dir=more','needle',(F 'tree')) @('tree/sub/deep.txt:needle deep','tree/sub/note.md:needle md','tree/top.txt:needle top') -Sort -Paths
T Missing '--include + -l' @('-rl','--include=*.txt','needle',(F 'tree')) @('tree/sub/deep.txt','tree/sub/more/deeper.txt','tree/top.txt') -Sort -Paths

# ----------------------------------------------------------------------
# MISSING: long option names
# ----------------------------------------------------------------------
T Missing '--ignore-case' @('--ignore-case','apple',(F 'a.txt')) @('apple pie','APPLE crumble','apple, apple, apple') 
T Missing '--line-number' @('--line-number','apple',(F 'a.txt')) @('1:apple pie','7:apple, apple, apple') 
T Missing '--count' @('--count','apple',(F 'a.txt')) @('2') 
T Missing '--invert-match' @('--invert-match','apple',(F 'a.txt')) @('Banana split','cherry tart','APPLE crumble','banana bread','date square') 
T Missing '--files-with-matches' @('--files-with-matches','apple',(F 'a.txt'),(F 'b.txt')) @('a.txt','b.txt') 
T Missing '--files-without-match' @('--files-without-match','apple',(F 'a.txt'),(F 'noeol.txt')) @('noeol.txt') 
T Missing '--regexp=PATTERN' @('--regexp=apple',(F 'a.txt')) @('apple pie','apple, apple, apple') 
T Missing '--regexp PATTERN' @('--regexp','apple',(F 'a.txt')) @('apple pie','apple, apple, apple') 
T Missing '--extended-regexp' @('--extended-regexp','^(apple|alpha)',(F 'a.txt'),(F 'b.txt')) @('a.txt:apple pie','a.txt:apple, apple, apple','b.txt:alpha','b.txt:apple orchard') 
T Missing '--fixed-strings' @('--fixed-strings','a.b',(F 'regex.txt')) @('a.b') 
T Missing '--with-filename' @('--with-filename','apple',(F 'b.txt')) @('b.txt:apple orchard') 
T Missing '--no-filename' @('--no-filename','apple',(F 'a.txt'),(F 'b.txt')) @('apple pie','apple, apple, apple','apple orchard') 
T Missing '--recursive' @('--recursive','needle',(F 'tree')) @('tree/sub/deep.txt:needle deep','tree/sub/more/deeper.txt:needle deeper','tree/sub/note.md:needle md','tree/top.txt:needle top') -Sort -Paths
T Missing '--dereference-recursive (-R long form)' @('--dereference-recursive','needle',(F 'tree')) @('tree/sub/deep.txt:needle deep','tree/sub/more/deeper.txt:needle deeper','tree/sub/note.md:needle md','tree/top.txt:needle top') -Sort -Paths
T Missing '--word-regexp' @('--word-regexp','cat',(F 'words.txt')) @('cat','the cat sat','cat-like') 
T Missing '--line-regexp' @('--line-regexp','cat',(F 'words.txt')) @('cat') 
T Missing '--only-matching' @('--only-matching','apple',(F 'a.txt')) @('apple','apple','apple','apple') 
T Missing '--quiet' @('--quiet','apple',(F 'a.txt')) @() -Exit 0
T Missing '--silent' @('--silent','zzz',(F 'a.txt')) @() -Exit 1
T Missing '--no-messages' @('--no-messages','apple',(F 'nope.txt')) @() -Exit 2 -SkipOut
T Missing '--file=PATTERNS' @(('--file=' + (F 'pats.txt')),(F 'a.txt')) @('cherry tart','date square') 
T Missing '--version prints a version string' @('--version') @() -SkipOut -Match '(?i)grep|version|\d+\.\d+'

# ---------------------------------------------------------------- summary
if ($List) { return }
$clock.Stop()
Write-Host ''
Write-Host ('=' * 64)
foreach ($tn in 'Core','Edge','Exit','Missing') {
    $rs = @($script:Results | Where-Object { $_.Tier -eq $tn })
    if ($rs.Count -eq 0) { continue }
    $pass = @($rs | Where-Object { $_.Status -eq 'PASS' }).Count
    $new  = @($rs | Where-Object { $_.Status -eq 'NEW' }).Count
    $todo = @($rs | Where-Object { $_.Status -eq 'TODO' }).Count
    $fail = @($rs | Where-Object { $_.Status -eq 'FAIL' }).Count
    $line = "{0,-8} {1,3} tests   pass {2,3}   fail {3,3}" -f $tn, $rs.Count, ($pass + $new), $fail
    if ($tn -eq 'Missing') { $line = "{0,-8} {1,3} tests   implemented {2,3}   not yet {3,3}" -f $tn, $rs.Count, $new, ($todo + $fail) }
    $c = if ($fail -gt 0) { 'Red' } elseif ($tn -eq 'Missing') { 'Yellow' } else { 'Green' }
    Write-Host $line -ForegroundColor $c
}
$failed = @($script:Results | Where-Object { $_.Status -eq 'FAIL' })
Write-Host ('{0} test(s) run in {1:N1}s, {2} failed' -f $script:Results.Count, $clock.Elapsed.TotalSeconds, $failed.Count)
if ($failed.Count -gt 0) {
    Write-Host ''
    Write-Host 'Failed:' -ForegroundColor Red
    foreach ($f in $failed) { Write-Host ("  [{0}] {1}" -f $f.Tier, $f.Name) -ForegroundColor Red }
    exit 1
}
exit 0