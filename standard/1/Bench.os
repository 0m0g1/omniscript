// standard/1/Bench.os
// Tier 4: timing helpers. Stopwatch + bench(name, iterations, fn).
module Str = import "standard/1/String.os";

extern "C" fn clock() => i64;                                  // CLOCKS_PER_SEC = 1000 on Windows
extern "C" fn QueryPerformanceCounter(v: i64*) => int;
extern "C" fn QueryPerformanceFrequency(v: i64*) => int;       // PLATFORM: Win32
extern "C" fn printf(fmt: char*, ...) => int;

function nowNs() => i64 {
    let c: i64 = 0; let f: i64 = 0;
    QueryPerformanceCounter(&c); QueryPerformanceFrequency(&f);
    return (c / f) * 1000000000 + ((c % f) * 1000000000) / f;
}

struct Stopwatch {
    start: i64 = 0;
    begin() => void { this.start = nowNs(); }
    elapsedNs() => i64 { return nowNs() - this.start; }
    elapsedMs() => double { return (this.elapsedNs() as double) / 1000000.0; }
    elapsedSec() => double { return (this.elapsedNs() as double) / 1000000000.0; }
}

// Runs f iterations times, prints "name: total ms (per-iter ns)". Returns ns per iteration.
function bench(name: char*, iterations: u64, f: fn() => void) => i64 {
    let sw: Stopwatch; sw.begin();
    let i: u64 = 0;
    while (i < iterations) { f(); i = i + 1; }
    let ns: i64 = sw.elapsedNs();
    let per: i64 = ns / (iterations as i64);
    printf("%s: %.3f ms total, %lld ns/iter\n", name, (ns as double) / 1000000.0, per);
    return per;
}
