// standard/1/Ui/Util.os
// Small helpers: min/max, C-string functions, a tiny lexer used by the CSS parser and attribute parsing.
// No `return` inside loop bodies: loops set a result variable instead.

function minF(a: float, b: float) => float { if (a < b) return a; return b; }
function maxF(a: float, b: float) => float { if (a > b) return a; return b; }
function minI(a: int, b: int) => int { if (a < b) return a; return b; }
function maxI(a: int, b: int) => int { if (a > b) return a; return b; }
function clampF(v: float, lo: float, hi: float) => float { if (v < lo) return lo; if (v > hi) return hi; return v; }
function lowerC(c: int) => int { if (c >= 65 && c <= 90) return c + 32; return c; }
function isSpaceC(c: int) => bool { return c == 32 || c == 9 || c == 10 || c == 13 || c == 12; }
function isDigitC(c: int) => bool { return c >= 48 && c <= 57; }
function isNameC(c: int) => bool {
    return (c >= 97 && c <= 122) || (c >= 65 && c <= 90) || (c >= 48 && c <= 57) || c == 45 || c == 95;
}

function strLen(s: char*) => int {
    let n: int = 0;
    while (s[n] as int != 0) n = n + 1;
    return n;
}

function strEq(a: char*, b: char*) => bool {
    let i: int = 0; let same: bool = true; let more: bool = true;
    while (more) {
        let x: int = a[i] as int; let y: int = b[i] as int;
        if (x != y) { same = false; more = false; }
        else if (x == 0) { more = false; }
        i = i + 1;
    }
    return same;
}

function strEqI(a: char*, b: char*) => bool {
    let i: int = 0; let same: bool = true; let more: bool = true;
    while (more) {
        let x: int = lowerC(a[i] as int); let y: int = lowerC(b[i] as int);
        if (x != y) { same = false; more = false; }
        else if (x == 0) { more = false; }
        i = i + 1;
    }
    return same;
}

// does s (from index `at`) start with the literal p? (case-insensitive)
function eqAtI(s: char*, at: int, p: char*) => bool {
    let i: int = 0; let ok: bool = true;
    while (ok && p[i] as int != 0) {
        if (lowerC(s[at + i] as int) != lowerC(p[i] as int)) ok = false;
        i = i + 1;
    }
    return ok;
}

function startsWith(s: char*, p: char*) => bool { return eqAtI(s, 0, p); }

function strDup(s: char*) => char* {
    let n: int = strLen(s);
    let p: char* = CMem.malloc((n + 1) as u64) as char*;
    let i: int = 0;
    while (i < n) { p[i] = s[i]; i = i + 1; }
    p[n] = 0 as char;
    return p;
}

function strLowerDup(s: char*) => char* {
    let n: int = strLen(s);
    let p: char* = CMem.malloc((n + 1) as u64) as char*;
    let i: int = 0;
    while (i < n) { p[i] = lowerC(s[i] as int) as char; i = i + 1; }
    p[n] = 0 as char;
    return p;
}

// copy of s[from .. from+n)
function strSub(s: char*, from: int, n: int) => char* {
    let p: char* = CMem.malloc((n + 1) as u64) as char*;
    let i: int = 0;
    while (i < n) { p[i] = s[from + i]; i = i + 1; }
    p[n] = 0 as char;
    return p;
}

// a + b as a new string (a or b may be nullptr)
function strCat(a: char*, b: char*) => char* {
    let la: int = 0; let lb: int = 0;
    if (a != nullptr) la = strLen(a);
    if (b != nullptr) lb = strLen(b);
    let p: char* = CMem.malloc((la + lb + 1) as u64) as char*;
    let i: int = 0;
    while (i < la) { p[i] = a[i]; i = i + 1; }
    i = 0;
    while (i < lb) { p[la + i] = b[i]; i = i + 1; }
    p[la + lb] = 0 as char;
    return p;
}

function strFree(p: char*) => void { if (p != nullptr) CMem.free(p as void*); }

function isBlank(s: char*) => bool {
    let blank: bool = true;
    if (s != nullptr) {
        let i: int = 0;
        while (s[i] as int != 0) { if (!isSpaceC(s[i] as int)) blank = false; i = i + 1; }
    }
    return blank;
}

// Is `name` one of the whitespace-separated tokens in `list`? (exact match; callers lowercase tag names)
function inList(name: char*, list: char*) => bool {
    let found: bool = false; let i: int = 0; let more: bool = true;
    while (more) {
        while (list[i] as int == 32) i = i + 1;
        if (list[i] as int == 0) { more = false; }
        else {
            let j: int = 0; let same: bool = true;
            while (list[i + j] as int != 32 && list[i + j] as int != 0) {
                if (same) { if (name[j] as int != list[i + j] as int) same = false; }
                j = j + 1;
            }
            if (same && name[j] as int == 0) { found = true; more = false; }
            i = i + j;
        }
    }
    return found;
}

// Is the token s[from .. from+len) one of the whitespace-separated tokens in `list`?
function hasToken(list: char*, s: char*, from: int, len: int) => bool {
    let found: bool = false;
    if (list != nullptr) {
        let i: int = 0; let more: bool = true;
        while (more) {
            while (list[i] as int == 32) i = i + 1;
            if (list[i] as int == 0) { more = false; }
            else {
                let j: int = 0;
                while (list[i + j] as int != 32 && list[i + j] as int != 0) j = j + 1;
                if (j == len) {
                    let k: int = 0; let same: bool = true;
                    while (k < len) { if (list[i + k] as int != s[from + k] as int) same = false; k = k + 1; }
                    if (same) found = true;
                }
                i = i + j;
            }
        }
    }
    return found;
}

// every token of `req` is present in `have`
function hasAllTokens(have: char*, req: char*) => bool {
    let ok: bool = true; let i: int = 0; let more: bool = true;
    while (more) {
        while (req[i] as int == 32) i = i + 1;
        if (req[i] as int == 0) { more = false; }
        else {
            let j: int = 0;
            while (req[i + j] as int != 32 && req[i + j] as int != 0) j = j + 1;
            if (!hasToken(have, req, i, j)) ok = false;
            i = i + j;
        }
    }
    return ok;
}

// alpha-scale a 0xRRGGBBAA colour
function fadeColor(c: u32, op: float) => u32 {
    if (op >= 0.999) return c;
    let a: float = ((c & 0xFF) as float) * op;
    let ai: u32 = a as u32;
    return (c & 0xFFFFFF00) | ai;
}

// Tiny lexer. Results come back through fields (`ok`, `val`) because the language has no out-params for scalars.
struct Lex {
    s: char* = nullptr;
    i: int = 0;
    ok: bool = false;
    val: float = 0.0;

    peek() => int { return this.s[this.i] as int; }
    skipWs() => void { while (isSpaceC(this.s[this.i] as int)) this.i = this.i + 1; }
    atEnd() => bool { return this.s[this.i] as int == 0; }

    // [+-]digits[.digits]; sets ok and val
    number() => float {
        let neg: bool = false; let v: float = 0.0; let any: bool = false;
        let c: int = this.peek();
        if (c == 45) { neg = true; this.i = this.i + 1; }
        else if (c == 43) { this.i = this.i + 1; }
        while (isDigitC(this.peek())) { v = v * 10.0 + ((this.peek() - 48) as float); this.i = this.i + 1; any = true; }
        if (this.peek() == 46) {
            this.i = this.i + 1;
            let f: float = 0.1;
            while (isDigitC(this.peek())) { v = v + f * ((this.peek() - 48) as float); f = f * 0.1; this.i = this.i + 1; any = true; }
        }
        if (neg) v = 0.0 - v;
        this.ok = any; this.val = v;
        return v;
    }

    // Next whitespace-separated token (parentheses group). Copies into buf (cap bytes). false at end.
    token(buf: char*, cap: int) => bool {
        this.skipWs();
        if (this.atEnd()) return false;
        let n: int = 0; let depth: int = 0; let more: bool = true;
        while (more) {
            let c: int = this.peek();
            if (c == 0) { more = false; }
            else if (depth == 0 && isSpaceC(c)) { more = false; }
            else {
                if (c == 40) depth = depth + 1;
                if (c == 41 && depth > 0) depth = depth - 1;
                if (n < cap - 1) { buf[n] = c as char; n = n + 1; }
                this.i = this.i + 1;
            }
        }
        buf[n] = 0 as char;
        return true;
    }
}

function parseFloatStr(s: char*, def: float) => float {
    let lx: Lex;
    lx.s = s; lx.i = 0; lx.ok = false; lx.val = 0.0;
    lx.skipWs();
    let v: float = lx.number();
    if (!lx.ok) v = def;
    return v;
}
