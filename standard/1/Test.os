// standard/1/Test.os
// Tiny test harness. Never panics: every failure prints one line and the run continues.
//   FAIL [Module.function] what was checked        <- grep FAIL to triage
// Usage:  Test.check(cond, "Random.range", "stays in [5,10)");   ...   return Test.summary();

extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn strcmp(a: char*, b: char*) => int;

let g_pass: int = 0;
let g_fail: int = 0;

function check(cond: bool, tag: char*, what: char*) => void {
    if (cond) {
        g_pass = g_pass + 1;
    } else {
        g_fail = g_fail + 1;
        printf("FAIL [%s] %s\n", tag, what);
        fflush(nullptr);
    }
}

function eqI64(got: i64, want: i64, tag: char*, what: char*) => void {
    if (got == want) {
        g_pass = g_pass + 1;
    } else {
        g_fail = g_fail + 1;
        printf("FAIL [%s] %s (got %lld, want %lld)\n", tag, what, got, want);
        fflush(nullptr);
    }
}

function eqF64(got: double, want: double, eps: double, tag: char*, what: char*) => void {
    let d = got - want;
    if (d < 0.0) d = 0.0 - d;
    if (d <= eps) {
        g_pass = g_pass + 1;
    } else {
        g_fail = g_fail + 1;
        printf("FAIL [%s] %s (got %f, want %f)\n", tag, what, got, want);
        fflush(nullptr);
    }
}

function eqStr(got: char*, want: char*, tag: char*, what: char*) => void {
    if (strcmp(got, want) == 0) {
        g_pass = g_pass + 1;
    } else {
        g_fail = g_fail + 1;
        printf("FAIL [%s] %s (got \"%s\", want \"%s\")\n", tag, what, got, want);
        fflush(nullptr);
    }
}

function summary() => i32 {
    printf("%d passed, %d failed\n", g_pass, g_fail);
    fflush(nullptr);
    if (g_fail > 0) return 1;
    return 0;
}

// ---- Text additions: base64 (standard + URL-safe, pure OmniScript, no OpenSSL), hex of a C string ----

// Encodes n bytes. urlSafe uses -_ instead of +/ ; pad appends '=' to a multiple of 4.
function base64EncodeOpts(p: u8*, n: u64, urlSafe: bool, pad: bool, out: Str.String*) => bool {
    let tbl: char* = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if (urlSafe) tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    let ok: bool = true;
    let i: u64 = 0;
    while (ok && i + 3 <= n) {
        let v: u32 = ((p[i] as u32) << 16) | ((p[i + 1] as u32) << 8) | (p[i + 2] as u32);
        ok = out.appendChar(tbl[((v >> 18) & 63) as u64]);
        if (ok) ok = out.appendChar(tbl[((v >> 12) & 63) as u64]);
        if (ok) ok = out.appendChar(tbl[((v >> 6) & 63) as u64]);
        if (ok) ok = out.appendChar(tbl[(v & 63) as u64]);
        i = i + 3;
    }
    let rem: u64 = n - i;
    if (ok && rem == 1) {
        let v1: u32 = (p[i] as u32) << 16;
        ok = out.appendChar(tbl[((v1 >> 18) & 63) as u64]);
        if (ok) ok = out.appendChar(tbl[((v1 >> 12) & 63) as u64]);
        if (ok && pad) ok = out.append("==");
    }
    if (ok && rem == 2) {
        let v2: u32 = ((p[i] as u32) << 16) | ((p[i + 1] as u32) << 8);
        ok = out.appendChar(tbl[((v2 >> 18) & 63) as u64]);
        if (ok) ok = out.appendChar(tbl[((v2 >> 12) & 63) as u64]);
        if (ok) ok = out.appendChar(tbl[((v2 >> 6) & 63) as u64]);
        if (ok && pad) ok = out.appendChar(61 as char);
    }
    return ok;
}

function base64Encode(p: u8*, n: u64, out: Str.String*) => bool {
    return base64EncodeOpts(p, n, false, true, out);
}

function base64EncodeStr(s: char*, out: Str.String*) => bool {
    return base64EncodeOpts(s as u8*, CStr.strlen(s), false, true, out);
}

function b64Val(c: int) => int {
    if (c >= 65 && c <= 90) return c - 65;
    if (c >= 97 && c <= 122) return c - 71;
    if (c >= 48 && c <= 57) return c + 4;
    if (c == 43 || c == 45) return 62;
    if (c == 47 || c == 95) return 63;
    return -1;
}

// Decodes standard or URL-safe base64. Padding is optional, ASCII whitespace is ignored.
// Appends the bytes to out; false on an invalid character, data after '=', or an impossible length.
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

// Decodes into a String (for text payloads).
function base64DecodeStr(s: char*, out: Str.String*) => bool {
    let tmp: L.List<u8>;
    let ok: bool = base64Decode(s, &tmp);
    if (ok && tmp.len > 0) ok = out.appendN(tmp.data as char*, tmp.len);
    tmp.free();
    return ok;
}

// Lowercase hex of the bytes of a C string.
function hexEncodeStr(s: char*, out: Str.String*) => bool {
    return hexEncode(s as u8*, CStr.strlen(s), out);
}
