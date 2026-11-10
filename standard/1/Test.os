// standard/1/Test.os
// Tiny test harness. Never panics: every failure prints one line and the run continues.
//   FAIL [Module.function] what was checked        <- grep FAIL to triage
// Usage:  Test.check(cond, "Random.range", "stays in [5,10)");   ...   return Test.summary();

extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn strcmp(a: char*, b: char*) => int;

let g_pass: int = 0;
let g_fail: int = 0;

function check(cond: bool, tag: char*, what: char*) => void {
    if (cond) {
        g_pass = g_pass + 1;
    } else {
        g_fail = g_fail + 1;
        printf("FAIL [%s] %s\n", tag, what);
        fflush(nullptr);
    }
}

function eqI64(got: i64, want: i64, tag: char*, what: char*) => void {
    if (got == want) {
        g_pass = g_pass + 1;
    } else {
        g_fail = g_fail + 1;
        printf("FAIL [%s] %s (got %lld, want %lld)\n", tag, what, got, want);
        fflush(nullptr);
    }
}

function eqF64(got: double, want: double, eps: double, tag: char*, what: char*) => void {
    let d = got - want;
    if (d < 0.0) d = 0.0 - d;
    if (d <= eps) {
        g_pass = g_pass + 1;
    } else {
        g_fail = g_fail + 1;
        printf("FAIL [%s] %s (got %f, want %f)\n", tag, what, got, want);
        fflush(nullptr);
    }
}

function eqStr(got: char*, want: char*, tag: char*, what: char*) => void {
    if (strcmp(got, want) == 0) {
        g_pass = g_pass + 1;
    } else {
        g_fail = g_fail + 1;
        printf("FAIL [%s] %s (got \"%s\", want \"%s\")\n", tag, what, got, want);
        fflush(nullptr);
    }
}

function summary() => i32 {
    printf("%d passed, %d failed\n", g_pass, g_fail);
    fflush(nullptr);
    if (g_fail > 0) return 1;
    return 0;
}
