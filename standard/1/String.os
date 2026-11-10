// standard/1/String.os
// Owned, growable, always NUL-terminated byte string (UTF-8 safe, byte-indexed).
//
// OWNERSHIP RULES
//   - A String owns its buffer. Call free() when done (there is no destructor/defer yet).
//   - NEVER copy one String into another with `=`; that copies the pointer and double-frees.
//     Use a.clone(&b) instead.
//   - Methods that can fail return bool.
//
// Usage:
//   let s: String;
//   s.set("hello");
//   s.append(", world");
//   printf("%s (%llu bytes)\n", s.cstr(), s.length());
//   s.free();
//
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

function isSpaceChar(c: int) => bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

struct String {
    data: char* = nullptr;
    len: u64 = 0;
    cap: u64 = 0;

    // Make room for n bytes plus the terminator. Keeps existing contents.
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

    free() => void {
        if (this.data != nullptr) free(this.data as void*);
        this.data = nullptr;
        this.len = 0;
        this.cap = 0;
    }

    clear() => void {
        this.len = 0;
        if (this.data != nullptr) this.data[0] = 0 as char;
    }

    truncate(n: u64) => void {
        if (n < this.len) {
            this.len = n;
            this.data[n] = 0 as char;
        }
    }

    length() => u64 { return this.len; }
    isEmpty() => bool { return this.len == 0; }

    // Always a valid C string, even for an empty/unallocated String.
    cstr() => char* {
        if (this.data == nullptr) return "";
        return this.data;
    }

    // ---- building ----
    set(src: char*) => bool {
        this.clear();
        return this.append(src);
    }

    appendN(src: char*, n: u64) => bool {
        if (!this.reserve(this.len + n)) return false;
        memcpy(&this.data[this.len] as void*, src as void*, n);
        this.len = this.len + n;
        this.data[this.len] = 0 as char;
        return true;
    }

    append(src: char*) => bool { return this.appendN(src, strlen(src)); }
    appendString(other: String*) => bool { return this.appendN(other.cstr(), other.len); }

    appendChar(c: char) => bool {
        if (!this.reserve(this.len + 1)) return false;
        this.data[this.len] = c;
        this.len = this.len + 1;
        this.data[this.len] = 0 as char;
        return true;
    }

    appendInt(v: i64) => bool {
        let tmp: [32]char;
        snprintf(&tmp as char*, 32, "%lld", v);
        return this.append(&tmp as char*);
    }

    clone(out: String*) => bool {
        out.clear();
        return out.appendN(this.cstr(), this.len);
    }

    // ---- querying ----
    equals(other: String*) => bool {
        if (this.len != other.len) return false;
        return memcmp(this.cstr() as void*, other.cstr() as void*, this.len) == 0;
    }

    equalsC(s: char*) => bool { return strcmp(this.cstr(), s) == 0; }

    startsWith(prefix: char*) => bool {
        let n = strlen(prefix);
        if (n > this.len) return false;
        return memcmp(this.cstr() as void*, prefix as void*, n) == 0;
    }

    endsWith(suffix: char*) => bool {
        let n = strlen(suffix);
        if (n > this.len) return false;
        return memcmp(&this.data[this.len - n] as void*, suffix as void*, n) == 0;
    }

    // Byte index of the first match, or -1.
    indexOf(needle: char*) => i64 {
        let p = strstr(this.cstr(), needle);
        if (p == nullptr) return -1;
        return ((p as u64) - (this.cstr() as u64)) as i64;
    }

    contains(needle: char*) => bool { return this.indexOf(needle) >= 0; }

    // Copies [start, start+count) (clamped) into out.
    substring(start: u64, count: u64, out: String*) => bool {
        if (start > this.len) return false;
        let n: u64 = count;
        if (start + n > this.len) n = this.len - start;
        out.clear();
        return out.appendN(&this.data[start], n);
    }

    // Parses a whole-string base-10 integer. false if empty or trailing junk.
    parseInt(out: i64*) => bool {
        if (this.len == 0) return false;
        let end: char* = nullptr;
        let v: i64 = strtoll(this.cstr(), &end, 10);
        if (end == this.cstr()) return false;
        if ((*end as int) != 0) return false;
        *out = v;
        return true;
    }

    // ---- editing in place ----
    trim() => void {
        let start: u64 = 0;
        while (start < this.len && isSpaceChar(this.data[start] as int)) start = start + 1;
        let end: u64 = this.len;
        while (end > start && isSpaceChar(this.data[end - 1] as int)) end = end - 1;
        let n: u64 = end - start;
        if (start > 0 && n > 0) memmove(this.data as void*, &this.data[start] as void*, n);
        if (this.data != nullptr) {
            this.len = n;
            this.data[n] = 0 as char;
        }
    }

    toUpper() => void {
        for (let i: u64 = 0; i < this.len; i++) this.data[i] = toupper(this.data[i] as int) as char;
    }

    toLower() => void {
        for (let i: u64 = 0; i < this.len; i++) this.data[i] = tolower(this.data[i] as int) as char;
    }

    // Replaces every occurrence of `find` with `with`. false if find is empty or out of memory.
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
