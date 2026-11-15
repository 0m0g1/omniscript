// Isolates why `*out = x` does not reach the caller's variable (List.get/pop out-params).
// Here a FAIL means that case is BROKEN: the check states what should be true.
module H = import "standard/1/tests/Harness.os";

function wr(out: i32*) => void { *out = 7; }
function wr_idx(out: i32*) => void { out[0] = 8; }
function wr_val(out: i32*, x: i32) => void { *out = x; }
function wr_from(out: i32*, src: i32*) => void { *out = src[1]; }
function wr_bool(out: i32*) => bool { *out = 6; return true; }
function put<T extends i32 | i64>(out: T*, x: T) => void { *out = x; }

function main() => int {
    H.suite("P1 *out = 7 through a parameter");
    let a: i32 = -1;
    wr(&a);
    H.check(a == 7, "P1 a == 7 after wr(&a)");

    H.suite("P2 out[0] = 8 through a parameter");
    let b: i32 = -1;
    wr_idx(&b);
    H.check(b == 8, "P2 b == 8 after wr_idx(&b)");

    H.suite("P3 *out = x (value parameter)");
    let c: i32 = -1;
    wr_val(&c, 9);
    H.check(c == 9, "P3 c == 9 after wr_val(&c, 9)");

    H.suite("P4 *out = src[1] (what List.get does)");
    let arr: i32[3] = [10, 20, 30];
    let d: i32 = -1;
    wr_from(&d, &arr[0]);
    H.check(d == 20, "P4 d == 20 after wr_from");

    H.suite("P5 write inside an `if` on the call result (List test style)");
    let e: i32 = -1;
    H.check(wr_bool(&e), "P5 call returns true");
    H.check(e == 6, "P5 e == 6 after wr_bool(&e)");

    H.suite("P6 pointer local: let p = &f; *p = 5");
    let f: i32 = -1;
    let p: i32* = &f;
    *p = 5;
    H.check(f == 5, "P6 f == 5 after *p = 5");
    H.check(*p == 5, "P6 *p == 5");

    H.suite("P7 generic function out-param");
    let g: i32 = -1;
    put<i32>(&g, 11);
    H.check(g == 11, "P7 g == 11 after put<i32>(&g, 11)");

    H.suite("P8 does the memory change even if the variable read does not?");
    let h: i32 = -1;
    let hp: i32* = &h;
    wr(hp);
    H.check(*hp == 7, "P8 *hp == 7 (memory written)");
    H.check(h == 7, "P8 h == 7 (variable read sees it)");

    return H.finish();
}
