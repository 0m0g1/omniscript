// standard/1/List.os
// Tier 2: growable array List<T>. Owns its buffer: call free() when done (no destructors yet).
// NEVER copy a List with `=` (double free); everything that can fail returns bool.
// If T itself owns memory (e.g. String), free the elements first, then the list.
//
//   let xs: List.List<i32>;
//   xs.push(10); xs.push(20);
//   let v: i32 = 0;
//   if (xs.get(1, &v)) { ... }
//   xs.free();

module Core = import "standard/1/Core.os";
module CMem = import "standard/1/c/mem.os";

struct List<T> {
    data: T* = nullptr;
    len: u64 = 0;
    cap: u64 = 0;

    reserve(n: u64) => bool {
        if (n <= this.cap) return true;
        let newCap: u64 = this.cap;
        if (newCap == 0) newCap = 8;
        while (newCap < n) newCap = newCap * 2;
        let p = CMem.realloc(this.data as void*, newCap * sizeof(T));
        if (p == nullptr) return false;
        this.data = p as T*;
        this.cap = newCap;
        return true;
    }

    free() => void {
        if (this.data != nullptr) CMem.free(this.data as void*);
        this.data = nullptr;
        this.len = 0;
        this.cap = 0;
    }

    length() => u64 { return this.len; }
    isEmpty() => bool { return this.len == 0; }
    clear() => void { this.len = 0; }

    push(v: T) => bool {
        if (!this.reserve(this.len + 1)) return false;
        this.data[this.len] = v;
        this.len = this.len + 1;
        return true;
    }

    pop(out: T*) => bool {
        if (this.len == 0) return false;
        this.len = this.len - 1;
        *out = this.data[this.len];
        return true;
    }

    get(i: u64, out: T*) => bool {
        if (i >= this.len) return false;
        *out = this.data[i];
        return true;
    }

    // Panics when out of range. Use get() when the index is not trusted.
    at(i: u64) => T {
        if (i >= this.len) Core.panic("List.at: index out of range");
        return this.data[i];
    }

    set(i: u64, v: T) => bool {
        if (i >= this.len) return false;
        this.data[i] = v;
        return true;
    }

    // Pointer to element i, or nullptr. Invalid after the next push/insert/free.
    ptr(i: u64) => T* {
        if (i >= this.len) return nullptr;
        return &this.data[i];
    }

    insert(i: u64, v: T) => bool {
        if (i > this.len) return false;
        if (!this.reserve(this.len + 1)) return false;
        if (i < this.len) {
            CMem.memmove(&this.data[i + 1] as void*, &this.data[i] as void*, (this.len - i) * sizeof(T));
        }
        this.data[i] = v;
        this.len = this.len + 1;
        return true;
    }

    removeAt(i: u64) => bool {
        if (i >= this.len) return false;
        if (i + 1 < this.len) {
            CMem.memmove(&this.data[i] as void*, &this.data[i + 1] as void*, (this.len - i - 1) * sizeof(T));
        }
        this.len = this.len - 1;
        return true;
    }

    reverse() => void {
        if (this.len < 2) return;
        let i: u64 = 0;
        let j: u64 = this.len - 1;
        while (i < j) {
            Core.swapMem(&this.data[i] as void*, &this.data[j] as void*, sizeof(T));
            i = i + 1;
            j = j - 1;
        }
    }
}
