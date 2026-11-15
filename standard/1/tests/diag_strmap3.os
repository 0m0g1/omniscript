// Why does a repeated StrMap key not match? Prints hashes, slots and eq results.
module H = import "standard/1/tests/Harness.os";
module M = import "standard/1/Map.os";

extern "C" fn printf(fmt: char*, ...) => int;

function main() => int {
    let sm: M.StrMap<i32>;
    sm.init();

    H.suite("U1 same text, two different pointers");
    let k1: char* = "alpha";
    let k2: char* = "alpha";
    let ha = sm.m.hashFn(&k1);
    let hb = sm.m.hashFn(&k2);
    printf("hash k1=%llu k2=%llu  ptr k1=%llu k2=%llu\n", ha, hb, k1 as u64, k2 as u64);
    H.check(ha == hb, "U1 hash equal for equal text");
    H.check(sm.m.eqFn(&k1, &k2), "U1 eqFn equal for equal text");

    H.suite("U2 put alpha then find it");
    sm.put("alpha", 1);
    let f1 = sm.m.find(&k1);
    printf("after put alpha: find=%lld len=%llu used=%llu cap=%llu\n", f1, sm.m.len, sm.m.used, sm.m.cap);
    H.check(f1 >= 0, "U2 find alpha right after put");

    H.suite("U3 put beta, find both");
    let kb: char* = "beta";
    sm.put("beta", 2);
    let fa = sm.m.find(&k1);
    let fb = sm.m.find(&kb);
    printf("find alpha=%lld beta=%lld len=%llu used=%llu\n", fa, fb, sm.m.len, sm.m.used);
    H.check(fa >= 0, "U3 find alpha after beta put");
    H.check(fb >= 0, "U3 find beta");

    H.suite("U4 put alpha again (should overwrite)");
    sm.put("alpha", 3);
    printf("after re-put: len=%llu used=%llu find alpha=%lld\n", sm.m.len, sm.m.used, sm.m.find(&k1));
    H.check(sm.m.len == 2, "U4 length 2 after overwrite");

    H.suite("U5 dump slots");
    let i: u64 = 0;
    while (i < sm.m.cap) {
        if (sm.m.state[i] != 0) {
            let sk: char* = sm.m.keys[i];
            printf("slot %llu state=%d key=%s\n", i, sm.m.state[i] as int, sk);
        }
        i = i + 1;
    }

    sm.free();
    return H.finish();
}
