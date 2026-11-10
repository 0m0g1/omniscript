// standard/1/ListI64.os
// Non-generic growable array of i64. Same API as List<T>, plus contains/indexOf/sort,
// which need comparison. Fallback while generics settle, and handy for plain integer data.

module Core = import "standard/1/Core.os";
module CMem = import "standard/1/c/mem.os";

struct ListI64 {
    data: i64* = nullptr;
    len: u64 = 0;
    cap: u64 = 0;

    reserve(n: u64) => bool {
        if (n <= this.cap) return true;
        let newCap: u64 = this.cap;
        if (newCap == 0) newCap = 8;
        while (newCap < n) newCap = newCap * 2;
        let p = CMem.realloc(this.data as void*, newCap * 8);
        if (p == nullptr) return false;
        this.data = p as i64*;
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

    push(v: i64) => bool {
        if (!this.reserve(this.len + 1)) return false;
        this.data[this.len] = v;
        this.len = this.len + 1;
        return true;
    }

    pop(out: i64*) => bool {
        if (this.len == 0) return false;
        this.len = this.len - 1;
        *out = this.data[this.len];
        return true;
    }

    get(i: u64, out: i64*) => bool {
        if (i >= this.len) return false;
        *out = this.data[i];
        return true;
    }

    at(i: u64) => i64 {
        if (i >= this.len) Core.panic("ListI64.at: index out of range");
        return this.data[i];
    }

    set(i: u64, v: i64) => bool {
        if (i >= this.len) return false;
        this.data[i] = v;
        return true;
    }

    insert(i: u64, v: i64) => bool {
        if (i > this.len) return false;
        if (!this.reserve(this.len + 1)) return false;
        if (i < this.len) CMem.memmove(&this.data[i + 1] as void*, &this.data[i] as void*, (this.len - i) * 8);
        this.data[i] = v;
        this.len = this.len + 1;
        return true;
    }

    removeAt(i: u64) => bool {
        if (i >= this.len) return false;
        if (i + 1 < this.len) CMem.memmove(&this.data[i] as void*, &this.data[i + 1] as void*, (this.len - i - 1) * 8);
        this.len = this.len - 1;
        return true;
    }

    // Index of the first match, or -1.
    indexOf(v: i64) => i64 {
        for (let i: u64 = 0; i < this.len; i++) {
            if (this.data[i] == v) return i as i64;
        }
        return -1;
    }

    contains(v: i64) => bool { return this.indexOf(v) >= 0; }

    reverse() => void {
        if (this.len < 2) return;
        let i: u64 = 0;
        let j: u64 = this.len - 1;
        while (i < j) {
            let t = this.data[i];
            this.data[i] = this.data[j];
            this.data[j] = t;
            i = i + 1;
            j = j - 1;
        }
    }

    // Ascending, in place (insertion sort: fine for small lists; replaced by std/algo later).
    sort() => void {
        for (let i: u64 = 1; i < this.len; i++) {
            let key = this.data[i];
            let j: u64 = i;
            while (j > 0 && this.data[j - 1] > key) {
                this.data[j] = this.data[j - 1];
                j = j - 1;
            }
            this.data[j] = key;
        }
    }
}
