// standard/1/Font.os
// Tier 5: TrueType loading + glyph rasterising via stb_truetype. The file buffer must outlive the Font.
module T = import "standard/1/c/font.os";
module CMem = import "standard/1/c/mem.os";

extern "C" fn fopen(path: char*, mode: char*) => void*;
extern "C" fn fclose(f: void*) => int;
extern "C" fn fread(buf: void*, sz: u64, n: u64, f: void*) => u64;
extern "C" fn fseek(f: void*, off: i64, whence: int) => int;
extern "C" fn ftell(f: void*) => i64;

struct Glyph { bitmap: u8*; w: int; h: int; xoff: int; yoff: int; advance: int; }

struct Font {
    info: [256]u8;        // stbtt_fontinfo (opaque, generously sized)
    file: u8* = nullptr;
    scale: float = 0.0;
    ascent: int = 0; descent: int = 0; lineGap: int = 0;

    load(path: char*, pixelHeight: float) => bool {
        let f = fopen(path, "rb");
        if (f == nullptr) return false;
        fseek(f, 0, 2);
        let n: i64 = ftell(f);
        fseek(f, 0, 0);
        this.file = CMem.malloc(n as u64) as u8*;
        fread(this.file as void*, 1, n as u64, f);
        fclose(f);
        if (T.stbtt_InitFont(&this.info as void*, this.file, 0) == 0) return false;
        this.setSize(pixelHeight);
        return true;
    }
    free() => void { if (this.file != nullptr) { CMem.free(this.file as void*); this.file = nullptr; } }

    setSize(px: float) => void {
        this.scale = T.stbtt_ScaleForPixelHeight(&this.info as void*, px);
        T.stbtt_GetFontVMetrics(&this.info as void*, &this.ascent, &this.descent, &this.lineGap);
    }
    lineHeight() => int { return (((this.ascent - this.descent + this.lineGap) as float) * this.scale) as int; }

    // Rasterise one code point to an 8-bit alpha bitmap. Call freeGlyph() afterwards.
    glyph(cp: int, out: Glyph*) => bool {
        let adv: int = 0; let lsb: int = 0;
        T.stbtt_GetCodepointHMetrics(&this.info as void*, cp, &adv, &lsb);
        out.advance = ((adv as float) * this.scale) as int;
        out.bitmap = T.stbtt_GetCodepointBitmap(&this.info as void*, this.scale, this.scale, cp, &out.w, &out.h, &out.xoff, &out.yoff);
        return true;   // bitmap may be nullptr for blank glyphs (space)
    }
    freeGlyph(g: Glyph*) => void { if (g.bitmap != nullptr) { T.stbtt_FreeBitmap(g.bitmap, nullptr); g.bitmap = nullptr; } }
}
