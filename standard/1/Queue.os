// standard/1/Queue.os
// Tier 3: FIFO Queue<T> (wraps Deque). Call free().
module D = import "standard/1/Deque.os";

struct Queue<T> {
    d: D.Deque<T>;
    free() => void { this.d.free(); }
    length() => u64 { return this.d.len; }
    isEmpty() => bool { return this.d.len == 0; }
    clear() => void { this.d.clear(); }
    push(v: T) => bool { return this.d.pushBack(v); }
    pop(out: T*) => bool { return this.d.popFront(out); }
    peek(out: T*) => bool { return this.d.peekFront(out); }
}
