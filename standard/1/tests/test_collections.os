// standard/1/tests/test_collections.os  (Tier 2/3)
// Written against the real APIs: structs are declared with `let x: Mod.Type<T>;`, nothing has
// `.new()`, fallible calls return bool and hand values back through out pointers, and every
// owning container needs free().
module H = import "standard/1/tests/Harness.os";
module L = import "standard/1/List.os";
module M = import "standard/1/Map.os";
module S = import "standard/1/Set.os";
module Q = import "standard/1/Queue.os";
module St = import "standard/1/Stack.os";
module PQ = import "standard/1/PriorityQueue.os";
// Algo is `include`d, not imported: its functions are generic and must be called unqualified
// (`sort<i32>(...)`), and module members can't be passed as function values (`M.hashI64`).
include "standard/1/Algo.os";

function toInt(x: u64) => int {
    return x as int;
}

function hashKey64(k: i64*) => u64 {
    let x: u64 = (*k) as u64;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
    return x ^ (x >> 31);
}

function eqKey64(a: i64*, b: i64*) => bool {
    return *a == *b;
}

function eqI32(a: i32*, b: i32*) => bool {
    return *a == *b;
}

function isEven(x: i32*) => bool {
    return (*x % 2) == 0;
}

function main() => int {
    // ------------------------------------------------------------------ List
    H.suite("List");
    let l: L.List<i32>;

    let n: i32 = 0;
    let ok: bool = true;
    while (n < 1000) {                       // forces several resizes (8 -> ... -> 1024)
        if (!l.push(n)) ok = false;
        n = n + 1;
    }
    H.check(ok, "push 1000 succeeds");
    H.eqInt(toInt(l.length()), 1000, "length after 1000 pushes");
    H.check(!l.isEmpty(), "not empty");

    let v: i32 = -1;
    H.check(l.get(0, &v), "get first ok");
    H.eqInt(v as int, 0, "get first value");
    H.check(l.get(999, &v), "get last ok");
    H.eqInt(v as int, 999, "get last value");
    H.eqInt(l.at(500) as int, 500, "at(500)");

    H.check(l.set(5, 77), "set ok");
    l.get(5, &v);
    H.eqInt(v as int, 77, "set value");

    H.check(l.pop(&v), "pop ok");
    H.eqInt(v as int, 999, "pop value");
    H.eqInt(toInt(l.length()), 999, "length after pop");

    H.check(l.insert(0, 42), "insert front ok");
    l.get(0, &v);
    H.eqInt(v as int, 42, "insert front value");
    l.get(1, &v);
    H.eqInt(v as int, 0, "insert shifts right");
    H.eqInt(toInt(l.length()), 1000, "length after insert");
    H.check(l.removeAt(0), "removeAt ok");
    l.get(0, &v);
    H.eqInt(v as int, 0, "removeAt shifts left");
    H.eqInt(toInt(l.length()), 999, "length after removeAt");

    let p: i32* = l.ptr(3);
    H.check(p != nullptr, "ptr in range");
    H.check(l.ptr(5000) == nullptr, "ptr out of range is null");

    H.check(!l.get(5000, &v), "get out of range fails");
    H.check(!l.set(5000, 1), "set out of range fails");
    H.check(!l.removeAt(5000), "removeAt out of range fails");
    H.check(!l.insert(l.length() + 1, 1), "insert past end fails");

    l.clear();
    H.eqInt(toInt(l.length()), 0, "clear");
    H.check(l.isEmpty(), "empty after clear");
    H.check(!l.pop(&v), "pop on empty fails");

    l.push(1);
    l.push(2);
    l.push(3);
    l.reverse();
    l.get(0, &v);
    H.eqInt(v as int, 3, "reverse first");
    l.get(2, &v);
    H.eqInt(v as int, 1, "reverse last");

    l.free();
    H.eqInt(toInt(l.length()), 0, "free resets length");
    H.check(l.push(9), "push after free");
    l.free();

    // ------------------------------------------------------------------- Map
    H.suite("Map");
    let m: M.Map<i64, i64>;
    m.init(hashKey64, eqKey64);

    let i: i64 = 0;
    ok = true;
    while (i < 2000) {                       // forces several rehashes
        if (!m.put(i, i * 2)) ok = false;
        i = i + 1;
    }
    H.check(ok, "put 2000 succeeds");
    H.eqInt(toInt(m.length()), 2000, "2000 entries");

    let mv: i64 = -1;
    H.check(m.get(1500, &mv), "get after rehash ok");
    H.eqInt(mv as int, 3000, "get after rehash value");
    H.check(!m.get(99999, &mv), "missing key not found");
    H.check(m.ptr(99999) == nullptr, "ptr of missing key is null");
    H.check(m.ptr(7) != nullptr, "ptr of present key");

    m.put(10, 1);
    m.get(10, &mv);
    H.eqInt(mv as int, 1, "overwrite value");
    H.eqInt(toInt(m.length()), 2000, "overwrite keeps length");

    H.check(m.remove(10), "remove present");
    H.check(!m.has(10), "removed key gone");
    H.check(!m.remove(10), "remove twice fails");
    H.eqInt(toInt(m.length()), 1999, "length after remove");

    // iterate: count entries and sum keys (0..1999 minus the removed 10)
    let it: u64 = 0;
    let kp: i64* = nullptr;
    let vp: i64* = nullptr;
    let cnt: i64 = 0;
    let sum: i64 = 0;
    while (m.next(&it, &kp, &vp)) {
        cnt = cnt + 1;
        sum = sum + *kp;
    }
    H.eqInt(cnt as int, 1999, "iteration visits every entry");
    H.eqInt(sum as int, 1998990, "iteration key sum");

    m.put(10, 5);                            // reuses the tombstone path
    H.check(m.has(10), "re-put after remove");
    H.eqInt(toInt(m.length()), 2000, "length after re-put");

    m.clear();
    H.eqInt(toInt(m.length()), 0, "clear");
    H.check(!m.has(5), "cleared key gone");
    H.check(m.put(5, 6), "put after clear");
    H.check(m.has(5), "has after clear+put");
    m.free();

    H.suite("StrMap");
    let sm: M.StrMap<i32>;
    sm.init();
    sm.put("alpha", 1);
    sm.put("beta", 2);
    sm.put("alpha", 3);
    H.eqInt(toInt(sm.length()), 2, "StrMap dedupes keys");
    let sv: i32 = 0;
    H.check(sm.get("alpha", &sv), "StrMap get ok");
    H.eqInt(sv as int, 3, "StrMap overwrite value");
    H.check(!sm.has("gamma"), "StrMap missing key");
    H.check(sm.remove("beta"), "StrMap remove");
    H.eqInt(toInt(sm.length()), 1, "StrMap length after remove");
    sm.free();

    // ------------------------------------------------------------------- Set
    H.suite("Set");
    let st: S.Set<i64>;
    st.init(hashKey64, eqKey64);
    st.add(1);
    st.add(2);
    st.add(2);
    H.eqInt(toInt(st.length()), 2, "dedupe");
    H.check(st.has(2), "has present");
    H.check(!st.has(3), "has absent");
    H.check(st.remove(2), "remove present");
    H.check(!st.has(2), "removed gone");
    H.eqInt(toInt(st.length()), 1, "length after remove");
    st.clear();
    H.check(st.isEmpty(), "clear");

    let sa: S.Set<i64>;
    let sb: S.Set<i64>;
    let sc: S.Set<i64>;
    sa.init(hashKey64, eqKey64);
    sb.init(hashKey64, eqKey64);
    sc.init(hashKey64, eqKey64);
    sa.add(1); sa.add(2); sa.add(3);
    sb.add(2); sb.add(3); sb.add(4);
    H.check(sa.intersect(&sb, &sc), "intersect ok");
    H.eqInt(toInt(sc.length()), 2, "intersect size");
    H.check(sc.has(2) && sc.has(3) && !sc.has(1), "intersect members");
    H.check(sa.unionWith(&sb), "union ok");
    H.eqInt(toInt(sa.length()), 4, "union size");
    H.check(sa.has(4), "union has new member");
    st.free();
    sa.free();
    sb.free();
    sc.free();

    // ----------------------------------------------------- Queue / Stack / PQ
    H.suite("Queue");
    let q: Q.Queue<i32>;
    let qv: i32 = 0;
    q.push(1);
    q.push(2);
    q.push(3);
    H.eqInt(toInt(q.length()), 3, "queue length");
    H.check(q.pop(&qv), "FIFO pop 1 ok");
    H.eqInt(qv as int, 1, "FIFO 1");
    H.check(q.peek(&qv), "peek ok");
    H.eqInt(qv as int, 2, "peek value");
    H.eqInt(toInt(q.length()), 2, "peek keeps length");
    q.pop(&qv);
    H.eqInt(qv as int, 2, "FIFO 2");
    q.pop(&qv);
    H.eqInt(qv as int, 3, "FIFO 3");
    H.check(!q.pop(&qv), "pop on empty fails");
    H.check(q.isEmpty(), "queue empty");

    // wrap-around: interleave pushes and pops so the ring buffer wraps and grows
    n = 0;
    while (n < 50) { q.push(n); n = n + 1; }
    n = 0;
    while (n < 25) { q.pop(&qv); n = n + 1; }
    n = 50;
    while (n < 100) { q.push(n); n = n + 1; }
    ok = true;
    n = 25;
    while (n < 100) {
        if (!q.pop(&qv) || qv != n) ok = false;
        n = n + 1;
    }
    H.check(ok, "FIFO order survives wrap and growth");
    H.check(q.isEmpty(), "empty after draining");
    q.free();

    H.suite("Stack");
    let sk: St.Stack<i32>;
    let kv: i32 = 0;
    sk.push(1);
    sk.push(2);
    H.check(sk.peek(&kv), "stack peek ok");
    H.eqInt(kv as int, 2, "stack peek value");
    H.eqInt(toInt(sk.length()), 2, "peek keeps length");
    sk.pop(&kv);
    H.eqInt(kv as int, 2, "LIFO 2");
    sk.pop(&kv);
    H.eqInt(kv as int, 1, "LIFO 1");
    H.check(!sk.pop(&kv), "pop on empty fails");
    H.check(!sk.peek(&kv), "peek on empty fails");
    sk.free();

    H.suite("PriorityQueue");
    let pq: PQ.PriorityQueue<i32>;
    pq.init(cmpI32);
    let pv: i32 = 0;
    pq.push(5);
    pq.push(1);
    pq.push(9);
    pq.push(3);
    H.check(pq.peek(&pv), "pq peek ok");
    H.eqInt(pv as int, 1, "pq peek smallest");
    pq.pop(&pv);
    H.eqInt(pv as int, 1, "pq pop 1");
    pq.pop(&pv);
    H.eqInt(pv as int, 3, "pq pop 3");
    pq.pop(&pv);
    H.eqInt(pv as int, 5, "pq pop 5");
    pq.pop(&pv);
    H.eqInt(pv as int, 9, "pq pop 9");
    H.check(!pq.pop(&pv), "pq pop on empty fails");

    n = 0;
    while (n < 200) { pq.push((n * 7919) % 1009); n = n + 1; }
    let prev: i32 = -1;
    ok = true;
    n = 0;
    while (n < 200) {
        pq.pop(&pv);
        if (pv < prev) ok = false;
        prev = pv;
        n = n + 1;
    }
    H.check(ok, "pq pops 200 items in nondecreasing order");
    H.check(pq.isEmpty(), "pq drained");
    pq.free();

    // ------------------------------------------------------------------ Algo
    H.suite("Algo");
    let a: L.List<i32>;
    a.push(5);
    a.push(2);
    a.push(9);
    a.push(1);
    sort<i32>(a.data, a.len, cmpI32);
    a.get(0, &v);
    H.eqInt(v as int, 1, "sort first");
    a.get(3, &v);
    H.eqInt(v as int, 9, "sort last");

    let key: i32 = 5;
    H.eqInt(binarySearch<i32>(a.data, a.len, &key, cmpI32) as int, 2, "binarySearch hit");
    key = 7;
    H.eqInt(binarySearch<i32>(a.data, a.len, &key, cmpI32) as int, -1, "binarySearch miss");

    reverse<i32>(a.data, a.len);        // now 9 5 2 1
    a.get(0, &v);
    H.eqInt(v as int, 9, "reverse first");
    a.get(3, &v);
    H.eqInt(v as int, 1, "reverse last");

    key = 2;
    H.eqInt(indexOf<i32>(a.data, a.len, &key, eqI32) as int, 2, "indexOf hit");
    key = 100;
    H.eqInt(indexOf<i32>(a.data, a.len, &key, eqI32) as int, -1, "indexOf miss");

    H.eqInt(toInt(count<i32>(a.data, a.len, isEven)), 1, "count evens");
    H.check(anyOf<i32>(a.data, a.len, isEven), "anyOf even");
    H.check(!allOf<i32>(a.data, a.len, isEven), "allOf even is false");

    let mn: i32* = minOf<i32>(a.data, a.len, cmpI32);
    let mx: i32* = maxOf<i32>(a.data, a.len, cmpI32);
    H.check(mn != nullptr && mx != nullptr, "min/max non-null");
    H.eqInt(*mn as int, 1, "minOf");
    H.eqInt(*mx as int, 9, "maxOf");
    a.clear();
    H.check(minOf<i32>(a.data, a.len, cmpI32) == nullptr, "minOf empty is null");

    // larger sort: exercises the quicksort path (n > 16), duplicates included
    n = 0;
    while (n < 500) { a.push((n * 7919) % 101); n = n + 1; }
    sort<i32>(a.data, a.len, cmpI32);
    ok = true;
    let k: u64 = 1;
    while (k < a.len) {
        if (a.at(k - 1) > a.at(k)) ok = false;
        k = k + 1;
    }
    H.check(ok, "sort 500 elements is ordered");
    H.eqInt(toInt(a.length()), 500, "sort keeps length");
    a.free();

    return H.finish();
}
