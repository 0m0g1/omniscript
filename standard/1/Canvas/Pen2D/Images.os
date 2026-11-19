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
