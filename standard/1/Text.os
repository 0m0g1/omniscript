//! Text helpers over `char*` and `String`: split, join, pad, parse, escape, UTF-8, hex, URL and base64.
//!
//! Tier 3. Functions that build text append into a `String` you own (see `String.os`), so call
//! `free()` on it when you are done. Functions that can fail return `bool`.
module Str = import "standard/1/String.os";
module L = import "standard/1/List.os";
module CMem = import "standard/1/c/mem.os";
module CStr = import "standard/1/c/string.os";

/// Splits `s` on every occurrence of `sep` and pushes each part onto `out`.
///
/// Each part is a newly allocated `String`: free each one, then the list (or call `freeParts`).
/// Returns `false` if `sep` is empty or an allocation fails.
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

/// Frees every `String` in `parts`, then the list itself. Use it on a list filled by `split`.
function freeParts(parts: L.List<Str.String>*) => void {
    let i: u64 = 0;
    while (i < parts.len) { parts.data[i].free(); i = i + 1; }
    parts.free();
}

/// Appends all `parts` to `out`, separated by `sep`.
function join(parts: L.List<Str.String>*, sep: char*, out: Str.String*) => bool {
    let i: u64 = 0;
    while (i < parts.len) {
        if (i > 0) out.append(sep);
        out.appendString(&parts.data[i]);
        i = i + 1;
    }
    return true;
}

/// Appends `s` to `out`, `n` times.
function repeat(s: char*, n: u64, out: Str.String*) => bool {
    let i: u64 = 0;
    while (i < n) { if (!out.append(s)) return false; i = i + 1; }
    return true;
}

/// Appends `s` to `out`, left-padded with `fill` until it is at least `width` bytes long.
function padLeft(s: char*, width: u64, fill: char, out: Str.String*) => bool {
    let n: u64 = CStr.strlen(s);
    while (n < width) { out.appendChar(fill); n = n + 1; }
    return out.append(s);
}

/// Appends `s` to `out`, right-padded with `fill` until it is at least `width` bytes long.
function padRight(s: char*, width: u64, fill: char, out: Str.String*) => bool {
    let n: u64 = CStr.strlen(s);
    out.append(s);
    while (n < width) { out.appendChar(fill); n = n + 1; }
    return true;
}

/// Parses a floating-point number from the start of `s` into `out[0]`.
/// Returns `false` if no number could be read.
function parseF64(s: char*, out: double*) => bool {
    let end: char* = nullptr;
    let v: double = CStr.strtod(s, &end);
    if (end == s) return false;
    out[0] = v;
    return true;
}

/// Parses an unsigned integer in the given `base` (2 to 36) from the start of `s` into `out[0]`.
/// Returns `false` if no digits could be read.
function parseU64(s: char*, base: int, out: u64*) => bool {
    let end: char* = nullptr;
    let v: u64 = CStr.strtoull(s, &end, base);
    if (end == s) return false;
    out[0] = v;
    return true;
}

/// `true` if `c` is an ASCII digit (`0` to `9`).
function isDigit(c: int) => bool { return c >= 48 && c <= 57; }

/// `true` if `c` is an ASCII letter.
function isAlpha(c: int) => bool { return (c >= 65 && c <= 90) || (c >= 97 && c <= 122); }

/// `true` if `c` is an ASCII letter or digit.
function isAlnum(c: int) => bool { return isDigit(c) || isAlpha(c); }

/// `true` if `c` is ASCII whitespace (space, tab, newline, vertical tab, form feed, carriage return).
function isSpace(c: int) => bool { return c == 32 || (c >= 9 && c <= 13); }

/// Appends `s` to `out` with `"`, `\` and the control characters newline, carriage return and tab
/// escaped, JSON-style.
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

/// Appends the UTF-8 encoding of the code point `cp` to `out` (1 to 4 bytes).
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

/// Returns the number of code points in the UTF-8 string `s` (not the number of bytes).
function utf8Length(s: char*) => u64 {
    let n: u64 = 0; let i: u64 = 0;
    while ((s[i] as int) != 0) { if (((s[i] as int) & 0xC0) != 0x80) n = n + 1; i = i + 1; }
    return n;
}

// ---- digit helpers: computed, not looked up in a string-literal table ----

/// Returns the hex digit character for `n` in 0 to 15, lowercase or uppercase.
function hexDigit(n: int, upper: bool) => int {
    if (n < 10) return 48 + n;
    if (upper) return 55 + n;
    return 87 + n;
}

/// Returns the base64 alphabet character for `v` in 0 to 63.
/// Standard alphabet uses `+` and `/`; with `urlSafe` it uses `-` and `_`.
function b64Char(v: int, urlSafe: bool) => int {
    if (v < 26) return 65 + v;
    if (v < 52) return 71 + v;
    if (v < 62) return v - 4;
    if (v == 62) { if (urlSafe) return 45; return 43; }
    if (urlSafe) return 95;
    return 47;
}

// ---- hex / url encoding ----

/// Appends the lowercase hex encoding of the `n` bytes at `p` to `out`.
function hexEncode(p: u8*, n: u64, out: Str.String*) => bool {
    let i: u64 = 0;
    while (i < n) {
        let b: int = p[i] as int;
        out.appendChar(hexDigit(b >> 4, false) as char);
        out.appendChar(hexDigit(b & 15, false) as char);
        i = i + 1;
    }
    return true;
}

/// Returns the value (0 to 15) of the hex digit character `c`, or `-1` if it is not a hex digit.
function hexNibble(c: int) => int {
    if (c >= 48 && c <= 57) return c - 48;
    if (c >= 97 && c <= 102) return c - 87;
    if (c >= 65 && c <= 70) return c - 55;
    return -1;
}

/// Decodes the hex string `s` and pushes the bytes onto `out` (half as many bytes as characters).
/// Returns `false` on an odd length or a character that is not a hex digit.
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

/// Appends the percent-encoding of `s` to `out`.
/// Letters, digits and `-` `_` `.` `~` are kept; every other byte becomes `%XX`.
function urlEncode(s: char*, out: Str.String*) => bool {
    let i: u64 = 0;
    while ((s[i] as int) != 0) {
        let c: int = (s[i] as int) & 255;
        if (isAlnum(c) || c == 45 || c == 95 || c == 46 || c == 126) out.appendChar(c as char);
        else {
            out.appendChar(37 as char);
            out.appendChar(hexDigit(c >> 4, true) as char);
            out.appendChar(hexDigit(c & 15, true) as char);
        }
        i = i + 1;
    }
    return true;
}

/// Appends the percent-decoding of `s` to `out`. `+` becomes a space.
/// Returns `false` on a malformed `%XX` sequence.
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

// ---- Text additions: base64 (standard + URL-safe, pure OmniScript, no OpenSSL), hex of a C string ----

/// Appends the base64 encoding of the `n` bytes at `p` to `out`.
///
/// With `urlSafe` the alphabet uses `-` and `_` instead of `+` and `/`.
/// With `pad` the output is padded with `=` to a multiple of 4 characters.
function base64EncodeOpts(p: u8*, n: u64, urlSafe: bool, pad: bool, out: Str.String*) => bool {
    let ok: bool = true;
    let i: u64 = 0;
    while (ok && i + 3 <= n) {
        let v: u32 = ((p[i] as u32) << 16) | ((p[i + 1] as u32) << 8) | (p[i + 2] as u32);
        ok = out.appendChar(b64Char(((v >> 18) & 63) as int, urlSafe) as char);
        if (ok) ok = out.appendChar(b64Char(((v >> 12) & 63) as int, urlSafe) as char);
        if (ok) ok = out.appendChar(b64Char(((v >> 6) & 63) as int, urlSafe) as char);
        if (ok) ok = out.appendChar(b64Char((v & 63) as int, urlSafe) as char);
        i = i + 3;
    }
    let rem: u64 = n - i;
    if (ok && rem == 1) {
        let v1: u32 = (p[i] as u32) << 16;
        ok = out.appendChar(b64Char(((v1 >> 18) & 63) as int, urlSafe) as char);
        if (ok) ok = out.appendChar(b64Char(((v1 >> 12) & 63) as int, urlSafe) as char);
        if (ok && pad) ok = out.append("==");
    }
    if (ok && rem == 2) {
        let v2: u32 = ((p[i] as u32) << 16) | ((p[i + 1] as u32) << 8);
        ok = out.appendChar(b64Char(((v2 >> 18) & 63) as int, urlSafe) as char);
        if (ok) ok = out.appendChar(b64Char(((v2 >> 12) & 63) as int, urlSafe) as char);
        if (ok) ok = out.appendChar(b64Char(((v2 >> 6) & 63) as int, urlSafe) as char);
        if (ok && pad) ok = out.appendChar(61 as char);
    }
    return ok;
}

/// Appends the standard, padded base64 encoding of the `n` bytes at `p` to `out`.
function base64Encode(p: u8*, n: u64, out: Str.String*) => bool {
    return base64EncodeOpts(p, n, false, true, out);
}

/// Appends the standard, padded base64 encoding of the C string `s` to `out`.
function base64EncodeStr(s: char*, out: Str.String*) => bool {
    return base64EncodeOpts(s as u8*, CStr.strlen(s), false, true, out);
}

/// Returns the value (0 to 63) of the base64 character `c` (standard or URL-safe),
/// or `-1` if it is not a base64 character.
function b64Val(c: int) => int {
    if (c >= 65 && c <= 90) return c - 65;
    if (c >= 97 && c <= 122) return c - 71;
    if (c >= 48 && c <= 57) return c + 4;
    if (c == 43 || c == 45) return 62;
    if (c == 47 || c == 95) return 63;
    return -1;
}

/// Decodes standard or URL-safe base64 from `s` and pushes the bytes onto `out`.
///
/// Padding is optional and ASCII whitespace is ignored.
/// Returns `false` on an invalid character, data after `=`, or an impossible length.
function base64Decode(s: char*, out: L.List<u8>*) => bool {
    let ok: bool = true;
    let acc: u32 = 0;
    let bits: int = 0;
    let count: u64 = 0;
    let seenPad: bool = false;
    let i: u64 = 0;
    while (ok && (s[i] as int) != 0) {
        let c: int = s[i] as int;
        if (!isSpace(c)) {
            if (c == 61) {
                seenPad = true;
            } else {
                let v: int = b64Val(c);
                if (v < 0 || seenPad) {
                    ok = false;
                } else {
                    acc = (acc << 6) | (v as u32);
                    bits = bits + 6;
                    count = count + 1;
                    if (bits >= 8) {
                        bits = bits - 8;
                        ok = out.push(((acc >> (bits as u32)) & 255) as u8);
                    }
                }
            }
        }
        i = i + 1;
    }
    if (ok && (count % 4) == 1) ok = false;
    return ok;
}

/// Decodes base64 from `s` into a `String`, for text payloads. See `base64Decode` for the rules.
function base64DecodeStr(s: char*, out: Str.String*) => bool {
    let tmp: L.List<u8>;
    let ok: bool = base64Decode(s, &tmp);
    if (ok && tmp.len > 0) ok = out.appendN(tmp.data as char*, tmp.len);
    tmp.free();
    return ok;
}

/// Appends the lowercase hex encoding of the bytes of the C string `s` to `out`.
function hexEncodeStr(s: char*, out: Str.String*) => bool {
    return hexEncode(s as u8*, CStr.strlen(s), out);
}
