// standard/1/Core.os
include "standard/1/c/console.os";
include "standard/1/c/os.os";

function panic(msg: char*) => void {
    fflush(stdout);
    fputs("panic: ", stderr);
    fputs(msg, stderr);
    fputs("\n", stderr);
    fflush(stderr);
    exit(1);
}

function assert(cond: bool, msg: char*) => void {
    if (!cond) {
        fflush(stdout);
        fputs("assertion failed: ", stderr);
        fputs(msg, stderr);
        fputs("\n", stderr);
        fflush(stderr);
        exit(1);
    }
}

// Inlined (does not call panic) as a workaround for the 'panic' lookup failure.
function unreachable() => void {
    fflush(stdout);
    fputs("panic: unreachable code reached\n", stderr);
    fflush(stderr);
    exit(1);
}

// ---- integers ----
function minI64(a: i64, b: i64) => i64 { if (a < b) return a; return b; }
function maxI64(a: i64, b: i64) => i64 { if (a > b) return a; return b; }
function clampI64(v: i64, lo: i64, hi: i64) => i64 {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
function absI64(v: i64) => i64 { if (v < 0) return 0 - v; return v; }

// ---- floating point (NaN handling: NaN in -> the other argument out for min/max) ----
function minF64(a: double, b: double) => double { if (a < b) return a; return b; }
function maxF64(a: double, b: double) => double { if (a > b) return a; return b; }
function clampF64(v: double, lo: double, hi: double) => double {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// ---- raw memory ----
// Swaps n bytes between a and b. Regions must not overlap.
function swapMem(a: void*, b: void*, n: u64) => void {
    let pa = a as char*;
    let pb = b as char*;
    for (let i: u64 = 0; i < n; i++) {
        let t = pa[i];
        pa[i] = pb[i];
        pb[i] = t;
    }
}
