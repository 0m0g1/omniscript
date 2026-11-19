// browser/util.os  -- string/memory helpers shared by every layer.
// ADAPT: if CMem.malloc needs a u64, change the one call in alloc().
module CMem = import "standard/1/c/mem.os";

function slen(s: char*) => int {
    let n: int = 0;
    while ((s[n] as int) != 0) { n = n + 1; }
    return n;
}

function alloc(n: int) => void* {
    let p: u8* = CMem.malloc(n) as u8*;
    let i: int = 0;
    while (i < n) { p[i] = 0 as u8; i = i + 1; }
    return p as void*;
}

function newStr(n: int) => char* { return alloc(n + 1) as char*; }

function sub(s: char*, a: int, b: int) => char* {
    let n: int = b - a;
    if (n < 0) n = 0;
    let r: char* = newStr(n);
    let i: int = 0;
    while (i < n) { r[i] = s[a + i]; i = i + 1; }
    return r;
}

function dup(s: char*) => char* { return sub(s, 0, slen(s)); }

function cat(a: char*, b: char*) => char* {
    let la: int = slen(a);
    let lb: int = slen(b);
    let r: char* = newStr(la + lb);
    let i: int = 0;
    while (i < la) { r[i] = a[i]; i = i + 1; }
    i = 0;
    while (i < lb) { r[la + i] = b[i]; i = i + 1; }
    return r;
}

function streq(a: char*, b: char*) => bool {
    let i: int = 0;
    let same: bool = true;
    let more: bool = true;
    while (more) {
        let ca: int = a[i] as int;
        let cb: int = b[i] as int;
        if (ca != cb) { same = false; more = false; }
        else if (ca == 0) { more = false; }
        else { i = i + 1; }
    }
    return same;
}

function startsAt(s: char*, at: int, p: char*) => bool {
    let i: int = 0;
    let ok: bool = true;
    let more: bool = true;
    while (more) {
        let cp: int = p[i] as int;
        if (cp == 0) { more = false; }
        else if ((s[at + i] as int) != cp) { ok = false; more = false; }
        else { i = i + 1; }
    }
    return ok;
}

// index of needle in s at or after `from`, or -1
function indexOf(s: char*, from: int, needle: char*) => int {
    let n: int = slen(s);
    let i: int = from;
    let found: int = 0 - 1;
    while (i < n && found < 0) {
        if (startsAt(s, i, needle)) found = i;
        i = i + 1;
    }
    return found;
}

function lastIndexOfCh(s: char*, c: int) => int {
    let i: int = slen(s) - 1;
    let found: int = 0 - 1;
    while (i >= 0 && found < 0) {
        if ((s[i] as int) == c) found = i;
        i = i - 1;
    }
    return found;
}

function isSpace(c: int) => bool { return c == 32 || c == 9 || c == 10 || c == 13; }
function isDigit(c: int) => bool { return c >= 48 && c <= 57; }
function isAlpha(c: int) => bool { return (c >= 65 && c <= 90) || (c >= 97 && c <= 122) || c == 95 || c == 36; }
function isAlnum(c: int) => bool { return isAlpha(c) || isDigit(c); }

function lowerCh(c: int) => int {
    if (c >= 65 && c <= 90) return c + 32;
    return c;
}

function lower(s: char*) => char* {
    let n: int = slen(s);
    let r: char* = newStr(n);
    let i: int = 0;
    while (i < n) { r[i] = lowerCh(s[i] as int) as char; i = i + 1; }
    return r;
}

function trim(s: char*) => char* {
    let a: int = 0;
    let b: int = slen(s);
    while (a < b && isSpace(s[a] as int)) { a = a + 1; }
    while (b > a && isSpace(s[b - 1] as int)) { b = b - 1; }
    return sub(s, a, b);
}

function hexv(c: int) => int {
    if (c >= 48 && c <= 57) return c - 48;
    if (c >= 97 && c <= 102) return c - 87;
    if (c >= 65 && c <= 70) return c - 55;
    return 0 - 1;
}

function toNum(s: char*) => double {
    let i: int = 0;
    let neg: bool = false;
    if ((s[0] as int) == 45) { neg = true; i = 1; }
    let v: double = 0.0;
    while (isDigit(s[i] as int)) { v = v * 10.0 + ((s[i] as int) - 48) as double; i = i + 1; }
    if ((s[i] as int) == 46) {
        i = i + 1;
        let scale: double = 0.1;
        while (isDigit(s[i] as int)) { v = v + ((s[i] as int) - 48) as double * scale; scale = scale / 10.0; i = i + 1; }
    }
    if (neg) v = 0.0 - v;
    return v;
}

function fromInt(n: int) => char* {
    let out: char* = newStr(12);
    let v: int = n;
    let p: int = 0;
    if (v < 0) { out[p] = 45 as char; p = p + 1; v = 0 - v; }
    let digits: int = 0;
    let tmp: int = v;
    if (tmp == 0) digits = 1;
    while (tmp > 0) { digits = digits + 1; tmp = tmp / 10; }
    let k: int = digits - 1;
    while (k >= 0) { out[p + k] = (48 + v % 10) as char; v = v / 10; k = k - 1; }
    out[p + digits] = 0 as char;
    return out;
}

function fromNum(v: double) => char* {
    let iv: i64 = v as i64;
    if ((iv as double) == v) return fromInt(iv as int);
    // TODO: real float formatting; 2 decimals for now
    let neg: bool = v < 0.0;
    let a: double = v;
    if (neg) a = 0.0 - v;
    let whole: int = a as int;
    let frac: int = ((a - (whole as double)) * 100.0) as int;
    let s: char* = cat(fromInt(whole), ".");
    if (frac < 10) s = cat(s, "0");
    s = cat(s, fromInt(frac));
    if (neg) s = cat("-", s);
    return s;
}
