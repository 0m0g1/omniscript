// standard/1/Debug.os
// Tier 4: debug helpers: dbg macros-as-functions, hexdump, assertEq, trace, counters.
module Core = import "standard/1/Core.os";
module CStr = import "standard/1/c/string.os";

extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fprintf(f: void*, fmt: char*, ...) => int;
extern "C" fn __acrt_iob_func(index: uint) => void*;   // PLATFORM

let enabled: bool = true;
function setEnabled(on: bool) => void { enabled = on; }

function trace(file: char*, line: int, msg: char*) => void {
    if (!enabled) return;
    fprintf(__acrt_iob_func(2), "[trace] %s:%d %s\n", file, line, msg);
}
function dbgInt(label: char*, v: i64) => i64 { if (enabled) fprintf(__acrt_iob_func(2), "[dbg] %s = %lld\n", label, v); return v; }
function dbgF64(label: char*, v: double) => double { if (enabled) fprintf(__acrt_iob_func(2), "[dbg] %s = %g\n", label, v); return v; }
function dbgStr(label: char*, v: char*) => char* { if (enabled) fprintf(__acrt_iob_func(2), "[dbg] %s = \"%s\"\n", label, v); return v; }
function dbgPtr(label: char*, v: void*) => void* { if (enabled) fprintf(__acrt_iob_func(2), "[dbg] %s = %p\n", label, v); return v; }

function assertEqInt(a: i64, b: i64, msg: char*) => void {
    if (a != b) { fprintf(__acrt_iob_func(2), "assertEq failed: %lld != %lld\n", a, b); Core.panic(msg); }
}
function assertEqStr(a: char*, b: char*, msg: char*) => void {
    if (CStr.strcmp(a, b) != 0) { fprintf(__acrt_iob_func(2), "assertEq failed: \"%s\" != \"%s\"\n", a, b); Core.panic(msg); }
}

function hexdump(p: u8*, n: u64) => void {
    let i: u64 = 0;
    while (i < n) {
        printf("%08llx  ", i);
        let j: u64 = 0;
        while (j < 16) { if (i + j < n) printf("%02x ", p[i + j] as int); else printf("   "); j = j + 1; }
        printf(" |");
        j = 0;
        while (j < 16 && i + j < n) { let c: int = p[i + j] as int; if (c >= 32 && c < 127) printf("%c", c); else printf("."); j = j + 1; }
        printf("|\n");
        i = i + 16;
    }
}

// Simple named counters (up to 32) for quick instrumentation.
let counterNames: [32]char*;
let counterVals: [32]i64;
let counterN: int = 0;
function count(name: char*) => void {
    let i: int = 0;
    while (i < counterN) { if (CStr.strcmp(counterNames[i], name) == 0) { counterVals[i] = counterVals[i] + 1; return; } i = i + 1; }
    if (counterN < 32) { counterNames[counterN] = name; counterVals[counterN] = 1; counterN = counterN + 1; }
}
function dumpCounters() => void {
    let i: int = 0;
    while (i < counterN) { printf("%s: %lld\n", counterNames[i], counterVals[i]); i = i + 1; }
}
