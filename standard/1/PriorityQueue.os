// standard/1/PriorityQueue.os
// Tier 3: binary heap. cmp(a,b) < 0 means a pops first (min-heap by cmp). Call init(cmp) then free().
module L = import "standard/1/List.os";
module Core = import "standard/1/Core.os";

struct PriorityQueue<T> {
    l: L.List<T>;
    cmp: fn(T*, T*) => int = nullptr;

    init(c: fn(T*, T*) => int) => void { this.cmp = c; }
    free() => void { this.l.free(); }
    length() => u64 { return this.l.len; }
    isEmpty() => bool { return this.l.len == 0; }
    clear() => void { this.l.clear(); }

    push(v: T) => bool {
        if (!this.l.push(v)) return false;
        let i: u64 = this.l.len - 1;
        while (i > 0) {
            let p: u64 = (i - 1) / 2;
            if (this.cmp(&this.l.data[i], &this.l.data[p]) >= 0) return true;
            Core.swapMem(&this.l.data[i] as void*, &this.l.data[p] as void*, sizeof(T));
            i = p;
        }
        return true;
    }

    peek(out: T*) => bool { if (this.l.len == 0) return false; *out = this.l.data[0]; return true; }

    pop(out: T*) => bool {
        if (this.l.len == 0) return false;
        *out = this.l.data[0];
        this.l.len = this.l.len - 1;
        if (this.l.len == 0) return true;
        this.l.data[0] = this.l.data[this.l.len];
        let i: u64 = 0;
        while (true) {
            let a: u64 = i * 2 + 1;
            let b: u64 = a + 1;
            let m: u64 = i;
            if (a < this.l.len && this.cmp(&this.l.data[a], &this.l.data[m]) < 0) m = a;
            if (b < this.l.len && this.cmp(&this.l.data[b], &this.l.data[m]) < 0) m = b;
            if (m == i) return true;
            Core.swapMem(&this.l.data[i] as void*, &this.l.data[m] as void*, sizeof(T));
            i = m;
        }
        return true;
    }
}
