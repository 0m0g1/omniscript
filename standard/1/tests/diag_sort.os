// standard/1/tests/diag_sort.os
// Isolates the quicksort path (n > 16) of Algo.sort from the rest of test_collections.
// Each case prints a header first, so the last header printed before a crash names the case.
module H = import "standard/1/tests/Harness.os";
module L = import "standard/1/List.os";
include "standard/1/Algo.os";

function isSorted(a: i32*, n: u64) => bool {
    let k: u64 = 1;
    while (k < n) {
        if (a[k - 1] > a[k]) return false;
        k = k + 1;
    }
    return true;
}

// fill with (i * mul) % mod, push into a fresh list, sort, check order
function run(count: int, mul: int, md: int) => bool {
    let a: L.List<i32>;
    let i: int = 0;
    while (i < count) {
        a.push((i * mul) % md);
        i = i + 1;
    }
    sort<i32>(a.data, a.len, cmpI32);
    let ok: bool = isSorted(a.data, a.len);
    a.free();
    return ok;
}

function main() => int {
    H.suite("A: 17 distinct (smallest quicksort case)");
    H.check(run(17, 7919, 1009), "17 distinct");

    H.suite("B: 100 distinct");
    H.check(run(100, 7919, 1009), "100 distinct");

    H.suite("C: 500 distinct");
    H.check(run(500, 7919, 100003), "500 distinct");

    H.suite("D: 500 with duplicates (mod 101)");
    H.check(run(500, 7919, 101), "500 mod 101");

    H.suite("E: 500 all equal");
    H.check(run(500, 0, 101), "500 equal");

    return H.finish();
}
