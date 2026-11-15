// Bisects the StrMap crash in test_collections.os. The last suite header printed before the crash
// is the step that crashes.
module H = import "standard/1/tests/Harness.os";
module M = import "standard/1/Map.os";

function hs(k: char**) => u64 {
    let s: char* = *k;
    let h: u64 = 14695981039346656037;
    let i: u64 = 0;
    while ((s[i] as int) != 0) { h = (h ^ (s[i] as u64)) * 1099511628211; i = i + 1; }
    return h;
}

function main() => int {
    H.suite("S1 hash fn called directly");
    let key: char* = "alpha";
    let h1 = hs(&key);
    let h2 = hs(&key);
    H.check(h1 == h2, "S1 hash is stable");

    H.suite("S2 StrMap init");
    let sm: M.StrMap<i32>;
    sm.init();

    H.suite("S3 inner Map field set by init");
    H.check(sm.m.hashFn != nullptr, "S3 hashFn set");
    H.check(sm.m.eqFn != nullptr, "S3 eqFn set");

    H.suite("S4 length of empty");
    H.check(sm.length() == 0, "S4 length 0");

    H.suite("S5 put one key (grow + hash + find)");
    sm.put("alpha", 1);

    H.suite("S6 length after put");
    H.check(sm.length() == 1, "S6 length 1");

    H.suite("S7 put second key and overwrite");
    sm.put("beta", 2);
    sm.put("alpha", 3);
    H.check(sm.length() == 2, "S7 length 2");

    H.suite("S8 get / has / remove");
    let sv: i32 = 0;
    H.check(sm.get("alpha", &sv), "S8 get ok");
    H.check(sv == 3, "S8 value 3");
    H.check(!sm.has("gamma"), "S8 missing key");
    H.check(sm.remove("beta"), "S8 remove");

    H.suite("S9 free");
    sm.free();

    return H.finish();
}
