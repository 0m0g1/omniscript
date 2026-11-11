// standard/1/Compress.os
// Tier 5: zlib one-shot compress/decompress + crc32/adler32. Output buffers are malloc'd: CMem.free().
module Z = import "standard/1/c/zlib.os";
module CMem = import "standard/1/c/mem.os";

// level 0-9 (-1 default via 6). Returns malloc'd buffer + size.
function deflate(src: u8*, n: u64, level: int, outLen: u64*) => u8* {
    let cap: u64 = Z.compressBound(n);
    let buf = CMem.malloc(cap) as u8*;
    if (buf == nullptr) return nullptr;
    if (Z.compress2(buf, &cap, src, n, level) != 0) { CMem.free(buf as void*); return nullptr; }
    *outLen = cap;
    return buf;
}

// You must know (or store) the original size.
function inflate(src: u8*, n: u64, originalLen: u64, outLen: u64*) => u8* {
    let cap: u64 = originalLen;
    let buf = CMem.malloc(cap + 1) as u8*;
    if (buf == nullptr) return nullptr;
    if (Z.uncompress(buf, &cap, src, n) != 0) { CMem.free(buf as void*); return nullptr; }
    *outLen = cap;
    return buf;
}

function crc32(p: u8*, n: u64) => u32 { return Z.crc32(0, p, n as u32) as u32; }
function adler32(p: u8*, n: u64) => u32 { return Z.adler32(1, p, n as u32) as u32; }
// TODO: gzip/zip containers, streaming (deflateInit2_/inflate) -- needs z_stream layout.
// TODO(gzip/zip): gzip = deflate payload + 10-byte header + crc32/isize trailer (can be built from Binary.ByteWriter + deflate raw via deflateInit2_);
//                 zip read/write = local headers + central directory using Binary + crc32.
