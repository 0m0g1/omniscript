// standard/1/Compress.os
// Tier 5: zlib one-shot compress/decompress, crc32/adler32, gzip/gunzip and a raw-deflate decoder written in
// OmniScript (so decompression needs no z_stream layout). Output buffers are malloc'd: CMem.free().
// No `return` inside loop bodies (compiler quirk noted in Canvas.os).
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

// ---------------------------------------------------------------------------------------------------------------
// gzip (RFC 1952). Compression reuses zlib's one-shot compress2: a zlib stream is 2 header bytes + raw deflate data +
// 4 adler32 bytes, so the raw deflate part can be wrapped in a gzip header/trailer without a z_stream.
// ---------------------------------------------------------------------------------------------------------------
function putLE32(p: u8*, v: u32) => void {
    p[0] = (v & 0xFF) as u8;
    p[1] = ((v >> 8) & 0xFF) as u8;
    p[2] = ((v >> 16) & 0xFF) as u8;
    p[3] = ((v >> 24) & 0xFF) as u8;
}

function getLE32(p: u8*) => u32 {
    return (p[0] as u32) | ((p[1] as u32) << 8) | ((p[2] as u32) << 16) | ((p[3] as u32) << 24);
}

// level 0-9. Returns a malloc'd .gz file image (CMem.free it), nullptr on error.
function gzip(src: u8*, n: u64, level: int, outLen: u64*) => u8* {
    let zl: u64 = 0;
    let z = deflate(src, n, level, &zl);
    if (z == nullptr) return nullptr;
    if (zl < 6) { CMem.free(z as void*); return nullptr; }
    let dl: u64 = zl - 6;
    let out = CMem.malloc(dl + 18) as u8*;
    if (out == nullptr) { CMem.free(z as void*); return nullptr; }
    out[0] = 0x1F; out[1] = 0x8B; out[2] = 8; out[3] = 0;       // magic, deflate, no flags
    out[4] = 0; out[5] = 0; out[6] = 0; out[7] = 0;             // mtime unknown
    out[8] = 0; out[9] = 11;                                    // xfl, OS = NTFS
    CMem.memcpy(&out[10] as void*, &z[2] as void*, dl);
    putLE32(&out[10 + dl], crc32(src, n));
    putLE32(&out[14 + dl], (n & 0xFFFFFFFF) as u32);
    CMem.free(z as void*);
    *outLen = dl + 18;
    return out;
}

// ---------------------------------------------------------------------------------------------------------------
// Raw DEFLATE decoder (RFC 1951): stored, fixed-Huffman and dynamic-Huffman blocks.
// ---------------------------------------------------------------------------------------------------------------
struct Inf {
    src: u8* = nullptr;
    n: u64 = 0;
    pos: u64 = 0;
    bitbuf: u32 = 0;
    bitcnt: int = 0;
    out: u8* = nullptr;
    olen: u64 = 0;
    ocap: u64 = 0;
    maxOut: u64 = 0;
    err: bool = false;
    lc: [16]u16;      // literal/length tree: counts per code length, symbols sorted by code
    ls: [288]u16;
    dc: [16]u16;      // distance tree (also used for the code-length tree while a dynamic block header is read)
    ds: [32]u16;
    lbits: [30]u8;
    lbase: [30]u16;
    dbits: [30]u8;
    dbase: [30]u16;
    order: [19]u8;
}

function infBit(s: Inf*) => int {
    if (s.bitcnt == 0) {
        if (s.pos >= s.n) { s.err = true; return 0; }
        s.bitbuf = s.src[s.pos] as u32;
        s.pos = s.pos + 1;
        s.bitcnt = 8;
    }
    let b: int = (s.bitbuf & 1) as int;
    s.bitbuf = s.bitbuf >> 1;
    s.bitcnt = s.bitcnt - 1;
    return b;
}

// n bits, least significant first.
function infBits(s: Inf*, n: int) => u32 {
    let v: u32 = 0;
    let i: int = 0;
    while (i < n) {
        v = v | ((infBit(s) as u32) << (i as u32));
        i = i + 1;
    }
    return v;
}

function infPut(s: Inf*, b: u8) => void {
    if (s.olen >= s.maxOut) { s.err = true; return; }
    if (s.olen >= s.ocap) {
        let nc: u64 = s.ocap * 2;
        if (nc < 4096) nc = 4096;
        let p = CMem.realloc(s.out as void*, nc);
        if (p == nullptr) { s.err = true; return; }
        s.out = p as u8*;
        s.ocap = nc;
    }
    s.out[s.olen] = b;
    s.olen = s.olen + 1;
}

// Canonical Huffman tree from code lengths lengths[off .. off+n).
function infBuild(counts: u16*, symbols: u16*, lengths: u8*, off: int, n: int) => void {
    let offs: [16]u16;
    let i: int = 0;
    while (i < 16) { counts[i] = 0; offs[i] = 0; i = i + 1; }
    i = 0;
    while (i < n) { let l: int = lengths[off + i] as int; counts[l] = (counts[l] + 1) as u16; i = i + 1; }
    offs[1] = 0;
    i = 1;
    while (i < 15) { offs[i + 1] = (offs[i] + counts[i]) as u16; i = i + 1; }
    i = 0;
    while (i < n) {
        let l: int = lengths[off + i] as int;
        if (l != 0) {
            symbols[offs[l] as int] = i as u16;
            offs[l] = (offs[l] + 1) as u16;
        }
        i = i + 1;
    }
}

function infDecode(s: Inf*, counts: u16*, symbols: u16*) => int {
    let sum: int = 0;
    let cur: int = 0;
    let len: int = 0;
    let go: bool = true;
    while (go) {
        cur = 2 * cur + infBit(s);
        len = len + 1;
        if (len > 15) {
            s.err = true;
            go = false;
        } else {
            sum = sum + (counts[len] as int);
            cur = cur - (counts[len] as int);
            if (cur < 0) go = false;
        }
        if (s.err) go = false;
    }
    if (s.err) return 0;
    return symbols[sum + cur] as int;
}

function infBuildBits(bits: u8*, base: u16*, delta: int, first: int) => void {
    let i: int = 0;
    while (i < delta) { bits[i] = 0; i = i + 1; }
    i = 0;
    while (i < 30 - delta) { bits[i + delta] = (i / delta) as u8; i = i + 1; }
    let sum: int = first;
    i = 0;
    while (i < 30) {
        base[i] = sum as u16;
        sum = sum + (1 << (bits[i] as int));
        i = i + 1;
    }
}

function infInit(s: Inf*, src: u8*, n: u64, maxOut: u64) => void {
    s.src = src; s.n = n; s.pos = 0; s.bitbuf = 0; s.bitcnt = 0;
    s.out = nullptr; s.olen = 0; s.ocap = 0; s.err = false;
    s.maxOut = maxOut;
    if (maxOut == 0) s.maxOut = 1073741824;                 // default cap: 1 GiB
    infBuildBits(&s.lbits as u8*, &s.lbase as u16*, 4, 3);
    s.lbits[28] = 0;
    s.lbase[28] = 258;
    infBuildBits(&s.dbits as u8*, &s.dbase as u16*, 2, 1);
    s.order[0] = 16; s.order[1] = 17; s.order[2] = 18; s.order[3] = 0; s.order[4] = 8;
    s.order[5] = 7; s.order[6] = 9; s.order[7] = 6; s.order[8] = 10; s.order[9] = 5;
    s.order[10] = 11; s.order[11] = 4; s.order[12] = 12; s.order[13] = 3; s.order[14] = 13;
    s.order[15] = 2; s.order[16] = 14; s.order[17] = 1; s.order[18] = 15;
}

// Decodes literal/length + distance symbols until the end-of-block code, using s.lc/ls and s.dc/ds.
function infBlock(s: Inf*) => void {
    let go: bool = true;
    while (go && !s.err) {
        let sym: int = infDecode(s, &s.lc as u16*, &s.ls as u16*);
        if (s.err) {
            go = false;
        } else if (sym < 256) {
            infPut(s, sym as u8);
        } else if (sym == 256) {
            go = false;
        } else {
            sym = sym - 257;
            if (sym >= 29) {
                s.err = true;
            } else {
                let length: int = (s.lbase[sym] as int) + (infBits(s, s.lbits[sym] as int) as int);
                let dsym: int = infDecode(s, &s.dc as u16*, &s.ds as u16*);
                if (dsym >= 30) {
                    s.err = true;
                } else {
                    let dist: u64 = (s.dbase[dsym] as u64) + (infBits(s, s.dbits[dsym] as int) as u64);
                    if (dist > s.olen) {
                        s.err = true;
                    } else {
                        let i: int = 0;
                        while (i < length && !s.err) {
                            infPut(s, s.out[s.olen - dist]);
                            i = i + 1;
                        }
                    }
                }
            }
        }
    }
}

function infStored(s: Inf*) => void {
    s.bitcnt = 0;                                           // drop the rest of the current byte
    if (s.pos + 4 > s.n) { s.err = true; return; }
    let len: u64 = (s.src[s.pos] as u64) | ((s.src[s.pos + 1] as u64) << 8);
    let nlen: u64 = (s.src[s.pos + 2] as u64) | ((s.src[s.pos + 3] as u64) << 8);
    s.pos = s.pos + 4;
    if (len != (nlen ^ 0xFFFF)) { s.err = true; return; }
    if (s.pos + len > s.n) { s.err = true; return; }
    let i: u64 = 0;
    while (i < len && !s.err) {
        infPut(s, s.src[s.pos + i]);
        i = i + 1;
    }
    s.pos = s.pos + len;
}

function infFixed(s: Inf*) => void {
    let lens: [288]u8;
    let i: int = 0;
    while (i < 144) { lens[i] = 8; i = i + 1; }
    while (i < 256) { lens[i] = 9; i = i + 1; }
    while (i < 280) { lens[i] = 7; i = i + 1; }
    while (i < 288) { lens[i] = 8; i = i + 1; }
    infBuild(&s.lc as u16*, &s.ls as u16*, &lens as u8*, 0, 288);
    i = 0;
    while (i < 30) { lens[i] = 5; i = i + 1; }
    infBuild(&s.dc as u16*, &s.ds as u16*, &lens as u8*, 0, 30);
    infBlock(s);
}

function infDynamic(s: Inf*) => void {
    let lens: [320]u8;
    let cl: [19]u8;
    let i: int = 0;
    while (i < 320) { lens[i] = 0; i = i + 1; }
    i = 0;
    while (i < 19) { cl[i] = 0; i = i + 1; }
    let hlit: int = (infBits(s, 5) as int) + 257;
    let hdist: int = (infBits(s, 5) as int) + 1;
    let hclen: int = (infBits(s, 4) as int) + 4;
    i = 0;
    while (i < hclen) { cl[s.order[i] as int] = infBits(s, 3) as u8; i = i + 1; }
    infBuild(&s.dc as u16*, &s.ds as u16*, &cl as u8*, 0, 19);       // code-length tree (temporary, in the distance tables)
    let total: int = hlit + hdist;
    let num: int = 0;
    while (num < total && !s.err) {
        let sym: int = infDecode(s, &s.dc as u16*, &s.ds as u16*);
        if (!s.err) {
            if (sym < 16) {
                lens[num] = sym as u8;
                num = num + 1;
            } else {
                let prev: u8 = 0;
                let rep: int = 0;
                if (sym == 16) {
                    if (num == 0) s.err = true;
                    else { prev = lens[num - 1]; rep = 3 + (infBits(s, 2) as int); }
                } else if (sym == 17) {
                    rep = 3 + (infBits(s, 3) as int);
                } else {
                    rep = 11 + (infBits(s, 7) as int);
                }
                if (!s.err) {
                    if (num + rep > total) {
                        s.err = true;
                    } else {
                        let k: int = 0;
                        while (k < rep) { lens[num] = prev; num = num + 1; k = k + 1; }
                    }
                }
            }
        }
    }
    if (!s.err) {
        infBuild(&s.lc as u16*, &s.ls as u16*, &lens as u8*, 0, hlit);
        infBuild(&s.dc as u16*, &s.ds as u16*, &lens as u8*, hlit, hdist);
        infBlock(s);
    }
}

// Decodes a raw deflate stream (no zlib/gzip wrapper). maxOut caps the output size (0 = 1 GiB) as a zip-bomb guard.
// Returns a malloc'd buffer (with one extra NUL byte after the data) or nullptr if the data is corrupt.
function inflateRaw(src: u8*, n: u64, maxOut: u64, outLen: u64*) => u8* {
    let s: Inf;
    infInit(&s, src, n, maxOut);
    let last: bool = false;
    while (!last && !s.err) {
        last = infBit(&s) == 1;
        let kind: u32 = infBits(&s, 2);
        if (s.err) {
            last = true;
        } else if (kind == 0) {
            infStored(&s);
        } else if (kind == 1) {
            infFixed(&s);
        } else if (kind == 2) {
            infDynamic(&s);
        } else {
            s.err = true;
        }
    }
    if (s.err) {
        if (s.out != nullptr) CMem.free(s.out as void*);
        return nullptr;
    }
    // make room for the NUL byte (and make sure empty output still yields a valid pointer)
    let p = CMem.realloc(s.out as void*, s.olen + 1);
    if (p == nullptr) {
        if (s.out != nullptr) CMem.free(s.out as void*);
        return nullptr;
    }
    let r: u8* = p as u8*;
    r[s.olen] = 0;
    *outLen = s.olen;
    return r;
}

// Decodes a .gz file image (first member only). Checks the CRC32 and size trailer. maxOut as for inflateRaw.
// Returns a malloc'd buffer (with a trailing NUL) or nullptr if the data is not valid gzip.
function gunzip(src: u8*, n: u64, maxOut: u64, outLen: u64*) => u8* {
    if (n < 18) return nullptr;
    if (src[0] != 0x1F || src[1] != 0x8B || src[2] != 8) return nullptr;
    let flg: int = src[3] as int;
    let pos: u64 = 10;
    if ((flg & 4) != 0) {                                    // FEXTRA
        if (pos + 2 > n) return nullptr;
        let xl: u64 = (src[pos] as u64) | ((src[pos + 1] as u64) << 8);
        pos = pos + 2 + xl;
    }
    if ((flg & 8) != 0) {                                    // FNAME (NUL-terminated)
        while (pos < n && src[pos] != 0) pos = pos + 1;
        pos = pos + 1;
    }
    if ((flg & 16) != 0) {                                   // FCOMMENT (NUL-terminated)
        while (pos < n && src[pos] != 0) pos = pos + 1;
        pos = pos + 1;
    }
    if ((flg & 2) != 0) pos = pos + 2;                       // FHCRC
    if (pos + 8 > n) return nullptr;
    let dl: u64 = n - pos - 8;
    let ol: u64 = 0;
    let out = inflateRaw(&src[pos], dl, maxOut, &ol);
    if (out == nullptr) return nullptr;
    let wantCrc: u32 = getLE32(&src[n - 8]);
    let wantSize: u32 = getLE32(&src[n - 4]);
    if (crc32(out, ol) != wantCrc || ((ol & 0xFFFFFFFF) as u32) != wantSize) {
        CMem.free(out as void*);
        return nullptr;
    }
    *outLen = ol;
    return out;
}
