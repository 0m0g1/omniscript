# from the project root:  .\docs\examples\syntax_probe\RUN_ALL.ps1   (adjust $dir / $exe if needed)
$exe = ".\bin\Release-windows-x86_64\Osengine.exe"
$dir = "docs\examples\syntax_probe"
Get-ChildItem "$dir\p*.os" | Sort-Object Name | ForEach-Object {
  "== $($_.Name)"
  & $exe $_.FullName 2>&1 | Where-Object { $_ -notmatch "Starting Omniscript|Engine execution completed|Application completed" } | ForEach-Object { "   $_" }
} | Tee-Object -FilePath syntax_probe_results.txt
