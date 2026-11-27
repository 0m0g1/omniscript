// bench_sieve: sieve of Eratosthenes; measures memory + tight loops. Known: primes <= 10^7 = 664579, <= 10^8 = 5761455.
// Run:  .\bin\Release-windows-x86_64\Osengine.exe bench\bench_sieve.os

module CMem = import "standard/1/c/mem.os";
module Console = import "standard/1/Console.os";

const LIMIT: i64 = 100000000;

function sieve(n: i64) => i64 {
    let bytes: usize = (n + 1) as usize;
    let s: u8* = CMem.malloc(bytes) as u8*;
    let k: i64 = 0;
    while (k <= n) { s[k] = 0; k = k + 1; }
    let count: i64 = 0;
    let i: i64 = 2;
    while (i <= n) {
        if (s[i] == 0) {
            count = count + 1;
            let j: i64 = i * i;
            while (j <= n) { s[j] = 1; j = j + i; }
        }
        i = i + 1;
    }
    CMem.free(s as void*);
    return count;
}

function expected(n: i64) => i64 {
    if (n == 10000000) { return 664579; }
    if (n == 100000000) { return 5761455; }
    return -1;
}

let got: i64 = sieve(LIMIT);
let want: i64 = expected(LIMIT);
if (want < 0) { Console.print("N/A   sieve  (no known value for this LIMIT)  primes="); }
else if (got == want) { Console.print("PASS  sieve  primes="); }
else { Console.print("FAIL  sieve  primes="); }
Console.println(got);
