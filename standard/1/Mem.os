// standard/1/Mem.os
// Raw memory plus two safer helpers: Buffer (owned byte block) and Arena (bump allocator).
// Everything else in the standard library allocates through this module.
//
// Raw functions are thin wrappers over C, under plain names:
//   alloc(n) / release(p) / resize(p, n) / copy / move / fill / compare
// The extern C declarations are also here if you want the originals.
//
// ASSUMPTION: functions in an imported module are called as Mem.alloc(...), types as Mem.Buffer.

extern "C" fn malloc(size: u64) => void*;
extern "C" fn calloc(count: u64, size: u64) => void*;
extern "C" fn realloc(p: void*, size: u64) => void*;
extern "C" fn free(p: void*) => void;
extern "C" fn memcpy(dst: void*, src: void*, n: u64) => void*;
extern "C" fn memmove(dst: void*, src: void*, n: u64) => void*;
extern "C" fn memset(dst: void*, value: int, n: u64) => void*;
extern "C" fn memcmp(a: void*, b: void*, n: u64) => int;

// ---- raw ----
function alloc(n: u64) => void* { return malloc(n); }                       // nullptr on failure
function allocZeroed(n: u64) => void* { return calloc(1, n); }
function resize(p: void*, n: u64) => void* { return realloc(p, n); }        // p may be nullptr
function release(p: void*) => void { if (p != nullptr) free(p); }
function copy(dst: void*, src: void*, n: u64) => void { memcpy(dst, src, n); }       // no overlap
function move(dst: void*, src: void*, n: u64) => void { memmove(dst, src, n); }      // overlap ok
function fill(dst: void*, value: int, n: u64) => void { memset(dst, value, n); }
function compare(a: void*, b: void*, n: u64) => int { return memcmp(a, b, n); }

// ---- Buffer: an owned block of bytes ----
// A slice() is a non-owning view: free() on a slice does nothing to the memory.
struct Buffer {
    data: void* = nullptr;
    size: u64 = 0;
    owns: bool = false;

    alloc(n: u64) => bool {
        this.data = malloc(n);
        if (this.data == nullptr) return false;
        this.size = n;
        this.owns = true;
        return true;
    }

    allocZeroed(n: u64) => bool {
        this.data = calloc(1, n);
        if (this.data == nullptr) return false;
        this.size = n;
        this.owns = true;
        return true;
    }

    free() => void {
        if (this.owns && this.data != nullptr) free(this.data);
        this.data = nullptr;
        this.size = 0;
        this.owns = false;
    }

    // Keeps contents up to the smaller of the old and new size. Owned buffers only.
    resize(n: u64) => bool {
        if (!this.owns && this.data != nullptr) return false;
        let p = realloc(this.data, n);
        if (p == nullptr && n > 0) return false;
        this.data = p;
        this.size = n;
        this.owns = true;
        return true;
    }

    ptr() => void* { return this.data; }
    length() => u64 { return this.size; }

    getByte(i: u64) => int {
        let p = this.data as char*;
        return (p[i] as int) & 255;
    }

    setByte(i: u64, v: int) => void {
        let p = this.data as char*;
        p[i] = v as char;
    }

    // Non-owning view of [start, start+count) (clamped). false if start is past the end.
    slice(start: u64, count: u64, out: Buffer*) => bool {
        if (start > this.size) return false;
        let n: u64 = count;
        if (start + n > this.size) n = this.size - start;
        let p = this.data as char*;
        out.data = &p[start] as void*;
        out.size = n;
        out.owns = false;
        return true;
    }
}

// ---- Arena: bump allocator. Individual allocations are never freed; reset() reuses it all. ----
struct Arena {
    base: void* = nullptr;
    cap: u64 = 0;
    used: u64 = 0;

    init(capacity: u64) => bool {
        this.base = malloc(capacity);
        if (this.base == nullptr) return false;
        this.cap = capacity;
        this.used = 0;
        return true;
    }

    // 8-byte aligned. nullptr when the arena is full.
    alloc(n: u64) => void* {
        let aligned: u64 = (n + 7) & ~7;
        if (this.used + aligned > this.cap) return nullptr;
        let p = this.base as char*;
        let result = &p[this.used] as void*;
        this.used = this.used + aligned;
        return result;
    }

    reset() => void { this.used = 0; }

    free() => void {
        if (this.base != nullptr) free(this.base);
        this.base = nullptr;
        this.cap = 0;
        this.used = 0;
    }
}
