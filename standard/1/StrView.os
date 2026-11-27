//! Non-owning string view: a pointer and a length into text that someone else owns.
//! Slicing and trimming never allocate and never copy. The view is only valid while the text it
//! points into is alive (a string literal, a `String`, a buffer). Nothing to free, no destructor.
//!
//! ```
//! let v: SV.StrView = SV.view("key = value");
//! let eq: int = v.indexOf("=");
//! let key: SV.StrView = v.slice(0, eq).trim();
//! ```

extern "C" fn memcmp(a: void*, b: void*, n: u64) => int;
extern "C" fn strlen(s: char*) => u64;

function spaceChar(c: int) => bool {
    return c == 32 || (c >= 9 && c <= 13);
}

struct StrView {
    /// First byte of the view. The text is NOT NUL-terminated at `len`.
    data: char* = nullptr;
    /// Length in bytes.
    len: u64 = 0;

    /// `true` if the view has no bytes.
    isEmpty() => bool { return this.len == 0; }

    /// The bytes from `from` up to (not including) `to`, clamped to the view. No allocation.
    slice(from: u64, to: u64) => StrView {
        let a: u64 = from;
        let b: u64 = to;
        if (b > this.len) b = this.len;
        if (a > b) a = b;
        let out: StrView;
        out.data = &this.data[a];
        out.len = b - a;
        return out;
    }

    /// The view without leading and trailing whitespace. No allocation.
    trim() => StrView {
        let a: u64 = 0;
        let b: u64 = this.len;
        while (a < b && spaceChar(this.data[a] as int)) a = a + 1;
        while (b > a && spaceChar(this.data[b - 1] as int)) b = b - 1;
        return this.slice(a, b);
    }

    /// Byte index of the first match of `needle`, or -1 if there is none.
    indexOf(needle: char*) => int {
        let n: u64 = strlen(needle);
        if (n == 0) return 0;
        if (n > this.len) return -1;
        let i: u64 = 0;
        while (i + n <= this.len) {
            if (memcmp((&this.data[i]) as void*, needle as void*, n) == 0) return i as int;
            i = i + 1;
        }
        return -1;
    }

    /// `true` if the view equals the C string `s`.
    equalsC(s: char*) => bool {
        let n: u64 = strlen(s);
        if (n != this.len) return false;
        if (n == 0) return true;
        return memcmp(this.data as void*, s as void*, n) == 0;
    }
}

/// A view of the whole C string `s`.
function view(s: char*) => StrView {
    let out: StrView;
    out.data = s;
    out.len = strlen(s);
    return out;
}
