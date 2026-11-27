// bench_fib: recursive Fibonaccio; measures call overhead. Known: fib(40) = 102334155, fib(35) = 9227465.
// Run:  .\bin\Release-windows-x86_64\Osengine.exe bench\bench_fib.os

module Console = import "standard/1/Console.os";

const N: int = 40;

function fib(n: int) => int {
    if (n < 2) { return n; }
    return fib(n - 1) + fib(n - 2);
}

function expected(n: int) => i64 {
    if (n == 40) { return 102334155; }
    if (n == 35) { return 9227465; }
    return -1;
}

let got: i64 = fib(N) as i64;
let want: i64 = expected(N);
if (want < 0) { Console.print("N/A   fib  (no known value for this N)  result="); }
else if (got == want) { Console.print("PASS  fib  result="); }
else { Console.print("FAIL  fib  result="); }
Console.println(got);
