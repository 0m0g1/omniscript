// standard/1/ListU32.os
// Non-generic growable array of u32. Same API as List<T> plus pushArray(). Handy for building
// vertex/index data for OpenGL.Mesh and Vulkan buffers: pass `data` and `len` straight to upload calls.
// Owns its buffer: call free() when done. NEVER copy with `=` (double free).

module Core = import "standard/1/Core.os";
module CMem = import "standard/1/c/mem.os";

struct ListU32 {
    data: u32* = nullptr;
    len: u64 = 0;
    cap: u64 = 0;

    reserve(n: u64) => bool {
        if (n <= this.cap) return true;
        let newCap: u64 = this.cap;
        if (newCap == 0) newCap = 8;
        while (newCap < n) newCap = newCap * 2;
        let p = CMem.realloc(this.data as void*, newCap * 4);
        if (p == nullptr) return false;
        this.data = p as u32*;
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
    byteSize() => u64 { return this.len * 4; }

    push(v: u32) => bool {
        if (!this.reserve(this.len + 1)) return false;
        this.data[this.len] = v;
        this.len = this.len + 1;
        return true;
    }

    // Append n values from src.
    pushArray(src: u32*, n: u64) => bool {
        if (!this.reserve(this.len + n)) return false;
        let i: u64 = 0;
        while (i < n) {
            this.data[this.len + i] = src[i];
            i = i + 1;
        }
        this.len = this.len + n;
        return true;
    }

    pop(out: u32*) => bool {
        if (this.len == 0) return false;
        this.len = this.len - 1;
        *out = this.data[this.len];
        return true;
    }

    get(i: u64, out: u32*) => bool {
        if (i >= this.len) return false;
        *out = this.data[i];
        return true;
    }

    at(i: u64) => u32 {
        if (i >= this.len) Core.panic("ListU32.at: index out of range");
        return this.data[i];
    }

    set(i: u64, v: u32) => bool {
        if (i >= this.len) return false;
        this.data[i] = v;
        return true;
    }

    insert(i: u64, v: u32) => bool {
        if (i > this.len) return false;
        if (!this.reserve(this.len + 1)) return false;
        if (i < this.len) CMem.memmove(&this.data[i + 1] as void*, &this.data[i] as void*, (this.len - i) * 4);
        this.data[i] = v;
        this.len = this.len + 1;
        return true;
    }

    removeAt(i: u64) => bool {
        if (i >= this.len) return false;
        if (i + 1 < this.len) CMem.memmove(&this.data[i] as void*, &this.data[i + 1] as void*, (this.len - i - 1) * 4);
        this.len = this.len - 1;
        return true;
    }

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
}
