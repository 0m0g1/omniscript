// Why does Map.find reach eqFn on an empty map? Tests: return-inside-if-inside-while, && short-circuit,
// each as a plain function and as a method. A FAIL says what is broken.
module H = import "standard/1/tests/Harness.os";

function bump(c: i32*) => bool { *c = *c + 1; return true; }

function ret_in_while(a: u8*, c: i32*) => i64 {
    let i: u64 = 0;
    let n: u64 = 0;
    while (n < 16) {
        if (a[i] == 0) return -1;
        bump(c);                  // must NOT run: the return above exits first
        i = i + 1;
        n = n + 1;
    }
    return -2;
}

function and_sc(a: u8*, c: i32*) => bool {
    if (a[0] == 1 && bump(c)) return true;   // a[0] is 0, so bump must NOT run
    return false;
}

function find_like(a: u8*, c: i32*) => i64 {
    let i: u64 = 0;
    let n: u64 = 0;
    while (n < 16) {
        if (a[i] == 0) return -1;
        if (a[i] == 1 && bump(c)) return i as i64;
        i = (i + 1) % 16;
        n = n + 1;
    }
    return -1;
}

struct W {
    st: u8* = nullptr;
    ret_in_while_m(c: i32*) => i64 {
        let i: u64 = 0;
        let n: u64 = 0;
        while (n < 16) {
            if (this.st[i] == 0) return -1;
            bump(c);
            i = i + 1;
            n = n + 1;
        }
        return -2;
    }
    find_like_m(c: i32*) => i64 {
        let i: u64 = 0;
        let n: u64 = 0;
        while (n < 16) {
            if (this.st[i] == 0) return -1;
            if (this.st[i] == 1 && bump(c)) return i as i64;
            i = (i + 1) % 16;
            n = n + 1;
        }
        return -1;
    }
}

function main() => int {
    let a: [16]u8;
    let k: u64 = 0;
    while (k < 16) { a[k] = 0 as u8; k = k + 1; }
    let c: i32 = 0;

    H.suite("R1 return inside if inside while (function)");
    let r1 = ret_in_while(&a[0], &c);
    H.check(r1 == -1, "R1 returns -1");
    H.check(c == 0, "R1 code after the return did not run");

    H.suite("R2 && short-circuit (function)");
    c = 0;
    and_sc(&a[0], &c);
    H.check(c == 0, "R2 right side of && not evaluated when left is false");

    H.suite("R3 find-shaped loop (function)");
    c = 0;
    H.check(find_like(&a[0], &c) == -1, "R3 returns -1");
    H.check(c == 0, "R3 bump never ran");

    H.suite("R4 return inside while (method through this)");
    c = 0;
    let w: W;
    w.st = &a[0];
    H.check(w.ret_in_while_m(&c) == -1, "R4 returns -1");
    H.check(c == 0, "R4 code after the return did not run");

    H.suite("R5 find-shaped loop (method through this)");
    c = 0;
    H.check(w.find_like_m(&c) == -1, "R5 returns -1");
    H.check(c == 0, "R5 bump never ran");

    return H.finish();
}
