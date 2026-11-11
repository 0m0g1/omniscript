// standard/1/c/ssl.os  (libcrypto)
extern "C" fn SHA256(data: u8*, n: u64, out: u8*) => u8*;
extern "C" fn SHA1(data: u8*, n: u64, out: u8*) => u8*;
extern "C" fn MD5(data: u8*, n: u64, out: u8*) => u8*;
extern "C" fn HMAC(md: void*, key: void*, klen: int, d: u8*, n: u64, out: u8*, olen: u32*) => u8*;
extern "C" fn EVP_sha256() => void*;
extern "C" fn RAND_bytes(buf: u8*, n: int) => int;
extern "C" fn EVP_EncodeBlock(dst: u8*, src: u8*, n: int) => int;
extern "C" fn EVP_DecodeBlock(dst: u8*, src: u8*, n: int) => int;
