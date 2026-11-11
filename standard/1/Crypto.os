// standard/1/Crypto.os
// Tier 5: hashes/HMAC/random/base64 via OpenSSL libcrypto. Digests written to caller buffers (32/20/16 bytes).
module O = import "standard/1/c/ssl.os";
module Str = import "standard/1/String.os";
module Bin = import "standard/1/Binary.os";
module CMem = import "standard/1/c/mem.os";
module CStr = import "standard/1/c/string.os";

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
// TODO: aesEncrypt/aesDecrypt (EVP_CIPHER_CTX_new, EVP_aes_256_gcm, EVP_EncryptInit_ex/Update/Final_ex, GCM tag),
//       password hashing (PKCS5_PBKDF2_HMAC is in libcrypto; bcrypt/argon2 need libsodium or a vendored impl).
