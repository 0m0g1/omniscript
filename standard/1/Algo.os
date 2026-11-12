// standard/1/Algo.os
// Tier 3: generic algorithms over T* + length (works with List.data / List.len).
// Comparators return <0, 0, >0.
//
// Generics: the compiler does eager monomorphization — every generic function
// must declare `<T extends ...>` with the exhaustive set of types it instantiates
// for. Unconstrained `<T>` is a compile error.
//
// Pointer casts: `(&x) as void*` (parenthesized) is required. Bare `&x as void*`
// parses as `&(x as void*)` — taking the address of a temporary cast, which fails.

include "standard/1/Core.os";

function sort<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64, cmp: fn(T*, T*) => int) => void {
    if (n < 2) return;
    // Insertion sort for small, else heap-free quicksort (median of 3).
    if (n <= 16) {
        let i: u64 = 1;
        while (i < n) {
            let j: u64 = i;
            while (j > 0 && cmp(&a[j - 1], &a[j]) > 0) {
                swapMem((&a[j - 1]) as void*, (&a[j]) as void*, sizeof(T));
                j = j - 1;
            }
            i = i + 1;
        }
        return;
    }
    let mid: u64 = n / 2;
    if (cmp(&a[mid], &a[0]) < 0) swapMem((&a[mid]) as void*, (&a[0]) as void*, sizeof(T));
    if (cmp(&a[n - 1], &a[0]) < 0) swapMem((&a[n - 1]) as void*, (&a[0]) as void*, sizeof(T));
    if (cmp(&a[n - 1], &a[mid]) < 0) swapMem((&a[n - 1]) as void*, (&a[mid]) as void*, sizeof(T));
    swapMem((&a[mid]) as void*, (&a[n - 2]) as void*, sizeof(T));
    let pivot: T* = &a[n - 2];
    let i: u64 = 0;
    let j: u64 = n - 2;
    while (true) {
        i = i + 1;
        while (cmp(&a[i], pivot) < 0) i = i + 1;
        j = j - 1;
        while (j > 0 && cmp(&a[j], pivot) > 0) j = j - 1;
        if (i >= j) break;
        swapMem((&a[i]) as void*, (&a[j]) as void*, sizeof(T));
    }
    swapMem((&a[i]) as void*, (&a[n - 2]) as void*, sizeof(T));
    sort<T>(a, i, cmp);
    sort<T>(&a[i + 1], n - i - 1, cmp);
}

// Index of key in sorted array, or -1.
function binarySearch<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64, key: T*, cmp: fn(T*, T*) => int) => i64 {
    let lo: i64 = 0;
    let hi: i64 = n as i64 - 1;
    while (lo <= hi) {
        let m: i64 = lo + (hi - lo) / 2;
        let c: int = cmp(&a[m], key);
        if (c == 0) return m;
        if (c < 0) lo = m + 1; else hi = m - 1;
    }
    return -1;
}

function indexOf<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64, key: T*, eq: fn(T*, T*) => bool) => i64 {
    let i: u64 = 0;
    while (i < n) { if (eq(&a[i], key)) return i as i64; i = i + 1; }
    return -1;
}

function reverse<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64) => void {
    if (n < 2) return;
    let i: u64 = 0;
    let j: u64 = n - 1;
    while (i < j) {
        swapMem((&a[i]) as void*, (&a[j]) as void*, sizeof(T));
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
    while (i < n) { if (pred(&a[i])) c = c + 1; i = i + 1; }
    return c;
}

function anyOf<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64, pred: fn(T*) => bool) => bool {
    let i: u64 = 0;
    while (i < n) { if (pred(&a[i])) return true; i = i + 1; }
    return false;
}

function allOf<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64, pred: fn(T*) => bool) => bool {
    let i: u64 = 0;
    while (i < n) { if (!pred(&a[i])) return false; i = i + 1; }
    return true;
}

// Min/max element pointer (nullptr if empty).
function minOf<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64, cmp: fn(T*, T*) => int) => T* {
    if (n == 0) return nullptr;
    let m: T* = &a[0];
    let i: u64 = 1;
    while (i < n) { if (cmp(&a[i], m) < 0) m = &a[i]; i = i + 1; }
    return m;
}

function maxOf<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64, cmp: fn(T*, T*) => int) => T* {
    if (n == 0) return nullptr;
    let m: T* = &a[0];
    let i: u64 = 1;
    while (i < n) { if (cmp(&a[i], m) > 0) m = &a[i]; i = i + 1; }
    return m;
}

// ---- Concrete comparators for the common types ----

function cmpI64(a: i64*, b: i64*) => int { if (*a < *b) return -1; if (*a > *b) return 1; return 0; }
function cmpU64(a: u64*, b: u64*) => int { if (*a < *b) return -1; if (*a > *b) return 1; return 0; }
function cmpF64(a: double*, b: double*) => int { if (*a < *b) return -1; if (*a > *b) return 1; return 0; }
function cmpI32(a: i32*, b: i32*) => int { if (*a < *b) return -1; if (*a > *b) return 1; return 0; }
function cmpU32(a: u32*, b: u32*) => int { if (*a < *b) return -1; if (*a > *b) return 1; return 0; }
function cmpF32(a: f32*, b: f32*) => int { if (*a < *b) return -1; if (*a > *b) return 1; return 0; }