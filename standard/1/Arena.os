//! Bump allocator. `alloc(n)` hands out zeroed, 16-byte-aligned memory from a chain of blocks;
//! `destroy()` runs the recorded destructors (newest first), then frees every block at once.
//! Nothing is freed individually.
//!
//! ```
//! let a: Arena;
//! let p = a.alloc(64);
//! a.destroy();
//! ```
//! `new(a) T(args)` calls `alloc` and, when `T` has a destructor, `record`.

extern "C" fn malloc(size: u64) => void*;
extern "C" fn free(p: void*) => void;
extern "C" fn memset(p: void*, v: int, n: u64) => void*;
/// Calls the function pointer `f` with `obj` (defined by the compiler; see IRGeneratorCodegen.cpp).
intrinsic fn call_ptr1(f: void*, obj: void*) => void;

struct Arena {
    /// Current block. Its first 8 bytes hold the previous block (or 0); data starts at offset 16.
    head: void* = nullptr;
    /// Bytes used in the current block, including the 16-byte header.
    used: u64 = 0;
    /// Size of the current block in bytes.
    size: u64 = 0;
    /// Newest-first list of {object, destructor, next}, each node allocated from this arena.
    dtors: void* = nullptr;

    /// Returns `n` zeroed bytes (rounded up to 16), or `nullptr` if memory runs out.
    alloc(n: u64) => void* {
        let need: u64 = ((n + 15) / 16) * 16;
        if (this.head == nullptr || this.used + need > this.size) {
            let bsz: u64 = 65536;
            if (need + 16 > bsz) bsz = need + 16;
            let blk: void* = malloc(bsz);
            if (blk == nullptr) return nullptr;
            let hdr = blk as u64*;
            hdr[0] = this.head as u64;
            this.head = blk;
            this.used = 16;
            this.size = bsz;
        }
        let p: void* = ((this.head as u64) + this.used) as void*;
        this.used = this.used + need;
        memset(p, 0, need);
        return p;
    }

    /// Remembers that `obj` must be destroyed with `fn` when the arena is destroyed.
    record(obj: void*, fn: void*) => void {
        let node = this.alloc(24) as u64*;
        if (node == nullptr) return;
        node[0] = obj as u64;
        node[1] = fn as u64;
        node[2] = this.dtors as u64;
        this.dtors = node as void*;
    }

    /// Runs the recorded destructors (newest first), then frees every block.
    /// All memory handed out by this arena is invalid afterwards.
    destroy() => void {
        let d: void* = this.dtors;
        while (d != nullptr) {
            let node = d as u64*;
            call_ptr1(node[1] as void*, node[0] as void*);
            d = node[2] as void*;
        }
        this.dtors = nullptr;
        let b: void* = this.head;
        while (b != nullptr) {
            let hdr = b as u64*;
            let prev: void* = hdr[0] as void*;
            free(b);
            b = prev;
        }
        this.head = nullptr;
        this.used = 0;
        this.size = 0;
    }
}
