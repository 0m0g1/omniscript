// standard/1/Image.os
// Tier 5: load/save images via stb_image(+write). Pixels are 8-bit interleaved; free() when done.
module S = import "standard/1/c/image.os";
module CMem = import "standard/1/c/mem.os";

struct Image {
    px: u8* = nullptr;
    w: int = 0; h: int = 0; ch: int = 0;
    free() => void { if (this.px != nullptr) { S.stbi_image_free(this.px as void*); this.px = nullptr; } }
    // want: 0 = file's channels, or force 1..4 (4 = RGBA)
    load(path: char*, want: int) => bool {
        this.px = S.stbi_load(path, &this.w, &this.h, &this.ch, want);
        if (this.px == nullptr) return false;
        if (want != 0) this.ch = want;
        return true;
    }
    loadMemory(p: u8*, n: u64, want: int) => bool {
        this.px = S.stbi_load_from_memory(p, n as int, &this.w, &this.h, &this.ch, want);
        if (this.px == nullptr) return false;
        if (want != 0) this.ch = want;
        return true;
    }
    savePng(path: char*) => bool { return S.stbi_write_png(path, this.w, this.h, this.ch, this.px as void*, this.w * this.ch) != 0; }
    saveJpg(path: char*, q: int) => bool { return S.stbi_write_jpg(path, this.w, this.h, this.ch, this.px as void*, q) != 0; }
    saveBmp(path: char*) => bool { return S.stbi_write_bmp(path, this.w, this.h, this.ch, this.px as void*) != 0; }
    pixel(x: int, y: int) => u8* { return &this.px[((y * this.w) + x) * this.ch]; }
}
