// standard/1/Binary.os
// Tier 3: ByteWriter (growable) and ByteReader (cursor over a buffer). Little-endian by default; *BE variants.
module CMem = import "standard/1/c/mem.os";
module L = import "standard/1/List.os";

struct ByteWriter {
    buf: L.List<u8>;
    free() => void { this.buf.free(); }
    length() => u64 { return this.buf.len; }
    bytes() => u8* { return this.buf.data; }
    u8(v: u8) => bool { return this.buf.push(v); }
    raw(p: u8*, n: u64) => bool {
        let i: u64 = 0;
        while (i < n) { if (!this.buf.push(p[i])) return false; i = i + 1; }
        return true;
    }
    u16(v: u16) => bool { this.u8((v & 0xFF) as u8); return this.u8((v >> 8) as u8); }
    u32(v: u32) => bool { this.u16((v & 0xFFFF) as u16); return this.u16((v >> 16) as u16); }
    u64(v: u64) => bool { this.u32((v & 0xFFFFFFFF) as u32); return this.u32((v >> 32) as u32); }
    u16BE(v: u16) => bool { this.u8((v >> 8) as u8); return this.u8((v & 0xFF) as u8); }
    u32BE(v: u32) => bool { this.u16BE((v >> 16) as u16); return this.u16BE((v & 0xFFFF) as u16); }
    f64(v: double) => bool { let t: u64 = 0; CMem.memcpy(&t as void*, &v as void*, 8); return this.u64(t); }
    f32(v: float) => bool { let t: u32 = 0; CMem.memcpy(&t as void*, &v as void*, 4); return this.u32(t); }
    // NUL-free length-prefixed string (u32 len + bytes)
    str(s: char*, n: u64) => bool { this.u32(n as u32); return this.raw(s as u8*, n); }
}

struct ByteReader {
    data: u8* = nullptr;
    len: u64 = 0;
    pos: u64 = 0;
    init(d: u8*, n: u64) => void { this.data = d; this.len = n; this.pos = 0; }
    remaining() => u64 { return this.len - this.pos; }
    eof() => bool { return this.pos >= this.len; }
    seek(p: u64) => bool { if (p > this.len) return false; this.pos = p; return true; }
    skip(n: u64) => bool { if (this.pos + n > this.len) return false; this.pos = this.pos + n; return true; }
    u8(out: u8*) => bool { if (this.pos >= this.len) return false; *out = this.data[this.pos]; this.pos = this.pos + 1; return true; }
    u16(out: u16*) => bool {
        let a: u8 = 0; let b: u8 = 0;
        if (!this.u8(&a) || !this.u8(&b)) return false;
        *out = (a as u16) | ((b as u16) << 8); return true;
    }
    u32(out: u32*) => bool {
        let a: u16 = 0; let b: u16 = 0;
        if (!this.u16(&a) || !this.u16(&b)) return false;
        *out = (a as u32) | ((b as u32) << 16); return true;
    }
    u64(out: u64*) => bool {
        let a: u32 = 0; let b: u32 = 0;
        if (!this.u32(&a) || !this.u32(&b)) return false;
        *out = (a as u64) | ((b as u64) << 32); return true;
    }
    u16BE(out: u16*) => bool {
        let a: u8 = 0; let b: u8 = 0;
        if (!this.u8(&a) || !this.u8(&b)) return false;
        *out = ((a as u16) << 8) | (b as u16); return true;
    }
    u32BE(out: u32*) => bool {
        let a: u16 = 0; let b: u16 = 0;
        if (!this.u16BE(&a) || !this.u16BE(&b)) return false;
        *out = ((a as u32) << 16) | (b as u32); return true;
    }
    f64(out: double*) => bool { let t: u64 = 0; if (!this.u64(&t)) return false; CMem.memcpy(out as void*, &t as void*, 8); return true; }
    f32(out: float*) => bool { let t: u32 = 0; if (!this.u32(&t)) return false; CMem.memcpy(out as void*, &t as void*, 4); return true; }
    raw(dst: u8*, n: u64) => bool {
        if (this.pos + n > this.len) return false;
        CMem.memcpy(dst as void*, &this.data[this.pos] as void*, n);
        this.pos = this.pos + n; return true;
    }
}

function toHex(p: u8*, n: u64, out: char*) => void {   // out needs 2n+1 bytes
    let digits: char* = "0123456789abcdef";
    let i: u64 = 0;
    while (i < n) { out[i * 2] = digits[(p[i] >> 4) as u64]; out[i * 2 + 1] = digits[(p[i] & 15) as u64]; i = i + 1; }
    out[n * 2] = 0 as char;
}
