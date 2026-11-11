// standard/1/Text.os
// Tier 3: text helpers over char* / String (split, join, pad, parse, escape, utf8).
module Str = import "standard/1/String.os";
module L = import "standard/1/List.os";
module CMem = import "standard/1/c/mem.os";
module CStr = import "standard/1/c/string.os";

// Splits s on sep (non-empty). Each part is a newly allocated String: free each, then the list.
function split(s: char*, sep: char*, out: L.List<Str.String>*) => bool {
    let sl: u64 = CStr.strlen(sep);
    if (sl == 0) return false;
    let p: char* = s;
    while (true) {
        let q: char* = CStr.strstr(p, sep);
        let part: Str.String;
        if (q == nullptr) { part.set(p); out.push(part); return true; }
        part.appendN(p, (q as u64) - (p as u64));
        if (!out.push(part)) return false;
        p = (q as u64 + sl) as char*;
    }
    return true;
}

function freeParts(parts: L.List<Str.String>*) => void {
    let i: u64 = 0;
    while (i < parts.len) { parts.data[i].free(); i = i + 1; }
    parts.free();
}

function join(parts: L.List<Str.String>*, sep: char*, out: Str.String*) => bool {
    let i: u64 = 0;
    while (i < parts.len) {
        if (i > 0) out.append(sep);
        out.appendString(&parts.data[i]);
        i = i + 1;
    }
    return true;
}

function repeat(s: char*, n: u64, out: Str.String*) => bool {
    let i: u64 = 0;
    while (i < n) { if (!out.append(s)) return false; i = i + 1; }
    return true;
}

function padLeft(s: char*, width: u64, fill: char, out: Str.String*) => bool {
    let n: u64 = CStr.strlen(s);
    while (n < width) { out.appendChar(fill); n = n + 1; }
    return out.append(s);
}

function padRight(s: char*, width: u64, fill: char, out: Str.String*) => bool {
    let n: u64 = CStr.strlen(s);
    out.append(s);
    while (n < width) { out.appendChar(fill); n = n + 1; }
    return true;
}

function parseF64(s: char*, out: double*) => bool {
    let end: char* = nullptr;
    let v: double = CStr.strtod(s, &end);
    if (end == s) return false;
    *out = v;
    return true;
}

function parseU64(s: char*, base: int, out: u64*) => bool {
    let end: char* = nullptr;
    let v: u64 = CStr.strtoull(s, &end, base);
    if (end == s) return false;
    *out = v;
    return true;
}

function isDigit(c: int) => bool { return c >= 48 && c <= 57; }
function isAlpha(c: int) => bool { return (c >= 65 && c <= 90) || (c >= 97 && c <= 122); }
function isAlnum(c: int) => bool { return isDigit(c) || isAlpha(c); }
function isSpace(c: int) => bool { return c == 32 || (c >= 9 && c <= 13); }

// Escape " \ and control chars (JSON-style).
function escape(s: char*, out: Str.String*) => bool {
    let i: u64 = 0;
    while ((s[i] as int) != 0) {
        let c: int = s[i] as int;
        if (c == 34) out.append("\\\"");
        else if (c == 92) out.append("\\\\");
        else if (c == 10) out.append("\\n");
        else if (c == 13) out.append("\\r");
        else if (c == 9) out.append("\\t");
        else out.appendChar(c as char);
        i = i + 1;
    }
    return true;
}

// Appends the UTF-8 encoding of a code point.
function appendUtf8(cp: u32, out: Str.String*) => bool {
    if (cp < 0x80) return out.appendChar(cp as char);
    if (cp < 0x800) { out.appendChar((0xC0 | (cp >> 6)) as char); return out.appendChar((0x80 | (cp & 0x3F)) as char); }
    if (cp < 0x10000) {
        out.appendChar((0xE0 | (cp >> 12)) as char);
        out.appendChar((0x80 | ((cp >> 6) & 0x3F)) as char);
        return out.appendChar((0x80 | (cp & 0x3F)) as char);
    }
    out.appendChar((0xF0 | (cp >> 18)) as char);
    out.appendChar((0x80 | ((cp >> 12) & 0x3F)) as char);
    out.appendChar((0x80 | ((cp >> 6) & 0x3F)) as char);
    return out.appendChar((0x80 | (cp & 0x3F)) as char);
}

// Number of code points in a UTF-8 string.
function utf8Length(s: char*) => u64 {
    let n: u64 = 0; let i: u64 = 0;
    while ((s[i] as int) != 0) { if (((s[i] as int) & 0xC0) != 0x80) n = n + 1; i = i + 1; }
    return n;
}

// ---- hex / url encoding ----
function hexEncode(p: u8*, n: u64, out: Str.String*) => bool {
    let d: char* = "0123456789abcdef";
    let i: u64 = 0;
    while (i < n) { out.appendChar(d[(p[i] >> 4) as u64]); out.appendChar(d[(p[i] & 15) as u64]); i = i + 1; }
    return true;
}
function hexNibble(c: int) => int {
    if (c >= 48 && c <= 57) return c - 48;
    if (c >= 97 && c <= 102) return c - 87;
    if (c >= 65 && c <= 70) return c - 55;
    return -1;
}
// Decodes into out (n/2 bytes). false on odd length / bad digit.
function hexDecode(s: char*, out: L.List<u8>*) => bool {
    let i: u64 = 0;
    while ((s[i] as int) != 0) {
        let a: int = hexNibble(s[i] as int);
        let b: int = hexNibble(s[i + 1] as int);
        if (a < 0 || b < 0) return false;
        out.push((a * 16 + b) as u8);
        i = i + 2;
    }
    return true;
}
function urlEncode(s: char*, out: Str.String*) => bool {
    let d: char* = "0123456789ABCDEF";
    let i: u64 = 0;
    while ((s[i] as int) != 0) {
        let c: int = s[i] as int;
        if (isAlnum(c) || c == 45 || c == 95 || c == 46 || c == 126) out.appendChar(c as char);
        else { out.appendChar(37 as char); out.appendChar(d[(c >> 4) as u64]); out.appendChar(d[(c & 15) as u64]); }
        i = i + 1;
    }
    return true;
}
function urlDecode(s: char*, out: Str.String*) => bool {
    let i: u64 = 0;
    while ((s[i] as int) != 0) {
        let c: int = s[i] as int;
        if (c == 43) out.appendChar(32 as char);
        else if (c == 37) {
            let a: int = hexNibble(s[i + 1] as int);
            let b: int = hexNibble(s[i + 2] as int);
            if (a < 0 || b < 0) return false;
            out.appendChar((a * 16 + b) as char);
            i = i + 2;
        } else out.appendChar(c as char);
        i = i + 1;
    }
    return true;
}
