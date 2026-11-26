    // ---- portable images: works on every backend, returns an id for image()/imageTinted()/imageFlipped() ----
    findSlot() => int {
        let r: int = 0 - 1;
        let i: int = 1;
        while (i < MAX_IMAGES) {
            if (r < 0 && this.imgUsed[i] == 0) r = i;
            i = i + 1;
        }
        return r;
    }

    // SVG bytes -> image id (0 on failure). scale: 2.0 = rasterize at 2x intrinsic size.
    // rgb = 0xRRGGBB, used as the CSS color so currentColor resolves. Works on every backend.
    // outW/outH receive the INTRINSIC (1x) size, for layout; the image itself is scale times bigger.
    createImageSvg(data: u8*, len: int, scale: float, rgb: u32, outW: float*, outH: float*) => u32 {
        return this.createImageSvgFx(data, len, scale, rgb, outW, outH, false);
    }

    // same, with invert = true the rasterised RGB is inverted (alpha kept): CSS filter: invert(100%) on an <img> svg
    createImageSvgFx(data: u8*, len: int, scale: float, rgb: u32, outW: float*, outH: float*, invert: bool) => u32 {
        let result: u32 = 0;
        let sw: float = 0.0; let sh: float = 0.0;
        if (data == nullptr || len <= 0) { Console.log("[createImageSvg] no data"); return 0; }
        if (SVG.svg_size(data as char*, len, &sw, &sh) == 1) {
            if (sw < 1.0) sw = 24.0;
            if (sh < 1.0) sh = 24.0;
            // clamp the raster to 2048 px per side (huge intrinsic sizes, or a big scale, used to fail with "bad size")
            let sc: float = scale;
            if (sw * sc > 2048.0) sc = 2048.0 / sw;
            if (sh * sc > 2048.0) sc = 2048.0 / sh;
            let pw: int = (sw * sc) as int;
            let ph: int = (sh * sc) as int;
            if (pw > 0 && ph > 0 && pw <= 4096 && ph <= 4096) {
                let buf = CMem.malloc((pw * ph * 4) as u64) as u8*;
                if (buf != nullptr) {
                    if (SVG.svg_rasterize(data as char*, len, pw, ph, rgb, buf) == 1) {
                        if (invert) {
                            let kk: int = 0; let tot: int = pw * ph * 4;
                            while (kk < tot) {
                                buf[kk] = (255 - (buf[kk] as int)) as u8;
                                buf[kk + 1] = (255 - (buf[kk + 1] as int)) as u8;
                                buf[kk + 2] = (255 - (buf[kk + 2] as int)) as u8;
                                kk = kk + 4;
                            }
                        }
                        result = this.createImage(buf, pw, ph);
                        if (result > 0) { *outW = sw; *outH = sh; }
                    } else { Console.log("[createImageSvg] rasterize failed"); }
                    CMem.free(buf as void*);
                } else { Console.log("[createImageSvg] malloc failed"); }
            } else { Console.log("[createImageSvg] bad size"); }
        } else { Console.log("[createImageSvg] not a valid SVG"); }
        return result;
    }

    // px: RGBA8, w*h*4 bytes, row 0 = top. Returns 0 on failure. Free with freeImage().
    createImage(px: u8*, w: int, hh: int) => u32 {
        if (px == nullptr) { Console.log("[createImage] px is NULL"); return 0; }
        if (w <= 0 || hh <= 0) { Console.log("[createImage] bad size"); return 0; }
        let result: u32 = 0;
        if (this.backend == API_GL) {
            G.glfwMakeContextCurrent(this.h);
            let t: OGL.Texture;
            t.fromRGBA(px, w, hh, false);
            result = t.id;
        } else {
            let slot: int = this.findSlot();
            if (slot >= 0) {
                if (this.backend == API_SOFTWARE) {
                    let copy = CMem.malloc((w * hh * 4) as u64) as u8*;
                    if (copy != nullptr) {
                        copyBytes(copy, px, (w * hh * 4) as u64);
                        this.imgPx[slot] = copy;
                        this.imgW[slot] = w; this.imgH[slot] = hh;
                        this.imgUsed[slot] = 1 as u8;
                        result = (slot + 1) as u32;
                    } else { Console.log("[createImage] SW: malloc failed"); }
                } else {
                    if (this.vk.createTexture(&this.vkTex[slot], px, w, hh)) {
                        this.imgW[slot] = w; this.imgH[slot] = hh;
                        this.imgUsed[slot] = 1 as u8;
                        result = (slot + 1) as u32;
                    } else { Console.log("[createImage] VK: createTexture failed"); }
                }
            }
        }
        return result;
    }

    freeImage(id: u32) => void {
        if (id == 0) return;
        if (this.backend == API_GL) { G.glfwMakeContextCurrent(this.h); let tid: u32 = id; GL.glDeleteTextures(1, &tid); return; }
        let slot: int = (id as int) - 1;
        if (slot < 1 || slot >= MAX_IMAGES || this.imgUsed[slot] == 0) return;
        if (this.backend == API_SOFTWARE) { CMem.free(this.imgPx[slot] as void*); this.imgPx[slot] = nullptr; }
        else { this.vk.destroyTexture(&this.vkTex[slot]); }
        this.imgUsed[slot] = 0 as u8;
    }
