// standard/1/Uuid.os
// Tier 4: UUID v4 (random) and v7-style time-ordered. (PLATFORM: Win32 BCryptGenRandom)
module Str = import "standard/1/String.os";
module Bin = import "standard/1/Binary.os";

extern "C" fn BCryptGenRandom(alg: void*, buf: u8*, n: u32, flags: u32) => i32;
extern "C" fn time(t: i64*) => i64;

struct Uuid { b: [16]u8; }

function randomBytes(p: u8*, n: u32) => bool { return BCryptGenRandom(nullptr, p, n, 2) == 0; }  // 2 = USE_SYSTEM_PREFERRED_RNG

function v4(out: Uuid*) => bool {
    if (!randomBytes(&out.b as u8*, 16)) return false;
    out.b[6] = (out.b[6] & 0x0F) | 0x40;
    out.b[8] = (out.b[8] & 0x3F) | 0x80;
    return true;
}

function v7(out: Uuid*) => bool {
    if (!randomBytes(&out.b as u8*, 16)) return false;
    let ms: u64 = (time(nullptr) as u64) * 1000;
    let i: u64 = 0;
    while (i < 6) { out.b[i] = ((ms >> ((5 - i) * 8)) & 0xFF) as u8; i = i + 1; }
    out.b[6] = (out.b[6] & 0x0F) | 0x70;
    out.b[8] = (out.b[8] & 0x3F) | 0x80;
    return true;
}

// Canonical 8-4-4-4-12 lowercase form.
function toString(u: Uuid*, out: Str.String*) => bool {
    let hex: [33]char;
    Bin.toHex(&u.b as u8*, 16, &hex as char*);
    let i: u64 = 0;
    while (i < 32) {
        if (i == 8 || i == 12 || i == 16 || i == 20) out.appendChar(45 as char);
        out.appendChar(hex[i]);
        i = i + 1;
    }
    return true;
}

function hexVal(c: int) => int {
    if (c >= 48 && c <= 57) return c - 48;
    if (c >= 97 && c <= 102) return c - 87;
    if (c >= 65 && c <= 70) return c - 55;
    return -1;
}

function parse(s: char*, out: Uuid*) => bool {
    let n: u64 = 0; let i: u64 = 0;
    while ((s[i] as int) != 0) {
        if ((s[i] as int) == 45) { i = i + 1; continue; }
        let hi: int = hexVal(s[i] as int);
        let lo: int = hexVal(s[i + 1] as int);
        if (hi < 0 || lo < 0 || n >= 16) return false;
        out.b[n] = (hi * 16 + lo) as u8;
        n = n + 1; i = i + 2;
    }
    return n == 16;
}
