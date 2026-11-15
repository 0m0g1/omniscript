// standard/1/Crypto.os
// Tier 5: hashes/HMAC/random/base64, PBKDF2 password hashing and AES-256-GCM via OpenSSL libcrypto.
// Digests written to caller buffers (32/20/16 bytes). No `return` inside loop bodies (compiler quirk noted in Canvas.os).
module O = import "standard/1/c/ssl.os";
module Str = import "standard/1/String.os";
module Bin = import "standard/1/Binary.os";
module CMem = import "standard/1/c/mem.os";
module CStr = import "standard/1/c/string.os";
module Text = import "standard/1/Text.os";
module L = import "standard/1/List.os";

function sha256(p: u8*, n: u64, out: u8*) => void { O.SHA256(p, n, out); }       // out: 32 bytes
function sha1(p: u8*, n: u64, out: u8*) => void { O.SHA1(p, n, out); }           // out: 20 bytes
function md5(p: u8*, n: u64, out: u8*) => void { O.MD5(p, n, out); }             // out: 16 bytes
function hmacSha256(key: u8*, klen: u64, p: u8*, n: u64, out: u8*) => void {     // out: 32 bytes
    let l: u32 = 32;
    O.HMAC(O.EVP_sha256(), key as void*, klen as int, p, n, out, &l);
}
function randomBytes(p: u8*, n: u64) => bool { return O.RAND_bytes(p, n as int) == 1; }

function sha256Hex(s: char*, out: Str.String*) => bool {
    let d: [32]u8;
    sha256(s as u8*, CStr.strlen(s), &d as u8*);
    let hex: [65]char;
    Bin.toHex(&d as u8*, 32, &hex as char*);
    return out.append(&hex as char*);
}

function base64Encode(p: u8*, n: u64, out: Str.String*) => bool {
    let buf = CMem.malloc(((n + 2) / 3) * 4 + 1) as u8*;
    if (buf == nullptr) return false;
    let l: int = O.EVP_EncodeBlock(buf, p, n as int);
    let ok: bool = out.appendN(buf as char*, l as u64);
    CMem.free(buf as void*);
    return ok;
}

// Decodes into malloc'd buffer (padding stripped). nullptr on error.
function base64Decode(s: char*, outLen: u64*) => u8* {
    let n: u64 = CStr.strlen(s);
    let buf = CMem.malloc((n / 4) * 3 + 4) as u8*;
    if (buf == nullptr) return nullptr;
    let l: int = O.EVP_DecodeBlock(buf, s as u8*, n as int);
    if (l < 0) { CMem.free(buf as void*); return nullptr; }
    let pad: int = 0;
    if (n >= 1 && (s[n - 1] as int) == 61) pad = pad + 1;
    if (n >= 2 && (s[n - 2] as int) == 61) pad = pad + 1;
    *outLen = (l - pad) as u64;
    return buf;
}

function constantTimeEquals(a: u8*, b: u8*, n: u64) => bool {
    let diff: u8 = 0; let i: u64 = 0;
    while (i < n) { diff = diff | (a[i] ^ b[i]); i = i + 1; }
    return diff == 0;
}

// ---------------------------------------------------------------------------------------------------------------
// Key derivation / password hashing (PBKDF2-HMAC-SHA256; bcrypt/argon2 are not in OpenSSL)
// ---------------------------------------------------------------------------------------------------------------
let PW_ITERATIONS: int = 600000;     // OWASP guidance for PBKDF2-HMAC-SHA256

function pbkdf2Sha256(password: char*, salt: u8*, saltLen: u64, iterations: int, out: u8*, outLen: u64) => bool {
    return O.PKCS5_PBKDF2_HMAC(password, CStr.strlen(password) as int, salt, saltLen as int, iterations, O.EVP_sha256(), outLen as int, out) == 1;
}

// Appends "pbkdf2-sha256$<iterations>$<salt b64>$<hash b64>" (random 16-byte salt) to out. Store that string as-is.
function hashPassword(password: char*, out: Str.String*) => bool {
    let salt: [16]u8;
    let dk: [32]u8;
    if (!randomBytes(&salt as u8*, 16)) return false;
    if (!pbkdf2Sha256(password, &salt as u8*, 16, PW_ITERATIONS, &dk as u8*, 32)) return false;
    let ok: bool = out.append("pbkdf2-sha256$");
    if (ok) ok = out.appendInt(PW_ITERATIONS as i64);
    if (ok) ok = out.appendChar(36 as char);
    if (ok) ok = base64Encode(&salt as u8*, 16, out);
    if (ok) ok = out.appendChar(36 as char);
    if (ok) ok = base64Encode(&dk as u8*, 32, out);
    return ok;
}

// Checks a password against a string made by hashPassword (constant-time comparison).
function verifyPassword(password: char*, stored: char*) => bool {
    let parts: L.List<Str.String>;
    if (!Text.split(stored, "$", &parts)) return false;
    let ok: bool = parts.len == 4;
    if (ok) ok = parts.data[0].equalsC("pbkdf2-sha256");
    let iters: i64 = 0;
    if (ok) ok = parts.data[1].parseInt(&iters);
    if (ok) ok = iters > 0 && iters <= 100000000;
    let saltLen: u64 = 0;
    let hashLen: u64 = 0;
    let salt: u8* = nullptr;
    let want: u8* = nullptr;
    if (ok) {
        salt = base64Decode(parts.data[2].cstr(), &saltLen);
        want = base64Decode(parts.data[3].cstr(), &hashLen);
        ok = salt != nullptr && want != nullptr && saltLen > 0 && hashLen == 32;
    }
    let result: bool = false;
    if (ok) {
        let dk: [32]u8;
        if (pbkdf2Sha256(password, salt, saltLen, iters as int, &dk as u8*, 32)) result = constantTimeEquals(&dk as u8*, want, 32);
    }
    if (salt != nullptr) CMem.free(salt as void*);
    if (want != nullptr) CMem.free(want as void*);
    Text.freeParts(&parts);
    return result;
}

// ---------------------------------------------------------------------------------------------------------------
// AES-256-GCM (authenticated encryption). key = 32 bytes. Sealed layout: nonce(12) | ciphertext(n) | tag(16).
// A fresh random nonce is used for every call, so never reuse the same key for more than ~2^32 messages.
// ---------------------------------------------------------------------------------------------------------------
let AES_NONCE_LEN: u64 = 12;
let AES_TAG_LEN: u64 = 16;

// Returns a malloc'd buffer of n + 28 bytes (CMem.free it), or nullptr on error. *outLen = n + 28.
function aesEncrypt(key: u8*, plain: u8*, n: u64, outLen: u64*) => u8* {
    if (n > 2000000000) return nullptr;
    let buf = CMem.malloc(n + 28 + 1) as u8*;
    if (buf == nullptr) return nullptr;
    let ok: bool = randomBytes(buf, 12);
    let ctx: void* = nullptr;
    if (ok) { ctx = O.EVP_CIPHER_CTX_new(); ok = ctx != nullptr; }
    let l1: int = 0;
    let l2: int = 0;
    if (ok) ok = O.EVP_EncryptInit_ex(ctx, O.EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1;
    if (ok) ok = O.EVP_CIPHER_CTX_ctrl(ctx, 9, 12, nullptr) == 1;
    if (ok) ok = O.EVP_EncryptInit_ex(ctx, nullptr, nullptr, key, buf) == 1;
    if (ok && n > 0) ok = O.EVP_EncryptUpdate(ctx, &buf[12], &l1, plain, n as int) == 1;
    if (ok) ok = O.EVP_EncryptFinal_ex(ctx, &buf[12 + (l1 as u64)], &l2) == 1;
    if (ok) ok = O.EVP_CIPHER_CTX_ctrl(ctx, 16, 16, &buf[12 + n] as void*) == 1;
    if (ctx != nullptr) O.EVP_CIPHER_CTX_free(ctx);
    if (!ok) { CMem.free(buf as void*); return nullptr; }
    *outLen = n + 28;
    return buf;
}

// Input: a buffer made by aesEncrypt (>= 28 bytes). Returns a malloc'd plaintext (n - 28 bytes, plus a NUL byte after
// it for convenience), or nullptr if the key is wrong or the data was modified. *outLen = n - 28.
function aesDecrypt(key: u8*, data: u8*, n: u64, outLen: u64*) => u8* {
    if (n < 28 || n > 2000000000) return nullptr;
    let c: u64 = n - 28;
    let out = CMem.malloc(c + 1) as u8*;
    if (out == nullptr) return nullptr;
    let ctx: void* = O.EVP_CIPHER_CTX_new();
    let ok: bool = ctx != nullptr;
    let l1: int = 0;
    let l2: int = 0;
    let tmp: [16]u8;
    if (ok) ok = O.EVP_DecryptInit_ex(ctx, O.EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1;
    if (ok) ok = O.EVP_CIPHER_CTX_ctrl(ctx, 9, 12, nullptr) == 1;
    if (ok) ok = O.EVP_DecryptInit_ex(ctx, nullptr, nullptr, key, data) == 1;
    if (ok && c > 0) ok = O.EVP_DecryptUpdate(ctx, out, &l1, &data[12], c as int) == 1;
    if (ok) ok = O.EVP_CIPHER_CTX_ctrl(ctx, 17, 16, &data[12 + c] as void*) == 1;
    if (ok) ok = O.EVP_DecryptFinal_ex(ctx, &tmp as u8*, &l2) > 0;
    if (ctx != nullptr) O.EVP_CIPHER_CTX_free(ctx);
    if (!ok) { CMem.free(out as void*); return nullptr; }
    out[c] = 0;
    *outLen = c;
    return out;
}
