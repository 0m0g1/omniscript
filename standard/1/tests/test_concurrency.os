// standard/1/tests/test_concurrency.os  (Tier 5, thread-only part)
// Written against Thread.os / Channel.os as they exist: Mutex, Cond, Thread, Atomic and Channel<T>
// are plain structs (declare with `let m: T.Mutex;`, then init()/free()); there is no `.new()`.
// The Net / Http / Db sections of the old test_concurrency_net.os still need rewriting.
module H = import "standard/1/tests/Harness.os";
module T = import "standard/1/Thread.os";
module Ch = import "standard/1/Channel.os";

struct Shared {
    m: T.Mutex;
    n: i64 = 0;
    a: T.Atomic;
}

function incWorker(p: void*) => u32 {
    let s: Shared* = p as Shared*;
    let i: i32 = 0;
    while (i < 1000) {
        s.m.lock();
        s.n = s.n + 1;
        s.m.unlock();
        s.a.add(1);
        i = i + 1;
    }
    return 0;
}

function producer(p: void*) => u32 {
    let c: Ch.Channel<i32>* = p as Ch.Channel<i32>*;
    let i: i32 = 0;
    while (i < 100) {
        c.send(i);
        i = i + 1;
    }
    c.close();
    return 0;
}

function jobInc(arg: void*) => void {
    let a: T.Atomic* = arg as T.Atomic*;
    a.add(1);
}

function main() => int {
    // ----------------------------------------------------------------- Mutex
    H.suite("Mutex");
    let m: T.Mutex;
    m.init();
    m.lock();
    H.check(m.tryLock(), "tryLock succeeds on the owning thread (critical sections are recursive)");
    m.unlock();
    m.unlock();
    H.check(m.tryLock(), "tryLock succeeds when free");
    m.unlock();
    m.free();

    // ---------------------------------------------------------------- Atomic
    H.suite("Atomic");
    let at: T.Atomic;
    H.eqInt(at.load() as int, 0, "starts at 0");
    at.store(5);
    H.eqInt(at.load() as int, 5, "store/load");
    H.eqInt(at.add(3) as int, 8, "add returns the new value");
    H.check(at.compareExchange(8, 10), "compareExchange hit");
    H.eqInt(at.load() as int, 10, "value after exchange");
    H.check(!at.compareExchange(8, 1), "compareExchange miss");
    H.eqInt(at.load() as int, 10, "value unchanged after miss");

    // ---------------------------------------------------------- Misc helpers
    H.suite("Thread helpers");
    H.check(T.currentId() != 0, "currentId is non-zero");
    H.check(T.cpuCount() >= 1, "cpuCount >= 1");
    T.sleepMs(5);
    T.yieldNow();
    H.check(true, "sleepMs / yieldNow return");

    // --------------------------------------------------------------- Threads
    H.suite("Threads");
    let sh: Shared;
    sh.m.init();
    let t0: T.Thread;
    let t1: T.Thread;
    let t2: T.Thread;
    let t3: T.Thread;
    H.check(t0.spawn(incWorker, &sh as void*), "spawn 0");
    H.check(t1.spawn(incWorker, &sh as void*), "spawn 1");
    H.check(t2.spawn(incWorker, &sh as void*), "spawn 2");
    H.check(t3.spawn(incWorker, &sh as void*), "spawn 3");
    H.check(t0.join(), "join 0");
    H.check(t1.join(), "join 1");
    H.check(t2.join(), "join 2");
    H.check(t3.join(), "join 3");
    H.check(!t0.join(), "second join fails");
    H.eqInt(sh.n as int, 4000, "mutex-protected counter");
    H.eqInt(sh.a.load() as int, 4000, "atomic counter");
    sh.m.free();

    // --------------------------------------------------------------- Channel
    H.suite("Channel");
    let ch: Ch.Channel<i32>;
    ch.init(0);
    let v: i32 = -1;
    H.check(!ch.tryRecv(&v), "tryRecv on empty fails");
    ch.send(1);
    ch.send(2);
    ch.send(3);
    H.check(ch.recv(&v), "recv 1 ok");
    H.eqInt(v as int, 1, "FIFO 1");
    H.check(ch.tryRecv(&v), "tryRecv ok");
    H.eqInt(v as int, 2, "FIFO 2");
    ch.close();
    H.check(!ch.send(9), "send after close fails");
    H.check(ch.recv(&v), "closed channel still drains");
    H.eqInt(v as int, 3, "FIFO 3");
    H.check(!ch.recv(&v), "recv fails when closed and empty");
    ch.free();

    H.suite("Channel producer/consumer");
    let pc: Ch.Channel<i32>;
    pc.init(4);                              // bounded: producer must block and resume
    let tp: T.Thread;
    H.check(tp.spawn(producer, &pc as void*), "spawn producer");
    let sum: i32 = 0;
    let got: i32 = 0;
    while (pc.recv(&v)) {
        sum = sum + v;
        got = got + 1;
    }
    tp.join();
    H.eqInt(got, 100, "received every item");
    H.eqInt(sum, 4950, "sum 0..99");
    pc.free();

    // ------------------------------------------------------------ ThreadPool
    H.suite("ThreadPool");
    let counter: T.Atomic;
    let pool: Ch.ThreadPool;
    H.check(pool.start(4), "pool start");
    let i: i32 = 0;
    let ok: bool = true;
    while (i < 100) {
        if (!pool.submit(jobInc, &counter as void*)) ok = false;
        i = i + 1;
    }
    H.check(ok, "submit 100 jobs");
    pool.shutdown();                          // finishes queued jobs, joins workers
    H.eqInt(counter.load() as int, 100, "all jobs ran");

    return H.finish();
}
