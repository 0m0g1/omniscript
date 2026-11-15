// Prints a marker before every group of List operations. The last marker printed before the
// process dies is the group containing the crash.
module H = import "standard/1/tests/Harness.os";
module L = import "standard/1/List.os";

function main() => int {
    H.suite("L1 declare + push 1000");
    let l: L.List<i32>;
    let n: i32 = 0;
    let ok: bool = true;
    while (n < 1000) {
        if (!l.push(n)) ok = false;
        n = n + 1;
    }
    H.check(ok, "push 1000 succeeds");

    H.suite("L2 length / isEmpty");
    H.check(l.length() == 1000, "length is 1000");
    H.check(!l.isEmpty(), "not empty");

    H.suite("L3 get");
    let v: i32 = -1;
    H.check(l.get(0, &v), "get first ok");
    H.check(v == 0, "get first value");
    H.check(l.get(999, &v), "get last ok");
    H.check(v == 999, "get last value");

    H.suite("L4 at");
    H.check(l.at(500) == 500, "at(500)");

    H.suite("L5 set");
    H.check(l.set(5, 77), "set ok");
    l.get(5, &v);
    H.check(v == 77, "set value");

    H.suite("L6 pop");
    H.check(l.pop(&v), "pop ok");
    H.check(v == 999, "pop value");

    H.suite("L7 insert");
    H.check(l.insert(0, 42), "insert front ok");
    l.get(0, &v);
    H.check(v == 42, "insert front value");

    H.suite("L8 removeAt");
    H.check(l.removeAt(0), "removeAt ok");

    H.suite("L9 ptr");
    let p: i32* = l.ptr(3);
    H.check(p != nullptr, "ptr in range");
    H.check(l.ptr(5000) == nullptr, "ptr out of range is null");

    H.suite("L10 out-of-range");
    H.check(!l.get(5000, &v), "get out of range fails");
    H.check(!l.set(5000, 1), "set out of range fails");
    H.check(!l.removeAt(5000), "removeAt out of range fails");
    H.check(!l.insert(l.length() + 1, 1), "insert past end fails");

    H.suite("L11 clear");
    l.clear();
    H.check(l.isEmpty(), "empty after clear");
    H.check(!l.pop(&v), "pop on empty fails");

    H.suite("L12 reverse");
    l.push(1);
    l.push(2);
    l.push(3);
    l.reverse();
    l.get(0, &v);
    H.check(v == 3, "reverse first");

    H.suite("L13 free");
    l.free();
    H.check(l.length() == 0, "free resets length");
    H.check(l.push(9), "push after free");
    l.free();

    return H.finish();
}
