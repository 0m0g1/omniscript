// standard/1/c/ssl.os  (libcrypto)
// Tier 0: OpenSSL libcrypto: hashes, HMAC, PBKDF2, random, base64, EVP ciphers (AES-GCM).
// The library path below was a "PATH/TO/..." placeholder. It now points at dependencies/openssl/bin/ like the other
// bundled DLLs (curl, zlib). CHANGE IT if your OpenSSL 3 x64 DLLs live somewhere else. libssl (TLS) is in c/tls.os.
extern "dependencies/openssl/bin/libcrypto-3-x64.dll" {
    fn SHA256(data: u8*, n: u64, out: u8*) => u8*;
    fn SHA1(data: u8*, n: u64, out: u8*) => u8*;
    fn MD5(data: u8*, n: u64, out: u8*) => u8*;
    fn HMAC(md: void*, key: void*, klen: int, d: u8*, n: u64, out: u8*, olen: u32*) => u8*;
    fn EVP_sha256() => void*;
    fn RAND_bytes(buf: u8*, n: int) => int;
    fn EVP_EncodeBlock(dst: u8*, src: u8*, n: int) => int;
    fn EVP_DecodeBlock(dst: u8*, src: u8*, n: int) => int;

    // key derivation (password hashing)
    fn PKCS5_PBKDF2_HMAC(pass: char*, passlen: int, salt: u8*, saltlen: int, iter: int, md: void*, keylen: int, out: u8*) => int;

    // EVP symmetric ciphers (used for AES-256-GCM)
    fn EVP_CIPHER_CTX_new() => void*;
    fn EVP_CIPHER_CTX_free(ctx: void*) => void;
    fn EVP_CIPHER_CTX_ctrl(ctx: void*, kind: int, arg: int, ptr: void*) => int;   // 0x9 SET_IVLEN, 0x10 GET_TAG, 0x11 SET_TAG
    fn EVP_aes_256_gcm() => void*;
    fn EVP_EncryptInit_ex(ctx: void*, cipher: void*, engine: void*, key: u8*, iv: u8*) => int;
    fn EVP_EncryptUpdate(ctx: void*, out: u8*, outl: int*, inp: u8*, inl: int) => int;
    fn EVP_EncryptFinal_ex(ctx: void*, out: u8*, outl: int*) => int;
    fn EVP_DecryptInit_ex(ctx: void*, cipher: void*, engine: void*, key: u8*, iv: u8*) => int;
    fn EVP_DecryptUpdate(ctx: void*, out: u8*, outl: int*, inp: u8*, inl: int) => int;
    fn EVP_DecryptFinal_ex(ctx: void*, out: u8*, outl: int*) => int;
}
