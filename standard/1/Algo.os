// standard/1/Algo.os
// Tier 3: generic algorithms over T* + length (works with List.data / List.len).
// Comparators return <0, 0, >0.
//
// Generics: the compiler does eager monomorphization - every generic function
// must declare `<T extends ...>` with the exhaustive set of types it instantiates
// for. Unconstrained `<T>` is a compile error.
//
// Implementation notes (deliberately conservative codegen):
//  - elements are swapped through a typed temporary (`let t: T = ...`), not swapMem/sizeof(T)
//  - no local name is declared twice in one function
//  - loops exit through a flag instead of `while (true)` + `break`
//  - binarySearch uses u64 bounds only (no signed/unsigned mixing)

include "standard/1/Core.os";

function sort<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64, cmp: fn(T*, T*) => int) => void {
    if (n < 2) return;

    // Insertion sort for small inputs.
    if (n <= 16) {
        let ii: u64 = 1;
        while (ii < n) {
            let jj: u64 = ii;
            while (jj > 0 && cmp(&a[jj - 1], &a[jj]) > 0) {
                let t0: T = a[jj - 1];
                a[jj - 1] = a[jj];
                a[jj] = t0;
                jj = jj - 1;
            }
            ii = ii + 1;
        }
        return;
    }

    // Quicksort, median of 3. After this block a[0] <= a[mid] <= a[n-1].
    let mid: u64 = n / 2;
    if (cmp(&a[mid], &a[0]) < 0) {
        let t1: T = a[mid]; a[mid] = a[0]; a[0] = t1;
    }
    if (cmp(&a[n - 1], &a[0]) < 0) {
        let t2: T = a[n - 1]; a[n - 1] = a[0]; a[0] = t2;
    }
    if (cmp(&a[n - 1], &a[mid]) < 0) {
        let t3: T = a[n - 1]; a[n - 1] = a[mid]; a[mid] = t3;
    }

    // Park the pivot at n-2. Keep a copy of its value so no pointer into the array is held.
    let t4: T = a[mid]; a[mid] = a[n - 2]; a[n - 2] = t4;
    let pivot: T = a[n - 2];

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
            let t5: T = a[lo]; a[lo] = a[hi]; a[hi] = t5;
        }
    }
    let t6: T = a[lo]; a[lo] = a[n - 2]; a[n - 2] = t6;

    sort<T>(a, lo, cmp);
    sort<T>(&a[lo + 1], n - lo - 1, cmp);
}

// Index of key in sorted array, or -1.
function binarySearch<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64, key: T*, cmp: fn(T*, T*) => int) => i64 {
    let lo: u64 = 0;
    let hi: u64 = n;            // search window is [lo, hi)
    while (lo < hi) {
        let m: u64 = lo + (hi - lo) / 2;
        let c: int = cmp(&a[m], key);
        if (c == 0) return m as i64;
        if (c < 0) { lo = m + 1; } else { hi = m; }
    }
    return -1;
}

function indexOf<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64, key: T*, eq: fn(T*, T*) => bool) => i64 {
    let i: u64 = 0;
    while (i < n) {
        if (eq(&a[i], key)) return i as i64;
        i = i + 1;
    }
    return -1;
}

function reverse<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64) => void {
    if (n < 2) return;
    let i: u64 = 0;
    let j: u64 = n - 1;
    while (i < j) {
        let t: T = a[i];
        a[i] = a[j];
        a[j] = t;
        i = i + 1;
        j = j - 1;
    }
}

function forEach<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64, f: fn(T*) => void) => void {
    let i: u64 = 0;
    while (i < n) { f(&a[i]); i = i + 1; }
}

function count<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64, pred: fn(T*) => bool) => u64 {
    let c: u64 = 0;
    let i: u64 = 0;
    while (i < n) {
        if (pred(&a[i])) c = c + 1;
        i = i + 1;
    }
    return c;
}

function anyOf<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64, pred: fn(T*) => bool) => bool {
    let i: u64 = 0;
    while (i < n) {
        if (pred(&a[i])) return true;
        i = i + 1;
    }
    return false;
}

function allOf<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64, pred: fn(T*) => bool) => bool {
    let i: u64 = 0;
    while (i < n) {
        if (!pred(&a[i])) return false;
        i = i + 1;
    }
    return true;
}

// Min/max element pointer (nullptr if empty).
function minOf<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64, cmp: fn(T*, T*) => int) => T* {
    if (n == 0) return nullptr;
    let m: T* = &a[0];
    let i: u64 = 1;
    while (i < n) {
        if (cmp(&a[i], m) < 0) m = &a[i];
        i = i + 1;
    }
    return m;
}

function maxOf<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64, cmp: fn(T*, T*) => int) => T* {
    if (n == 0) return nullptr;
    let m: T* = &a[0];
    let i: u64 = 1;
    while (i < n) {
        if (cmp(&a[i], m) > 0) m = &a[i];
        i = i + 1;
    }
    return m;
}

// ---- Concrete comparators for the common types ----

function cmpI64(a: i64*, b: i64*) => int { if (*a < *b) return -1; if (*a > *b) return 1; return 0; }
function cmpU64(a: u64*, b: u64*) => int { if (*a < *b) return -1; if (*a > *b) return 1; return 0; }
function cmpF64(a: double*, b: double*) => int { if (*a < *b) return -1; if (*a > *b) return 1; return 0; }
function cmpI32(a: i32*, b: i32*) => int { if (*a < *b) return -1; if (*a > *b) return 1; return 0; }
function cmpU32(a: u32*, b: u32*) => int { if (*a < *b) return -1; if (*a > *b) return 1; return 0; }
function cmpF32(a: f32*, b: f32*) => int { if (*a < *b) return -1; if (*a > *b) return 1; return 0; }
