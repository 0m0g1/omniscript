// Is sizeof right for pointer types, directly and through a generic parameter? Expected: 8 for char*/i64*, 4 for i32.
module H = import "standard/1/tests/Harness.os";

extern "C" fn printf(fmt: char*, ...) => int;

function sz<T>() => u64 { return sizeof(T) as u64; }

struct Box<K> {
    cnt: u64 = 0;
    bytes() => u64 { return sizeof(K) as u64; }
}

function main() => int {
    H.suite("Z1 direct sizeof");
    let a: u64 = sizeof(char*) as u64;
    let b: u64 = sizeof(i64*) as u64;
    let c: u64 = sizeof(i32) as u64;
    let d: u64 = sizeof(i64) as u64;
    printf("direct: char*=%llu i64*=%llu i32=%llu i64=%llu\n", a, b, c, d);
    H.check(a == 8, "Z1 sizeof(char*) == 8");
    H.check(b == 8, "Z1 sizeof(i64*) == 8");
    H.check(c == 4, "Z1 sizeof(i32) == 4");
    H.check(d == 8, "Z1 sizeof(i64) == 8");

    H.suite("Z2 sizeof through a generic function");
    let e: u64 = sz<char*>();
    let f: u64 = sz<i32>();
    let g: u64 = sz<i64>();
    printf("generic fn: char*=%llu i32=%llu i64=%llu\n", e, f, g);
    H.check(e == 8, "Z2 sz<char*>() == 8");
    H.check(f == 4, "Z2 sz<i32>() == 4");
    H.check(g == 8, "Z2 sz<i64>() == 8");

    H.suite("Z3 sizeof through a generic struct method");
    let bp: Box<char*>;
    let bi: Box<i32>;
    let h: u64 = bp.bytes();
    let i: u64 = bi.bytes();
    printf("generic struct: Box<char*>=%llu Box<i32>=%llu\n", h, i);
    H.check(h == 8, "Z3 Box<char*>.bytes() == 8");
    H.check(i == 4, "Z3 Box<i32>.bytes() == 4");

    return H.finish();
}
