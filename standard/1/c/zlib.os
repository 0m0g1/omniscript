// standard/1/c/zlib.os
extern "dependencies/llvm/bin/zlib1.dll" {
    fn compress2(dst: u8*, dstLen: u64*, src: u8*, srcLen: u64, level: int) => int;
    fn uncompress(dst: u8*, dstLen: u64*, src: u8*, srcLen: u64) => int;
    fn compressBound(n: u64) => u64;
    fn crc32(crc: u64, buf: u8*, len: u32) => u64;
    fn adler32(a: u64, buf: u8*, len: u32) => u64;
}
