// standard/1/Deque.os
// Tier 3: ring-buffer double-ended queue. Call free().
module Core = import "standard/1/Core.os";
module CMem = import "standard/1/c/mem.os";

struct Deque<T> {
    data: T* = nullptr;
    head: u64 = 0;
    len: u64 = 0;
    cap: u64 = 0;

    free() => void {
        if (this.data != nullptr) CMem.free(this.data as void*);
        this.data = nullptr; this.head = 0; this.len = 0; this.cap = 0;
    }
    length() => u64 { return this.len; }
    isEmpty() => bool { return this.len == 0; }
    clear() => void { this.head = 0; this.len = 0; }

    grow() => bool {
        let ncap: u64 = this.cap * 2;
        if (ncap == 0) ncap = 8;
        let nd = CMem.malloc(ncap * sizeof(T)) as T*;
        if (nd == nullptr) return false;
        let i: u64 = 0;
        while (i < this.len) { nd[i] = this.data[(this.head + i) % this.cap]; i = i + 1; }
        if (this.data != nullptr) CMem.free(this.data as void*);
        this.data = nd; this.head = 0; this.cap = ncap;
        return true;
    }

    pushBack(v: T) => bool {
        if (this.len == this.cap) { if (!this.grow()) return false; }
        this.data[(this.head + this.len) % this.cap] = v;
        this.len = this.len + 1;
        return true;
    }
    pushFront(v: T) => bool {
        if (this.len == this.cap) { if (!this.grow()) return false; }
        this.head = (this.head + this.cap - 1) % this.cap;
        this.data[this.head] = v;
        this.len = this.len + 1;
        return true;
    }
    popFront(out: T*) => bool {
        if (this.len == 0) return false;
        *out = this.data[this.head];
        this.head = (this.head + 1) % this.cap;
        this.len = this.len - 1;
        return true;
    }
    popBack(out: T*) => bool {
        if (this.len == 0) return false;
        this.len = this.len - 1;
        *out = this.data[(this.head + this.len) % this.cap];
        return true;
    }
    peekFront(out: T*) => bool { if (this.len == 0) return false; *out = this.data[this.head]; return true; }
    peekBack(out: T*) => bool { if (this.len == 0) return false; *out = this.data[(this.head + this.len - 1) % this.cap]; return true; }
    get(i: u64, out: T*) => bool { if (i >= this.len) return false; *out = this.data[(this.head + i) % this.cap]; return true; }
}
