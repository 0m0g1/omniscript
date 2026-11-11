// standard/1/Channel.os
// Tier 5: thread-safe bounded/unbounded MPMC channel Channel<T>. cap 0 = unbounded. Call init(cap) / free().
module Th = import "standard/1/Thread.os";
module D = import "standard/1/Deque.os";
module L = import "standard/1/List.os";

struct Channel<T> {
    q: D.Deque<T>;
    m: Th.Mutex;
    notEmpty: Th.Cond;
    notFull: Th.Cond;
    cap: u64 = 0;
    closed: bool = false;

    init(cap: u64) => void { this.cap = cap; this.m.init(); this.notEmpty.init(); this.notFull.init(); }
    free() => void { this.q.free(); this.m.free(); }

    // blocks while full; false if closed
    send(v: T) => bool {
        this.m.lock();
        while (this.cap > 0 && this.q.len >= this.cap && !this.closed) this.notFull.wait(&this.m);
        if (this.closed) { this.m.unlock(); return false; }
        this.q.pushBack(v);
        this.notEmpty.signal();
        this.m.unlock();
        return true;
    }

    // blocks while empty; false when closed and drained
    recv(out: T*) => bool {
        this.m.lock();
        while (this.q.len == 0 && !this.closed) this.notEmpty.wait(&this.m);
        if (this.q.len == 0) { this.m.unlock(); return false; }
        this.q.popFront(out);
        this.notFull.signal();
        this.m.unlock();
        return true;
    }

    tryRecv(out: T*) => bool {
        this.m.lock();
        let ok: bool = this.q.popFront(out);
        if (ok) this.notFull.signal();
        this.m.unlock();
        return ok;
    }

    close() => void {
        this.m.lock();
        this.closed = true;
        this.notEmpty.broadcast(); this.notFull.broadcast();
        this.m.unlock();
    }
}

// ---- ThreadPool: fixed workers pulling jobs from a Channel ----
struct Job { f: fn(void*) => void = nullptr; arg: void* = nullptr; }

function workerMain(p: void*) => u32 {
    let pool: ThreadPool* = p as ThreadPool*;
    let j: Job;
    while (pool.jobs.recv(&j)) j.f(j.arg);
    return 0;
}

struct ThreadPool {
    jobs: Channel<Job>;
    workers: L.List<Th.Thread>;
    start(n: u64) => bool {
        this.jobs.init(0);
        let i: u64 = 0;
        while (i < n) {
            let t: Th.Thread;
            if (!t.spawn(workerMain, this as void*)) return false;
            this.workers.push(t);
            i = i + 1;
        }
        return true;
    }
    submit(f: fn(void*) => void, arg: void*) => bool { let j: Job; j.f = f; j.arg = arg; return this.jobs.send(j); }
    // Finish queued jobs, stop workers, release memory.
    shutdown() => void {
        this.jobs.close();
        let i: u64 = 0;
        while (i < this.workers.len) { this.workers.data[i].join(); i = i + 1; }
        this.workers.free();
        this.jobs.free();
    }
}
