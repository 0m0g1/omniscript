    free() => void {
        if (!this.ready) return;
        this.freeWidgets();
        if (this.backend == API_GL) {
            G.glfwMakeContextCurrent(this.h);
            this.prog.free();
            if (this.vbo != 0) OGL.glDeleteBuffers(1, &this.vbo);
            if (this.ebo != 0) OGL.glDeleteBuffers(1, &this.ebo);
            if (this.vao != 0) OGL.glDeleteVertexArrays(1, &this.vao);
            if (this.atlas != 0) GL.glDeleteTextures(1, &this.atlas);
        } else {
            if (this.backend == API_VULKAN) this.freeVulkan();
            let i: int = 0;
            while (i < MAX_IMAGES) {
                if (this.backend == API_SOFTWARE && this.imgPx[i] != nullptr) CMem.free(this.imgPx[i] as void*);
                i = i + 1;
            }
            CMem.free(this.imgUsed as void*); CMem.free(this.imgW as void*); CMem.free(this.imgH as void*); CMem.free(this.imgPx as void*);
            if (this.swBuf != nullptr) { CMem.free(this.swBuf as void*); this.swBuf = nullptr; }
        }
        this.verts.free(); this.idx.free(); this.cmds.free();
        if (this.clipStack != nullptr) CMem.free(this.clipStack as void*);
        this.clipStack = nullptr; this.ready = false;
    }

    // Called by Win each frame (size = framebuffer pixels).
    begin(w: int, hh: int) => void {
        this.w = w; this.ht = hh;
        if (this.backend == API_SOFTWARE) this.swResize(w, hh);
        this.verts.len = 0; this.idx.len = 0; this.cmds.len = 0;
        this.vcount = 0; this.clipDepth = 0;
    }

    width() => int { return this.w; }
    height() => int { return this.ht; }

    // Clears the window immediately (colour + depth). Call after the frame started.
    clear(r: float, g: float, b: float, a: float) => void {
        this.clearR = r; this.clearG = g; this.clearB = b; this.clearA = a;
        if (this.backend == API_GL) {
            G.glfwMakeContextCurrent(this.h);
            OGL.viewport(this.w, this.ht);
            OGL.clear(r, g, b, a);
        }
        if (this.backend == API_SOFTWARE && this.swBuf != nullptr) {
            let rb: u8 = (r * 255.0 + 0.5) as u8; let gb: u8 = (g * 255.0 + 0.5) as u8;
            let bb: u8 = (b * 255.0 + 0.5) as u8; let ab: u8 = (a * 255.0 + 0.5) as u8;
            let i: int = 0;
            let total: int = this.swW * this.swH;
            while (i < total) {
                this.swBuf[i * 4] = rb; this.swBuf[i * 4 + 1] = gb; this.swBuf[i * 4 + 2] = bb; this.swBuf[i * 4 + 3] = ab;
                i = i + 1;
            }
        }
        // Vulkan: the colour is applied when the frame's render pass begins (flush).
    }
