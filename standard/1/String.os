//! Owned, growable, always NUL-terminated byte string (UTF-8 safe, byte-indexed).
//!
//! ## Ownership rules
//!
//! - A `String` owns its buffer. Call `free()` when you are done (there is no destructor or `defer` yet).
//! - Never copy one `String` into another with `=`: that copies the pointer and double-frees.
//!   Use `a.clone(&b)` instead.
//! - Methods that can fail return `bool`.
//!
//! ## Usage
//!
//! ```
//! let s: String;
//! s.set("hello");
//! s.append(", world");
//! printf("%s (%llu bytes)\n", s.cstr(), s.length());
//! s.free();
//! ```

// ASSUMPTIONS (patch if the compiler rejects them): i64/u64 exist; pointer <-> u64 casts work;
// `&x` on a `char*` gives `char**`; `&arr` on a [N]char array decays to `char*` via `as`.

extern "C" fn malloc(size: u64) => void*;
extern "C" fn realloc(p: void*, size: u64) => void*;
extern "C" fn free(p: void*) => void;
extern "C" fn memcpy(dst: void*, src: void*, n: u64) => void*;
extern "C" fn memmove(dst: void*, src: void*, n: u64) => void*;
extern "C" fn memcmp(a: void*, b: void*, n: u64) => int;
extern "C" fn strlen(s: char*) => u64;
extern "C" fn strcmp(a: char*, b: char*) => int;
extern "C" fn strstr(hay: char*, needle: char*) => char*;
extern "C" fn strtoll(s: char*, end: char**, base: int) => i64;
extern "C" fn snprintf(buf: char*, size: u64, fmt: char*, ...) => int;
extern "C" fn toupper(c: int) => int;
extern "C" fn tolower(c: int) => int;

/// `true` if `c` is a space, tab, newline or carriage return.
function isSpaceChar(c: int) => bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

/// An owned, growable, NUL-terminated byte string. See the ownership rules at the top of this page.
struct String {
    /// The buffer, or `nullptr` before the first allocation. Always NUL-terminated once allocated.
    data: char* = nullptr;
    /// Length in bytes, not counting the terminator.
    len: u64 = 0;
    /// Allocated size in bytes, including room for the terminator.
    cap: u64 = 0;

    /// Makes room for `n` bytes plus the terminator, keeping the existing contents.
    /// Returns `false` if the allocation fails.
    reserve(n: u64) => bool {
        let need: u64 = n + 1;
        if (need <= this.cap) return true;
        let newCap: u64 = this.cap;
        if (newCap == 0) newCap = 16;
        while (newCap < need) newCap = newCap * 2;
        let p = realloc(this.data as void*, newCap);
        if (p == nullptr) return false;
        this.data = p as char*;
        this.cap = newCap;
        this.data[this.len] = 0 as char;
        return true;
    }

    /// Releases the buffer and resets the string to empty. Call this when you are done with it.
    free() => void {
        if (this.data != nullptr) free(this.data as void*);
        this.data = nullptr;
        this.len = 0;
        this.cap = 0;
    }

    /// Empties the string but keeps the allocated buffer for reuse.
    clear() => void {
        this.len = 0;
        if (this.data != nullptr) this.data[0] = 0 as char;
    }

    /// Shortens the string to `n` bytes. Does nothing if it is already `n` bytes or shorter.
    truncate(n: u64) => void {
        if (n < this.len) {
            this.len = n;
            this.data[n] = 0 as char;
        }
    }

    /// Returns the length in bytes.
    length() => u64 { return this.len; }

    /// `true` if the string has no bytes.
    isEmpty() => bool { return this.len == 0; }

    /// Returns the contents as a C string. Always valid, even for an empty or unallocated `String`.
    cstr() => char* {
        if (this.data == nullptr) return "";
        return this.data;
    }

    // ---- building ----

    /// Replaces the contents with the C string `src`.
    set(src: char*) => bool {
        this.clear();
        return this.append(src);
    }

    /// Appends the first `n` bytes of `src`.
    appendN(src: char*, n: u64) => bool {
        if (!this.reserve(this.len + n)) return false;
        // FIX: parenthesize & before cast
        memcpy((&this.data[this.len]) as void*, src as void*, n);
        this.len = this.len + n;
        this.data[this.len] = 0 as char;
        return true;
    }

    /// Appends the C string `src`.
    append(src: char*) => bool { return this.appendN(src, strlen(src)); }

    /// Appends another `String`.
    appendString(other: String*) => bool { return this.appendN(other.cstr(), other.len); }

    /// Appends one byte.
    appendChar(c: char) => bool {
        if (!this.reserve(this.len + 1)) return false;
        this.data[this.len] = c;
        this.len = this.len + 1;
        this.data[this.len] = 0 as char;
        return true;
    }

    /// Appends the decimal text of `v`.
    appendInt(v: i64) => bool {
        let tmp: [32]char;
        // FIX: parenthesize & before cast
        snprintf((&tmp) as char*, 32, "%lld", v);
        return this.append((&tmp) as char*);
    }

    /// Copies this string into `out`, replacing its contents. Use this instead of `=`.
    clone(out: String*) => bool {
        out.clear();
        return out.appendN(this.cstr(), this.len);
    }

    // ---- querying ----

    /// `true` if both strings have the same bytes.
    equals(other: String*) => bool {
        if (this.len != other.len) return false;
        return memcmp(this.cstr() as void*, other.cstr() as void*, this.len) == 0;
    }

    /// `true` if the string equals the C string `s`.
    equalsC(s: char*) => bool { return strcmp(this.cstr(), s) == 0; }

    /// `true` if the string begins with `prefix`.
    startsWith(prefix: char*) => bool {
        let n = strlen(prefix);
        if (n > this.len) return false;
        return memcmp(this.cstr() as void*, prefix as void*, n) == 0;
    }

    /// `true` if the string ends with `suffix`.
    endsWith(suffix: char*) => bool {
        let n = strlen(suffix);
        if (n > this.len) return false;
        // FIX: parenthesize & before cast
        return memcmp((&this.data[this.len - n]) as void*, suffix as void*, n) == 0;
    }

    /// Returns the byte index of the first match of `needle`, or `-1` if there is none.
    indexOf(needle: char*) => i64 {
        let p = strstr(this.cstr(), needle);
        if (p == nullptr) return -1;
        return ((p as u64) - (this.cstr() as u64)) as i64;
    }

    /// `true` if `needle` occurs anywhere in the string.
    contains(needle: char*) => bool { return this.indexOf(needle) >= 0; }

    /// Copies the bytes from `start` to `start + count` (clamped to the end) into `out`,
    /// replacing its contents. Returns `false` if `start` is past the end.
    substring(start: u64, count: u64, out: String*) => bool {
        if (start > this.len) return false;
        let n: u64 = count;
        if (start + n > this.len) n = this.len - start;
        out.clear();
        return out.appendN(&this.data[start], n);
    }

    /// Parses the whole string as a base-10 integer into `out[0]`.
    /// Returns `false` if the string is empty or has trailing characters.
    parseInt(out: i64*) => bool {
        if (this.len == 0) return false;
        let end: char* = nullptr;
        let v: i64 = strtoll(this.cstr(), &end, 10);
        if (end == this.cstr()) return false;
        if ((*end as int) != 0) return false;
        out[0] = v;
        return true;
    }

    // ---- editing in place ----

    /// Removes leading and trailing whitespace, in place.
    trim() => void {
        let start: u64 = 0;
        while (start < this.len && isSpaceChar(this.data[start] as int)) start = start + 1;
        let end: u64 = this.len;
        while (end > start && isSpaceChar(this.data[end - 1] as int)) end = end - 1;
        let n: u64 = end - start;
        // FIX: parenthesize & before cast
        if (start > 0 && n > 0) memmove(this.data as void*, (&this.data[start]) as void*, n);
        if (this.data != nullptr) {
            this.len = n;
            this.data[n] = 0 as char;
        }
    }

    /// Converts every byte to uppercase, in place (ASCII).
    toUpper() => void {
        for (let i: u64 = 0; i < this.len; i++) this.data[i] = toupper(this.data[i] as int) as char;
    }

    /// Converts every byte to lowercase, in place (ASCII).
    toLower() => void {
        for (let i: u64 = 0; i < this.len; i++) this.data[i] = tolower(this.data[i] as int) as char;
    }

    /// Replaces every occurrence of `find` with `with`, in place.
    /// Returns `false` if `find` is empty or memory runs out (the string is left unchanged).
    replace(find: char*, with: char*) => bool {
        let flen = strlen(find);
        if (flen == 0) return false;
        let result: String;
        let ok: bool = true;
        let cur: char* = this.cstr();
        let done: bool = false;
        while (!done && ok) {
            let p = strstr(cur, find);
            if (p == nullptr) {
                ok = result.append(cur);
                done = true;
            } else {
                ok = result.appendN(cur, (p as u64) - (cur as u64));
                if (ok) ok = result.append(with);
                cur = &p[flen];
            }
        }
        if (!ok) {
            result.free();
            return false;
        }
        if (this.data != nullptr) free(this.data as void*);
        this.data = result.data;
        this.len = result.len;
        this.cap = result.cap;
        return true;
    }
}
