// standard/1/Algo.os
// Tier 3: generic algorithms over T* + length (works with List.data / List.len).
// Comparators return <0, 0, >0.
module Core = import "standard/1/Core.os";

function sort<T>(a: T*, n: u64, cmp: fn(T*, T*) => int) => void {
    if (n < 2) return;
    // insertion sort for small, else heap-free quicksort (median of 3)
    if (n <= 16) {
        let i: u64 = 1;
        while (i < n) {
            let j: u64 = i;
            while (j > 0 && cmp(&a[j - 1], &a[j]) > 0) {
                Core.swapMem(&a[j - 1] as void*, &a[j] as void*, sizeof(T));
                j = j - 1;
            }
            i = i + 1;
        }
        return;
    }
    let mid: u64 = n / 2;
    if (cmp(&a[mid], &a[0]) < 0) Core.swapMem(&a[mid] as void*, &a[0] as void*, sizeof(T));
    if (cmp(&a[n - 1], &a[0]) < 0) Core.swapMem(&a[n - 1] as void*, &a[0] as void*, sizeof(T));
    if (cmp(&a[n - 1], &a[mid]) < 0) Core.swapMem(&a[n - 1] as void*, &a[mid] as void*, sizeof(T));
    Core.swapMem(&a[mid] as void*, &a[n - 2] as void*, sizeof(T));
    let pivot: T* = &a[n - 2];
    let i: u64 = 0;
    let j: u64 = n - 2;
    while (true) {
        i = i + 1;
        while (cmp(&a[i], pivot) < 0) i = i + 1;
        j = j - 1;
        while (j > 0 && cmp(&a[j], pivot) > 0) j = j - 1;
        if (i >= j) break;
        Core.swapMem(&a[i] as void*, &a[j] as void*, sizeof(T));
    }
    Core.swapMem(&a[i] as void*, &a[n - 2] as void*, sizeof(T));
    sort<T>(a, i, cmp);
    sort<T>(&a[i + 1], n - i - 1, cmp);
}

// Index of key in sorted array, or -1.
function binarySearch<T>(a: T*, n: u64, key: T*, cmp: fn(T*, T*) => int) => i64 {
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

function indexOf<T>(a: T*, n: u64, key: T*, eq: fn(T*, T*) => bool) => i64 {
    let i: u64 = 0;
    while (i < n) { if (eq(&a[i], key)) return i as i64; i = i + 1; }
    return -1;
}

function reverse<T>(a: T*, n: u64) => void {
    if (n < 2) return;
    let i: u64 = 0; let j: u64 = n - 1;
    while (i < j) { Core.swapMem(&a[i] as void*, &a[j] as void*, sizeof(T)); i = i + 1; j = j - 1; }
}

function forEach<T>(a: T*, n: u64, f: fn(T*) => void) => void {
    let i: u64 = 0;
    while (i < n) { f(&a[i]); i = i + 1; }
}

function count<T>(a: T*, n: u64, pred: fn(T*) => bool) => u64 {
    let c: u64 = 0; let i: u64 = 0;
    while (i < n) { if (pred(&a[i])) c = c + 1; i = i + 1; }
    return c;
}

function anyOf<T>(a: T*, n: u64, pred: fn(T*) => bool) => bool {
    let i: u64 = 0;
    while (i < n) { if (pred(&a[i])) return true; i = i + 1; }
    return false;
}

function allOf<T>(a: T*, n: u64, pred: fn(T*) => bool) => bool {
    let i: u64 = 0;
    while (i < n) { if (!pred(&a[i])) return false; i = i + 1; }
    return true;
}

// Min/max element pointer (nullptr if empty).
function minOf<T>(a: T*, n: u64, cmp: fn(T*, T*) => int) => T* {
    if (n == 0) return nullptr;
    let m: T* = &a[0]; let i: u64 = 1;
    while (i < n) { if (cmp(&a[i], m) < 0) m = &a[i]; i = i + 1; }
    return m;
}
function maxOf<T>(a: T*, n: u64, cmp: fn(T*, T*) => int) => T* {
    if (n == 0) return nullptr;
    let m: T* = &a[0]; let i: u64 = 1;
    while (i < n) { if (cmp(&a[i], m) > 0) m = &a[i]; i = i + 1; }
    return m;
}

function cmpI64(a: i64*, b: i64*) => int { if (*a < *b) return -1; if (*a > *b) return 1; return 0; }
function cmpU64(a: u64*, b: u64*) => int { if (*a < *b) return -1; if (*a > *b) return 1; return 0; }
function cmpF64(a: double*, b: double*) => int { if (*a < *b) return -1; if (*a > *b) return 1; return 0; }
