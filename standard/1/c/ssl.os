// standard/1/c/ssl.os  (libcrypto)
extern "PATH/TO/libcrypto-3-x64.dll" {
    fn SHA256(data: u8*, n: u64, out: u8*) => u8*;
    fn SHA1(data: u8*, n: u64, out: u8*) => u8*;
    fn MD5(data: u8*, n: u64, out: u8*) => u8*;
    fn HMAC(md: void*, key: void*, klen: int, d: u8*, n: u64, out: u8*, olen: u32*) => u8*;
    fn EVP_sha256() => void*;
    fn RAND_bytes(buf: u8*, n: int) => int;
    fn EVP_EncodeBlock(dst: u8*, src: u8*, n: int) => int;
    fn EVP_DecodeBlock(dst: u8*, src: u8*, n: int) => int;
}
