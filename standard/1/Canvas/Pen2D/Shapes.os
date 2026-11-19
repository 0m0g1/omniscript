    // ---- shapes ----
    fillRect(x: float, y: float, w: float, hh: float, c: u32) => void {
        let u: float = this.whiteU(); let v: float = this.whiteV();
        this.quad(x, y, w, hh, u, v, u, v, c, this.atlas, 0);
    }

    rect(x: float, y: float, w: float, hh: float, t: float, c: u32) => void {
        this.fillRect(x, y, w, t, c);
        this.fillRect(x, y + hh - t, w, t, c);
        this.fillRect(x, y + t, t, hh - 2.0 * t, c);
        this.fillRect(x + w - t, y + t, t, hh - 2.0 * t, c);
    }

    fillTriangle(x1: float, y1: float, x2: float, y2: float, x3: float, y3: float, c: u32) => void {
        let u: float = this.whiteU(); let v: float = this.whiteV();
        this.ensure(this.atlas, 0, 0.0, 0.0);
        let base: u32 = this.vcount;
        this.vert(x1, y1, u, v, c); this.vert(x2, y2, u, v, c); this.vert(x3, y3, u, v, c);
        this.tri(0, 1, 2, base);
        this.grow(3);
    }

    line(x1: float, y1: float, x2: float, y2: float, t: float, c: u32) => void {
        let dx: float = x2 - x1; let dy: float = y2 - y1;
        let l: float = msqrt(dx * dx + dy * dy);
        if (l == 0.0) return;
        let nx: float = (0.0 - dy) / l * t * 0.5;
        let ny: float = dx / l * t * 0.5;
        let u: float = this.whiteU(); let v: float = this.whiteV();
        this.ensure(this.atlas, 0, 0.0, 0.0);
        let base: u32 = this.vcount;
        this.vert(x1 + nx, y1 + ny, u, v, c); this.vert(x2 + nx, y2 + ny, u, v, c);
        this.vert(x2 - nx, y2 - ny, u, v, c); this.vert(x1 - nx, y1 - ny, u, v, c);
        this.tri(0, 1, 2, base); this.tri(0, 2, 3, base);
        this.grow(6);
    }

    circle(cx: float, cy: float, r: float, c: u32) => void {
        let segs: int = 32;
        let u: float = this.whiteU(); let v: float = this.whiteV();
        this.ensure(this.atlas, 0, 0.0, 0.0);
        let base: u32 = this.vcount;
        this.vert(cx, cy, u, v, c);
        let i: int = 0;
        while (i < segs) {
            let a: float = (i as float) * 2.0 * PI / (segs as float);
            this.vert(cx + mcos(a) * r, cy + msin(a) * r, u, v, c);
            i = i + 1;
        }
        i = 0;
        while (i < segs) {
            let nxt: int = (i + 1) % segs;
            this.tri(0, (i + 1) as u32, (nxt + 1) as u32, base);
            i = i + 1;
        }
        this.grow((segs * 3) as u32);
    }

    // ---- images (GL texture ids, e.g. OpenGL.Texture.id or Framebuffer.tex) ----
    image(tex: u32, x: float, y: float, w: float, hh: float) => void {
        this.quad(x, y, w, hh, 0.0, 0.0, 1.0, 1.0, 0xFFFFFFFF, tex, 0);
    }
    imageTinted(tex: u32, x: float, y: float, w: float, hh: float, c: u32) => void {
        this.quad(x, y, w, hh, 0.0, 0.0, 1.0, 1.0, c, tex, 0);
    }
    // Render-target textures are bottom-up: this flips v.
    imageFlipped(tex: u32, x: float, y: float, w: float, hh: float) => void {
        this.quad(x, y, w, hh, 0.0, 1.0, 1.0, 0.0, 0xFFFFFFFF, tex, 0);
    }

    // Fills a rect with a program from createShader(). Breaks the batch.
    shaderRect(p: u32, x: float, y: float, w: float, hh: float) => void {
        this.quad(x, y, w, hh, 0.0, 0.0, 1.0, 1.0, 0xFFFFFFFF, this.atlas, p);
    }
