// Reveals what sizeof actually returns. A FAIL named "X is N" means X == N.
module H = import "standard/1/tests/Harness.os";
module T = import "standard/1/Thread.os";

struct Shared {
    m: T.Mutex;
    n: i64 = 0;
    a: T.Atomic;
}

struct Small {
    x: i32 = 0;
}

function sz<T extends i32 | i64>() => u64 {
    return sizeof(T);
}

function main() => int {
    let a: u64 = sz<i32>();
    H.suite("sz<i32>()");
    H.check(a != 0, "sz<i32>() is 0");
    H.check(a != 1, "sz<i32>() is 1");
    H.check(a != 2, "sz<i32>() is 2");
    H.check(a != 4, "sz<i32>() is 4");
    H.check(a != 8, "sz<i32>() is 8");
    H.check(a != 16, "sz<i32>() is 16");

    let b: u64 = sz<i64>();
    H.suite("sz<i64>()");
    H.check(b != 0, "sz<i64>() is 0");
    H.check(b != 1, "sz<i64>() is 1");
    H.check(b != 4, "sz<i64>() is 4");
    H.check(b != 8, "sz<i64>() is 8");
    H.check(b != 16, "sz<i64>() is 16");

    let c: u64 = sizeof(Small);
    H.suite("sizeof(Small) (expected 4)");
    H.check(c != 0, "sizeof(Small) is 0");
    H.check(c != 1, "sizeof(Small) is 1");
    H.check(c != 4, "sizeof(Small) is 4");
    H.check(c != 8, "sizeof(Small) is 8");

    let d: u64 = sizeof(Shared);
    H.suite("sizeof(Shared) (expected 56)");
    H.check(d != 0, "sizeof(Shared) is 0");
    H.check(d != 1, "sizeof(Shared) is 1");
    H.check(d != 8, "sizeof(Shared) is 8");
    H.check(d != 16, "sizeof(Shared) is 16");
    H.check(d != 40, "sizeof(Shared) is 40");
    H.check(d != 56, "sizeof(Shared) is 56");

    let e: u64 = sizeof(T.Mutex);
    H.suite("sizeof(T.Mutex) (expected 40)");
    H.check(e != 0, "sizeof(Mutex) is 0");
    H.check(e != 1, "sizeof(Mutex) is 1");
    H.check(e != 8, "sizeof(Mutex) is 8");
    H.check(e != 40, "sizeof(Mutex) is 40");

    return H.finish();
}
