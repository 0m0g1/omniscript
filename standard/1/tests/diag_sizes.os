// Checks sizes, field offsets, generic sizeof and the first List allocations. Every FAIL names what is wrong.
module H = import "standard/1/tests/Harness.os";
module T = import "standard/1/Thread.os";
module L = import "standard/1/List.os";

struct Shared {
    m: T.Mutex;
    n: i64 = 0;
    a: T.Atomic;
}

function sz<T extends i32 | i64>() => u64 {
    return sizeof(T);
}

function main() => int {
    H.suite("S1 sizeof, plain types");
    H.eqInt(sizeof(i32) as int, 4, "sizeof(i32) == 4");
    H.eqInt(sizeof(i64) as int, 8, "sizeof(i64) == 8");
    H.eqInt(sizeof(u8) as int, 1, "sizeof(u8) == 1");

    H.suite("S2 sizeof, structs");
    H.eqInt(sizeof(T.Mutex) as int, 40, "sizeof(Mutex) == 40");
    H.eqInt(sizeof(T.Cond) as int, 8, "sizeof(Cond) == 8");
    H.eqInt(sizeof(T.Atomic) as int, 8, "sizeof(Atomic) == 8");
    H.eqInt(sizeof(Shared) as int, 56, "sizeof(Shared) == 56");

    H.suite("S3 sizeof inside a generic function");
    H.eqInt(sz<i32>() as int, 4, "sz<i32>() == 4");
    H.eqInt(sz<i64>() as int, 8, "sz<i64>() == 8");

    H.suite("S4 field offsets in Shared");
    let sh: Shared;
    let base: u64 = (&sh) as u64;
    H.eqInt(((&sh.m) as u64 - base) as int, 0, "offset of m == 0");
    H.eqInt(((&sh.n) as u64 - base) as int, 40, "offset of n == 40");
    H.eqInt(((&sh.a) as u64 - base) as int, 48, "offset of a == 48");

    H.suite("S5 first List allocation");
    let l: L.List<i32>;
    l.push(1);
    H.eqInt(l.cap as int, 8, "cap after first push == 8");
    H.check(l.data != nullptr, "data allocated");

    H.suite("S6 fill to 8");
    let i: i32 = 2;
    while (i <= 8) {
        l.push(i);
        i = i + 1;
    }
    H.eqInt(l.len as int, 8, "len == 8");
    H.eqInt(l.cap as int, 8, "cap still 8");

    H.suite("S7 9th push forces the first realloc");
    l.push(9);
    H.eqInt(l.cap as int, 16, "cap == 16");
    H.eqInt(l.at(8) as int, 9, "ninth element");

    H.suite("S8 free");
    l.free();
    return H.finish();
}
