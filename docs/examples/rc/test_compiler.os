/// Regression tests for the compiler fixes: array-to-pointer decay, unsigned widening,
/// the declared type of `let`, and case-sensitive keywords.

module Console = import "standard/1/Console.os";

let fails: int = 0;

function check(name: char*, ok: bool) => void {
    if (ok) {
        Console.print("PASS  ");
    } else {
        Console.print("FAIL  ");
        fails = fails + 1;
    }
    Console.println(name);
}

let A: u8[4];

// ---- array to pointer: return, call argument, `as` cast; global and local ----
function retGlobal() => u8* { return A; }
function setSecond(p: u8*) => void { p[1] = 77 as u8; }

// ---- unsigned widening through a parameter and a return value ----
function widen(v: int) => int { return v; }
function retElem(i: int) => int { return A[i]; }

function main() => int {
    // decay through a call argument must alias the array, not copy it
    A[1] = 0 as u8;
    setSecond(A);
    check("global array passed as u8* writes through", A[1] == 77 as u8);

    let L: u8[4];
    L[1] = 0 as u8;
    setSecond(L);
    check("local array passed as u8* writes through", L[1] == 77 as u8);

    // decay through return
    let rp: u8* = retGlobal();
    rp[3] = 9 as u8;
    check("array returned as u8* aliases the array", A[3] == 9 as u8);

    // decay through `as`
    A[2] = 65 as u8;
    let q: char* = A as char*;
    check("global array `as char*` aliases the array", q[2] == 65 as char);
    let lq: char* = L as char*;
    L[2] = 66 as u8;
    check("local array `as char*` aliases the array", lq[2] == 66 as char);

    // `let` keeps its declared type (was kept as u8 before)
    A[0] = 200 as u8;
    let x: int = A[0];
    check("let x: int = u8 element keeps 200 (not -56)", x == 200);
    let y: int = x * 16;
    check("x * 16 is 3200 (144 would mean x stayed u8)", y == 3200);

    let t: int = 300;
    let r: int = 16;
    let z: int = (t & 0xFE) + (r >> 3);
    check("(t & 0xFE) + (r >> 3) is 46", z == 46);

    // the exact pattern that failed in gb.os
    let tile: int = A[0];
    tile = (tile & 0xFE) + (r >> 3);
    check("tile = (tile & 0xFE) + (row >> 3) compiles and is 202", tile == 202);

    // reassignment widens unsigned too
    x = 0;
    x = A[0];
    check("reassignment x = u8 element is 200", x == 200);

    // call argument and return value widen unsigned
    check("u8 element passed to an int parameter is 200", widen(A[0]) == 200);
    check("u8 element returned from an int function is 200", retElem(0) == 200);

    // case-sensitive keywords: only lowercase `fn` etc. are keywords
    let Fn: int = 1;
    let FOR: int = 2;
    let FN: int = 3;
    check("Fn, FOR and FN are ordinary names", Fn + FOR + FN == 6);

    if (fails == 0) { Console.println("ALL PASS"); }
    return fails;
}
