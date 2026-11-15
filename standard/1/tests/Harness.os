// standard/1/tests/Harness.os
// Tiny self-contained test harness. Depends only on libc printf/fflush (no std modules),
// so it still works if the standard library is broken. `say` is the only place that prints.
extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fflush(f: void*) => int;

let passCount: int = 0;
let failCount: int = 0;
let currentSuite: char* = "";

// printf + fflush (same as foundation.os): output printed by JIT-compiled code is otherwise lost
// when the process exits before the C stdio buffer is flushed.
function say(msg: char*) => void { printf("%s\n", msg); fflush(nullptr); }

function suite(name: char*) => void {
    currentSuite = name;
    say("");
    say(name);
}

function check(cond: bool, name: char*) => void {
    if (cond) { passCount = passCount + 1; }
    else { failCount = failCount + 1; say("  FAIL: "); say(name); }
}

function eqInt(got: int, want: int, name: char*) => void { check(got == want, name); }
function eqF(got: double, want: double, eps: double, name: char*) => void {
    let d: double = got - want;
    if (d < 0.0) d = 0.0 - d;
    check(d <= eps, name);
}
function eqStr(got: char*, want: char*, name: char*) => void {
    // plain byte compare so the harness does not depend on std/string being correct
    let i: int = 0;
    let same: bool = true;
    let going: bool = true;
    while (going) {
        if (got[i] != want[i]) { same = false; going = false; }
        else if (got[i] == 0) { going = false; }
        i = i + 1;
    }
    check(same, name);
}
function notNull(p: void*, name: char*) => void { check(p != nullptr, name); }

// Call at the end of main(); return its result from main so CI sees the exit code.
function finish() => int {
    say("");
    if (failCount == 0) { say("ALL PASSED"); return 0; }
    say("SOME TESTS FAILED");
    return 1;
}
