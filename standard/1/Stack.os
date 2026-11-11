// standard/1/Stack.os
// Tier 3: LIFO Stack<T> (wraps List). Call free().
module L = import "standard/1/List.os";

struct Stack<T> {
    l: L.List<T>;
    free() => void { this.l.free(); }
    length() => u64 { return this.l.len; }
    isEmpty() => bool { return this.l.len == 0; }
    clear() => void { this.l.clear(); }
    push(v: T) => bool { return this.l.push(v); }
    pop(out: T*) => bool { return this.l.pop(out); }
    peek(out: T*) => bool { if (this.l.len == 0) return false; *out = this.l.data[this.l.len - 1]; return true; }
}
