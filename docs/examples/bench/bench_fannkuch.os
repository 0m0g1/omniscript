// docs/examples/bench/bench_fannkuch.os
// bench_fannkuch: fannkuch-redux (CLBG); integer array shuffling.
// Run:  .\bin\Release-windows-x86_64\Osengine.exe bench\bench_fannkuch.os
// Known (CLBG): N=7 -> checksum 228, max flips 16 ; N=9 -> 8629 / 30 ; N=10 -> 73196 / 38 ; N=11 -> 556355 / 51 ; N=12 -> 3968050 / 65.

module CMem = import "standard/1/c/mem.os";
module Console = import "standard/1/Console.os";

const N: int = 11;

// returns the max flip count; the checksum is stored in out[0]
function fannkuch(n: int, out: i64*) => int {
    let bytes: usize = 128;
    let p: int* = CMem.malloc(bytes) as int*;
    let q: int* = CMem.malloc(bytes) as int*;
    let s: int* = CMem.malloc(bytes) as int*;
    for (let i: int = 0; i < n; i = i + 1) { p[i] = i; q[i] = i; s[i] = i; }

    let sign: int = 1;
    let sum: i64 = 0;
    let maxf: int = 0;
    let running: int = 1;
    while (running == 1) {
        let q0: int = p[0];
        if (q0 != 0) {
            for (let i: int = 1; i < n; i = i + 1) { q[i] = p[i]; }
            let flips: int = 1;
            let inner: int = 1;
            while (inner == 1) {
                let qq: int = q[q0];
                if (qq == 0) {
                    sum = sum + ((sign * flips) as i64);
                    if (flips > maxf) { maxf = flips; }
                    inner = 0;
                } else {
                    q[q0] = q0;
                    if (q0 >= 3) {
                        let lo: int = 1;
                        let hi: int = q0 - 1;
                        while (lo < hi) {
                            let t: int = q[lo];
                            q[lo] = q[hi];
                            q[hi] = t;
                            lo = lo + 1;
                            hi = hi - 1;
                        }
                    }
                    q0 = qq;
                    flips = flips + 1;
                }
            }
        }

        if (sign == 1) {
            let t: int = p[1];
            p[1] = p[0];
            p[0] = t;
            sign = -1;
        } else {
            let t: int = p[1];
            p[1] = p[2];
            p[2] = t;
            sign = 1;
            let i: int = 2;
            let searching: int = 1;
            while (searching == 1) {
                let sx: int = s[i];
                if (sx != 0) {
                    s[i] = sx - 1;
                    searching = 0;
                } else {
                    if (i == n - 1) {
                        searching = 0;
                        running = 0;
                    } else {
                        s[i] = i;
                        let first: int = p[0];
                        for (let k: int = 0; k <= i; k = k + 1) { p[k] = p[k + 1]; }
                        p[i + 1] = first;
                        i = i + 1;
                    }
                }
            }
        }
    }
    out[0] = sum;
    CMem.free(p as void*); CMem.free(q as void*); CMem.free(s as void*);
    return maxf;
}

let obytes: usize = 8;
let out: i64* = CMem.malloc(obytes) as i64*;
let flipsMax: int = fannkuch(N, out);
let checksum: i64 = out[0];
let flipsMax64: i64 = flipsMax as i64;

let wantSum: i64 = -1;
let wantFlips: i64 = -1;
if (N == 7)  { wantSum = 228;     wantFlips = 16; }
if (N == 9)  { wantSum = 8629;    wantFlips = 30; }
if (N == 10) { wantSum = 73196;   wantFlips = 38; }
if (N == 11) { wantSum = 556355;  wantFlips = 51; }
if (N == 12) { wantSum = 3968050; wantFlips = 65; }

if (wantSum < 0) { Console.print("N/A   fannkuch  (no known value for this N)  checksum="); }
else if (checksum == wantSum && flipsMax64 == wantFlips) { Console.print("PASS  fannkuch  checksum="); }
else { Console.print("FAIL  fannkuch  checksum="); }
Console.print(checksum);
Console.print("  maxflips=");
Console.println(flipsMax64);
CMem.free(out as void*);
