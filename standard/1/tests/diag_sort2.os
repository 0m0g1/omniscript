// standard/1/tests/diag_sort2.os
// Same quicksort as Algo.sort, specialised by hand to i32 (no generics).
//   A: keeps the cmp function-pointer parameter
//   B: compares directly with < and > (no function pointer at all)
// Headers print before each case; the last header before a crash names the case.
module H = import "standard/1/tests/Harness.os";
module L = import "standard/1/List.os";

function cmp32(a: i32*, b: i32*) => int {
    if (*a < *b) return -1;
    if (*a > *b) return 1;
    return 0;
}

function isSorted(a: i32*, n: u64) => bool {
    let k: u64 = 1;
    while (k < n) {
        if (a[k - 1] > a[k]) return false;
        k = k + 1;
    }
    return true;
}

function sortA(a: i32*, n: u64, cmp: fn(i32*, i32*) => int) => void {
    if (n < 2) return;
    if (n <= 16) {
        let ii: u64 = 1;
        while (ii < n) {
            let jj: u64 = ii;
            while (jj > 0 && cmp(&a[jj - 1], &a[jj]) > 0) {
                let t0: i32 = a[jj - 1];
                a[jj - 1] = a[jj];
                a[jj] = t0;
                jj = jj - 1;
            }
            ii = ii + 1;
        }
        return;
    }
    let mid: u64 = n / 2;
    if (cmp(&a[mid], &a[0]) < 0) {
        let t1: i32 = a[mid]; a[mid] = a[0]; a[0] = t1;
    }
    if (cmp(&a[n - 1], &a[0]) < 0) {
        let t2: i32 = a[n - 1]; a[n - 1] = a[0]; a[0] = t2;
    }
    if (cmp(&a[n - 1], &a[mid]) < 0) {
        let t3: i32 = a[n - 1]; a[n - 1] = a[mid]; a[mid] = t3;
    }
    let t4: i32 = a[mid]; a[mid] = a[n - 2]; a[n - 2] = t4;
    let pivot: i32 = a[n - 2];
    let lo: u64 = 0;
    let hi: u64 = n - 2;
    let done: bool = false;
    while (!done) {
        lo = lo + 1;
        while (cmp(&a[lo], &pivot) < 0) lo = lo + 1;
        hi = hi - 1;
        while (hi > 0 && cmp(&a[hi], &pivot) > 0) hi = hi - 1;
        if (lo >= hi) {
            done = true;
        } else {
            let t5: i32 = a[lo]; a[lo] = a[hi]; a[hi] = t5;
        }
    }
    let t6: i32 = a[lo]; a[lo] = a[n - 2]; a[n - 2] = t6;
    sortA(a, lo, cmp);
    sortA(&a[lo + 1], n - lo - 1, cmp);
}

function sortB(a: i32*, n: u64) => void {
    if (n < 2) return;
    if (n <= 16) {
        let ii: u64 = 1;
        while (ii < n) {
            let jj: u64 = ii;
            while (jj > 0 && a[jj - 1] > a[jj]) {
                let t0: i32 = a[jj - 1];
                a[jj - 1] = a[jj];
                a[jj] = t0;
                jj = jj - 1;
            }
            ii = ii + 1;
        }
        return;
    }
    let mid: u64 = n / 2;
    if (a[mid] < a[0]) {
        let t1: i32 = a[mid]; a[mid] = a[0]; a[0] = t1;
    }
    if (a[n - 1] < a[0]) {
        let t2: i32 = a[n - 1]; a[n - 1] = a[0]; a[0] = t2;
    }
    if (a[n - 1] < a[mid]) {
        let t3: i32 = a[n - 1]; a[n - 1] = a[mid]; a[mid] = t3;
    }
    let t4: i32 = a[mid]; a[mid] = a[n - 2]; a[n - 2] = t4;
    let pivot: i32 = a[n - 2];
    let lo: u64 = 0;
    let hi: u64 = n - 2;
    let done: bool = false;
    while (!done) {
        lo = lo + 1;
        while (a[lo] < pivot) { lo = lo + 1; }
        hi = hi - 1;
        while (hi > 0 && a[hi] > pivot) { hi = hi - 1; }
        if (lo >= hi) {
            done = true;
        } else {
            let t5: i32 = a[lo]; a[lo] = a[hi]; a[hi] = t5;
        }
    }
    let t6: i32 = a[lo]; a[lo] = a[n - 2]; a[n - 2] = t6;
    sortB(a, lo);
    sortB(&a[lo + 1], n - lo - 1);
}

function runA(count: int, mul: int, md: int) => bool {
    let a: L.List<i32>;
    let i: int = 0;
    while (i < count) { a.push((i * mul) % md); i = i + 1; }
    sortA(a.data, a.len, cmp32);
    let ok: bool = isSorted(a.data, a.len);
    a.free();
    return ok;
}

function runB(count: int, mul: int, md: int) => bool {
    let a: L.List<i32>;
    let i: int = 0;
    while (i < count) { a.push((i * mul) % md); i = i + 1; }
    sortB(a.data, a.len);
    let ok: bool = isSorted(a.data, a.len);
    a.free();
    return ok;
}

function main() => int {
    H.suite("B1: no fn pointer, 17 distinct");
    H.check(runB(17, 7919, 1009), "B 17");
    H.suite("B2: no fn pointer, 500 with duplicates");
    H.check(runB(500, 7919, 101), "B 500 dup");

    H.suite("A1: fn pointer param, 17 distinct");
    H.check(runA(17, 7919, 1009), "A 17");
    H.suite("A2: fn pointer param, 500 with duplicates");
    H.check(runA(500, 7919, 101), "A 500 dup");

    return H.finish();
}
