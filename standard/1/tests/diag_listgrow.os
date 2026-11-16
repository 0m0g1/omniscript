// standard/1/tests/diag_listgrow.os
// Does List<i32> keep its data after growing past the first realloc? Does sizeof(T) in a
// generic method give 4 for i32?
module H = import "standard/1/tests/Harness.os";
module L = import "standard/1/List.os";

function szOf<T extends i32 | i64>() => u64 {
    return sizeof(T);
}

function main() => int {
    H.suite("sizeof");
    H.eqInt(sizeof(i32) as int, 4, "sizeof(i32) direct");
    H.eqInt(szOf<i32>() as int, 4, "sizeof(T) in generic fn, T=i32");
    H.eqInt(szOf<i64>() as int, 8, "sizeof(T) in generic fn, T=i64");

    H.suite("list growth");
    let a: L.List<i32>;
    let n: int = 0;
    while (n < 500) { a.push(n); n = n + 1; }
    H.eqInt(a.cap as int, 512, "cap is 512");
    H.eqInt(a.at(0) as int, 0, "a[0]");
    H.eqInt(a.at(8) as int, 8, "a[8] (first growth)");
    H.eqInt(a.at(100) as int, 100, "a[100]");
    H.eqInt(a.at(498) as int, 498, "a[498]");
    H.eqInt(a.at(499) as int, 499, "a[499]");
    a.free();

    return H.finish();
}
