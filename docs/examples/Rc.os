//! Owning memory blocks: `Box` (one owner, deep-copied) and `Rc` (shared, reference counted).
//! Same conventions as String.os: the destructor releases, passing by value runs the copy constructor.
//!
//! ## Ownership rules
//!
//! - `Box` owns one heap block. Copying a `Box` (pass by value) copies the bytes into a new block,
//!   so two `Box`es never free the same memory.
//! - `Rc` shares one block. Copying an `Rc` (pass by value) only increments the count; the block is
//!   freed when the last `Rc` is destroyed or released.
//! - Never assign one `Box`/`Rc` to another with `=`: that copies the pointers and skips the count
//!   (a double free for `Box`, a wrong count for `Rc`). Pass by value, or use `share()` / `clone()`.
//!
//! This version holds raw bytes (`void*`). A generic `Box<T>` / `Rc<T>` can wrap it once the
//! generic struct syntax is confirmed.
//!
//! ```
//! let a: Rc = Rc(16);        // count 1
//! let n: u64 = a.refs();
//! ```

extern "C" fn malloc(size: u64) => void*;
extern "C" fn free(p: void*) => void;
extern "C" fn memcpy(dst: void*, src: void*, n: u64) => void*;

/// A uniquely owned heap block of `size` bytes.
struct Box {
    /// The block, or `nullptr` if empty or allocation failed.
    data: void* = nullptr;
    /// Size in bytes.
    size: u64 = 0;

    /// Allocates `n` bytes. Check `isValid()` afterwards.
    constructor(n: u64) {
        this.data = malloc(n);
        this.size = 0;
        if (this.data != nullptr) this.size = n;
    }

    /// Copy constructor: a deep copy, so each `Box` owns its own block.
    constructor(o: Box&) {
        this.data = nullptr;
        this.size = 0;
        if (o.data != nullptr) {
            this.data = malloc(o.size);
            if (this.data != nullptr) {
                memcpy(this.data, o.data, o.size);
                this.size = o.size;
            }
        }
    }

    /// Frees the block.
    ~destructor() {
        this.release();
    }

    /// `true` if the block was allocated.
    isValid() => bool { return this.data != nullptr; }

    /// The block's address.
    get() => void* { return this.data; }

    /// Frees the block now and leaves the `Box` empty. Safe to call twice.
    release() => void {
        if (this.data != nullptr) free(this.data);
        this.data = nullptr;
        this.size = 0;
    }
}

/// A shared, reference-counted heap block.
struct Rc {
    /// The shared block, or `nullptr` when empty.
    data: void* = nullptr;
    /// The shared count, a separate heap cell, or `nullptr` when empty.
    count: u64* = nullptr;

    /// Allocates a block of `n` bytes with a count of 1. Check `isValid()` afterwards.
    constructor(n: u64) {
        this.data = malloc(n);
        this.count = malloc(8) as u64*;
        if (this.data == nullptr || this.count == nullptr) {
            if (this.data != nullptr) free(this.data);
            if (this.count != nullptr) free(this.count as void*);
            this.data = nullptr;
            this.count = nullptr;
        } else {
            this.count[0] = 1;
        }
    }

    /// Copy constructor: shares the block and adds one to the count.
    constructor(o: Rc&) {
        this.data = o.data;
        this.count = o.count;
        if (this.count != nullptr) this.count[0] = this.count[0] + 1;
    }

    /// Drops this reference; frees the block when it was the last one.
    ~destructor() {
        this.release();
    }

    /// `true` if this `Rc` refers to a block.
    isValid() => bool { return this.data != nullptr; }

    /// The shared block's address.
    get() => void* { return this.data; }

    /// How many `Rc`s share the block (0 when empty).
    refs() => u64 {
        if (this.count == nullptr) return 0;
        return this.count[0];
    }

    /// Drops this reference now and leaves this `Rc` empty. Safe to call twice.
    release() => void {
        if (this.count != nullptr) {
            this.count[0] = this.count[0] - 1;
            if (this.count[0] == 0) {
                free(this.data);
                free(this.count as void*);
            }
        }
        this.data = nullptr;
        this.count = nullptr;
    }
}
