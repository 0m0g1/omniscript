// standard/1/c/zlib.os
extern "C" fn compress2(dst: u8*, dstLen: u64*, src: u8*, srcLen: u64, level: int) => int;
extern "C" fn uncompress(dst: u8*, dstLen: u64*, src: u8*, srcLen: u64) => int;
extern "C" fn compressBound(n: u64) => u64;
extern "C" fn crc32(crc: u64, buf: u8*, len: u32) => u64;
extern "C" fn adler32(a: u64, buf: u8*, len: u32) => u64;
