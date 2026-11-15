// Splits the first StrMap.put into its parts. The last suite header printed before the crash is the culprit.
module H = import "standard/1/tests/Harness.os";
module M = import "standard/1/Map.os";

function main() => int {
    let sm: M.StrMap<i32>;
    sm.init();
    let key: char* = "alpha";

    H.suite("T1 call hashFn through the inner map");
    let h1 = sm.m.hashFn(&key);
    H.check(h1 != 0, "T1 hashFn returned a hash");

    H.suite("T2 call eqFn(key,key)");
    let same = sm.m.eqFn(&key, &key);
    H.check(same, "T2 eqFn says key == key");

    H.suite("T3 grow");
    H.check(sm.m.grow(), "T3 grow ok");
    H.check(sm.m.cap == 16, "T3 cap 16");

    H.suite("T4 state is zeroed after grow");
    H.check(sm.m.state[0] == 0, "T4 state[0] == 0");
    H.check(sm.m.state[5] == 0, "T4 state[5] == 0");

    H.suite("T5 find on empty map");
    H.check(sm.m.find(&key) == -1, "T5 find returns -1");

    H.suite("T6 put through the inner map");
    H.check(sm.m.put(key, 1), "T6 inner put ok");

    H.suite("T7 put through StrMap");
    H.check(sm.put("beta", 2), "T7 StrMap put ok");

    H.suite("T8 free");
    sm.free();
    return H.finish();
}
