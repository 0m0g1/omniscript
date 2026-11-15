// Diagnostics for the failing test_concurrency checks. Read the FAIL lines: each name says what it means.
module H = import "standard/1/tests/Harness.os";
module T = import "standard/1/Thread.os";
module Ch = import "standard/1/Channel.os";
module Q = import "standard/1/Queue.os";

struct Shared {
    m: T.Mutex;
    n: i64 = 0;
    a: T.Atomic;
}

function incLocked(p: void*) => u32 {
    let s: Shared* = p as Shared*;
    let i: i32 = 0;
    while (i < 1000) {
        s.m.lock();
        s.n = s.n + 1;
        s.m.unlock();
        i = i + 1;
    }
    return 0;
}

function tryWorker(p: void*) => u32 {
    let s: Shared* = p as Shared*;
    if (s.m.tryLock()) {
        s.m.unlock();
        s.a.add(1);
    }
    return 0;
}

function main() => int {
    H.suite("D1 pointer field arithmetic, one thread");
    let sh: Shared;
    sh.m.init();
    let sp: Shared* = &sh;
    let i: i32 = 0;
    while (i < 1000) {
        sp.n = sp.n + 1;
        i = i + 1;
    }
    H.eqInt(sh.n as int, 1000, "s.n = s.n + 1 through a pointer, 1000 times");

    H.suite("D2 is the mutex really shared between threads?");
    let t: T.Thread;
    sp.m.lock();                              // lock taken through a pointer
    t.spawn(tryWorker, &sh as void*);
    t.join();
    H.eqInt(sh.a.load() as int, 0, "other thread could NOT lock a mutex held via pointer (FAIL = pointer call locks a copy)");
    sp.m.unlock();
    sh.m.lock();                              // lock taken through the variable
    t.spawn(tryWorker, &sh as void*);
    t.join();
    H.eqInt(sh.a.load() as int, 0, "other thread could NOT lock a mutex held via variable (FAIL = direct call locks a copy)");
    sh.m.unlock();
    t.spawn(tryWorker, &sh as void*);
    t.join();
    H.eqInt(sh.a.load() as int, 1, "other thread CAN lock a free mutex");

    H.suite("D3 how bad is the counter?");
    sh.n = 0;
    let t0: T.Thread;
    let t1: T.Thread;
    let t2: T.Thread;
    let t3: T.Thread;
    t0.spawn(incLocked, &sh as void*);
    t1.spawn(incLocked, &sh as void*);
    t2.spawn(incLocked, &sh as void*);
    t3.spawn(incLocked, &sh as void*);
    t0.join();
    t1.join();
    t2.join();
    t3.join();
    H.check(sh.n >= 4000, "n >= 4000");
    H.check(sh.n > 3000, "n > 3000");
    H.check(sh.n > 2000, "n > 2000");
    H.check(sh.n > 1000, "n > 1000");
    H.check(sh.n > 0, "n > 0");
    sh.m.free();

    H.suite("D4 Channel<i32>: which value does recv give?");
    let ch: Ch.Channel<i32>;
    ch.init(0);
    let v: i32 = -1;
    ch.send(10);
    ch.send(20);
    ch.send(30);
    ch.recv(&v);
    H.check(v == 10, "recv #1 gave 10");
    H.check(v != -1, "recv #1 wrote v (FAIL = out pointer never written)");
    H.check(v != 0, "recv #1 did not give 0");
    H.check(v != 20, "recv #1 did not give 20");
    H.check(v != 30, "recv #1 did not give 30");
    ch.free();

    H.suite("D5 Queue<i32> (Deque only, no mutex)");
    let q: Q.Queue<i32>;
    let qv: i32 = -1;
    q.push(10);
    q.push(20);
    q.push(30);
    q.pop(&qv);
    H.check(qv == 10, "pop #1 gave 10");
    H.check(qv != -1, "pop #1 wrote qv (FAIL = out pointer never written)");
    H.check(qv != 0, "pop #1 did not give 0");
    H.check(qv != 20, "pop #1 did not give 20");
    H.check(qv != 30, "pop #1 did not give 30");
    q.free();

    return H.finish();
}
