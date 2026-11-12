// standard/1/Image.os
// RGBA8, tightly packed, row-major, origin top-left.
// Image owns its pixels. Call free() when done.
// Works directly with Pen2D.createImage(img.pixels, img.width, img.height).

module C = import "standard/1/c/image.os";

// Returns 1=png 2=jpg 3=bmp 4=tga, 0=unknown (case-insensitive).
fn image_ext_kind(path: char*) => int {
    let n: int = 0;
    while (path[n] != 0) { n = n + 1; }
    if (n < 4) { return 0; }

    let a: int = (path[n - 3] as int) | 32;
    let b: int = (path[n - 2] as int) | 32;
    let c: int = (path[n - 1] as int) | 32;
    let dot: int = path[n - 4] as int;
    if (dot != 46) { return 0; }

    if (a == 112 && b == 110 && c == 103) { return 1; }   // png
    if (a == 106 && b == 112 && c == 103) { return 2; }   // jpg
    if (a == 98  && b == 109 && c == 112) { return 3; }   // bmp
    if (a == 116 && b == 103 && c == 97)  { return 4; }   // tga
    return 0;
}

struct Image {
    pixels: u8* = nullptr;
    width: int = 0;
    height: int = 0;

    // ---------------- construction ----------------

    // Blank (transparent black) image.
    create(w: int, h: int) => bool {
        this.free();
        if (w <= 0 || h <= 0) { return false; }
        let size: u64 = (w as u64) * (h as u64) * 4;
        let p = C.om_alloc(size) as u8*;
        if (p == nullptr) { return false; }
        let i: u64 = 0;
        while (i < size) { p[i] = 0; i = i + 1; }
        this.pixels = p;
        this.width = w;
        this.height = h;
        return true;
    }

    // Decode a file (png/jpg/bmp/tga/gif/hdr...). Always yields RGBA8.
    load(path: char*) => bool {
        this.free();
        let w: int = 0;
        let h: int = 0;
        let ch: int = 0;
        let p = C.stbi_load(path, &w, &h, &ch, 4);
        if (p == nullptr) { return false; }
        this.pixels = p;
        this.width = w;
        this.height = h;
        return true;
    }

    // Decode from memory.
    load_memory(buf: u8*, len: int) => bool {
        this.free();
        let w: int = 0;
        let h: int = 0;
        let ch: int = 0;
        let p = C.stbi_load_from_memory(buf, len, &w, &h, &ch, 4);
        if (p == nullptr) { return false; }
        this.pixels = p;
        this.width = w;
        this.height = h;
        return true;
    }

    // Deep copy into `out`.
    clone(out: Image*) => bool {
        if (this.pixels == nullptr) { return false; }
        if (!out.create(this.width, this.height)) { return false; }
        let size: u64 = (this.width as u64) * (this.height as u64) * 4;
        let i: u64 = 0;
        while (i < size) { out.pixels[i] = this.pixels[i]; i = i + 1; }
        return true;
    }

    // ---------------- saving ----------------

    // Format chosen by extension: .png .jpg .bmp .tga
    save(path: char*) => bool {
        if (this.pixels == nullptr) { return false; }
        let kind: int = image_ext_kind(path);
        if (kind == 1) { return C.stbi_write_png(path, this.width, this.height, 4, this.pixels as void*, this.width * 4) != 0; }
        if (kind == 2) { return C.stbi_write_jpg(path, this.width, this.height, 4, this.pixels as void*, 90) != 0; }
        if (kind == 3) { return C.stbi_write_bmp(path, this.width, this.height, 4, this.pixels as void*) != 0; }
        if (kind == 4) { return C.stbi_write_tga(path, this.width, this.height, 4, this.pixels as void*) != 0; }
        return false;
    }

    save_jpg(path: char*, quality: int) => bool {
        if (this.pixels == nullptr) { return false; }
        return C.stbi_write_jpg(path, this.width, this.height, 4, this.pixels as void*, quality) != 0;
    }

    // ---------------- queries ----------------

    valid() => bool { return this.pixels != nullptr; }

    in_bounds(x: int, y: int) => bool {
        return x >= 0 && y >= 0 && x < this.width && y < this.height;
    }

    // Packed little-endian RGBA: 0xAABBGGRR. Out of bounds -> 0.
    get(x: int, y: int) => u32 {
        if (!this.in_bounds(x, y)) { return 0; }
        let i: int = (y * this.width + x) * 4;
        let r: u32 = this.pixels[i] as u32;
        let g: u32 = this.pixels[i + 1] as u32;
        let b: u32 = this.pixels[i + 2] as u32;
        let a: u32 = this.pixels[i + 3] as u32;
        return r | (g << 8) | (b << 16) | (a << 24);
    }

    set(x: int, y: int, rgba: u32) => void {
        if (!this.in_bounds(x, y)) { return; }
        let i: int = (y * this.width + x) * 4;
        this.pixels[i]     = (rgba & 255) as u8;
        this.pixels[i + 1] = ((rgba >> 8) & 255) as u8;
        this.pixels[i + 2] = ((rgba >> 16) & 255) as u8;
        this.pixels[i + 3] = ((rgba >> 24) & 255) as u8;
    }

    // ---------------- editing ----------------

    fill(rgba: u32) => void {
        if (this.pixels == nullptr) { return; }
        let r: u8 = (rgba & 255) as u8;
        let g: u8 = ((rgba >> 8) & 255) as u8;
        let b: u8 = ((rgba >> 16) & 255) as u8;
        let a: u8 = ((rgba >> 24) & 255) as u8;
        let n: int = this.width * this.height;
        let i: int = 0;
        while (i < n) {
            let o: int = i * 4;
            this.pixels[o] = r; this.pixels[o + 1] = g;
            this.pixels[o + 2] = b; this.pixels[o + 3] = a;
            i = i + 1;
        }
    }

    // Crop into `out`. Rect is clamped to the image.
    crop(x: int, y: int, w: int, h: int, out: Image*) => bool {
        if (this.pixels == nullptr) { return false; }
        if (x < 0) { w = w + x; x = 0; }
        if (y < 0) { h = h + y; y = 0; }
        if (x + w > this.width)  { w = this.width - x; }
        if (y + h > this.height) { h = this.height - y; }
        if (w <= 0 || h <= 0) { return false; }
        if (!out.create(w, h)) { return false; }
        let row: int = 0;
        while (row < h) {
            let s: int = ((y + row) * this.width + x) * 4;
            let d: int = row * w * 4;
            let k: int = 0;
            while (k < w * 4) { out.pixels[d + k] = this.pixels[s + k]; k = k + 1; }
            row = row + 1;
        }
        return true;
    }

    // Bilinear resize into `out` (pure OmniScript; fine for up/down-scaling
    // by moderate factors. Very large downscales will alias).
    resize(w: int, h: int, out: Image*) => bool {
        if (this.pixels == nullptr) { return false; }
        if (w <= 0 || h <= 0) { return false; }
        if (!out.create(w, h)) { return false; }

        let sw: int = this.width;
        let sh: int = this.height;
        let y: int = 0;
        while (y < h) {
            let fy: float = ((y as float) + 0.5) * (sh as float) / (h as float) - 0.5;
            if (fy < 0.0) { fy = 0.0; }
            let y0: int = fy as int;
            if (y0 > sh - 1) { y0 = sh - 1; }
            let y1: int = y0 + 1;
            if (y1 > sh - 1) { y1 = sh - 1; }
            let ty: float = fy - (y0 as float);

            let x: int = 0;
            while (x < w) {
                let fx: float = ((x as float) + 0.5) * (sw as float) / (w as float) - 0.5;
                if (fx < 0.0) { fx = 0.0; }
                let x0: int = fx as int;
                if (x0 > sw - 1) { x0 = sw - 1; }
                let x1: int = x0 + 1;
                if (x1 > sw - 1) { x1 = sw - 1; }
                let tx: float = fx - (x0 as float);

                let i00: int = (y0 * sw + x0) * 4;
                let i10: int = (y0 * sw + x1) * 4;
                let i01: int = (y1 * sw + x0) * 4;
                let i11: int = (y1 * sw + x1) * 4;
                let o: int = (y * w + x) * 4;

                let k: int = 0;
                while (k < 4) {
                    let p00: float = this.pixels[i00 + k] as float;
                    let p10: float = this.pixels[i10 + k] as float;
                    let p01: float = this.pixels[i01 + k] as float;
                    let p11: float = this.pixels[i11 + k] as float;
                    let top: float = p00 + (p10 - p00) * tx;
                    let bot: float = p01 + (p11 - p01) * tx;
                    let v: float = top + (bot - top) * ty;
                    out.pixels[o + k] = ((v + 0.5) as int) as u8;
                    k = k + 1;
                }
                x = x + 1;
            }
            y = y + 1;
        }
        return true;
    }

    flip_h() => void {
        if (this.pixels == nullptr) { return; }
        let y: int = 0;
        while (y < this.height) {
            let x: int = 0;
            while (x < this.width / 2) {
                let a: int = (y * this.width + x) * 4;
                let b: int = (y * this.width + (this.width - 1 - x)) * 4;
                let k: int = 0;
                while (k < 4) {
                    let t: u8 = this.pixels[a + k];
                    this.pixels[a + k] = this.pixels[b + k];
                    this.pixels[b + k] = t;
                    k = k + 1;
                }
                x = x + 1;
            }
            y = y + 1;
        }
    }

    flip_v() => void {
        if (this.pixels == nullptr) { return; }
        let stride: int = this.width * 4;
        let y: int = 0;
        while (y < this.height / 2) {
            let a: int = y * stride;
            let b: int = (this.height - 1 - y) * stride;
            let k: int = 0;
            while (k < stride) {
                let t: u8 = this.pixels[a + k];
                this.pixels[a + k] = this.pixels[b + k];
                this.pixels[b + k] = t;
                k = k + 1;
            }
            y = y + 1;
        }
    }

    // Rec. 601 luma, alpha preserved.
    grayscale() => void {
        if (this.pixels == nullptr) { return; }
        let n: int = this.width * this.height;
        let i: int = 0;
        while (i < n) {
            let o: int = i * 4;
            let r: int = this.pixels[o] as int;
            let g: int = this.pixels[o + 1] as int;
            let b: int = this.pixels[o + 2] as int;
            let l: u8 = ((r * 299 + g * 587 + b * 114) / 1000) as u8;
            this.pixels[o] = l; this.pixels[o + 1] = l; this.pixels[o + 2] = l;
            i = i + 1;
        }
    }

    // Draw `src` onto this image at (dx, dy), source-over alpha blending.
    blit(src: Image*, dx: int, dy: int) => void {
        if (this.pixels == nullptr || src.pixels == nullptr) { return; }
        let y: int = 0;
        while (y < src.height) {
            let ty: int = dy + y;
            if (ty >= 0 && ty < this.height) {
                let x: int = 0;
                while (x < src.width) {
                    let tx: int = dx + x;
                    if (tx >= 0 && tx < this.width) {
                        let s: int = (y * src.width + x) * 4;
                        let d: int = (ty * this.width + tx) * 4;
                        let sa: int = src.pixels[s + 3] as int;
                        if (sa == 255) {
                            this.pixels[d]     = src.pixels[s];
                            this.pixels[d + 1] = src.pixels[s + 1];
                            this.pixels[d + 2] = src.pixels[s + 2];
                            this.pixels[d + 3] = 255;
                        } else if (sa > 0) {
                            let da: int = this.pixels[d + 3] as int;
                            let oa: int = sa + (da * (255 - sa)) / 255;
                            let k: int = 0;
                            while (k < 3) {
                                let sc: int = src.pixels[s + k] as int;
                                let dc: int = this.pixels[d + k] as int;
                                let c: int = (sc * sa + (dc * da * (255 - sa)) / 255) / oa;
                                this.pixels[d + k] = c as u8;
                                k = k + 1;
                            }
                            this.pixels[d + 3] = oa as u8;
                        }
                    }
                    x = x + 1;
                }
            }
            y = y + 1;
        }
    }

    // ---------------- lifetime ----------------

    free() => void {
        if (this.pixels != nullptr) { C.om_free(this.pixels as void*); }
        this.pixels = nullptr;
        this.width = 0;
        this.height = 0;
    }
}
