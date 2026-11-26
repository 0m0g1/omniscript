    // ---- shapes ----
    fillRect(x: float, y: float, w: float, hh: float, c: u32) => void {
        let u: float = this.whiteU(); let v: float = this.whiteV();
        this.quad(x, y, w, hh, u, v, u, v, c, this.atlas, 0);
    }

    // two-colour gradient rectangle: c0 at the left / top edge, c1 at the right / bottom edge (vertical = top to bottom).
    // The colours are per-vertex, which every backend (GL, Vulkan, software) already interpolates.
    fillGradient(x: float, y: float, w: float, hh: float, c0: u32, c1: u32, vertical: bool) => void {
        if (w <= 0.0 || hh <= 0.0) return;
        let u: float = this.whiteU(); let v: float = this.whiteV();
        this.ensure(this.atlas, 0, 0.0, 0.0);
        let base: u32 = this.vcount;
        if (vertical) {
            this.vert(x, y, u, v, c0); this.vert(x + w, y, u, v, c0);
            this.vert(x + w, y + hh, u, v, c1); this.vert(x, y + hh, u, v, c1);
        }
        else {
            this.vert(x, y, u, v, c0); this.vert(x + w, y, u, v, c1);
            this.vert(x + w, y + hh, u, v, c1); this.vert(x, y + hh, u, v, c0);
        }
        this.tri(0, 1, 2, base); this.tri(0, 2, 3, base);
        this.grow(6);
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

    // ---- rounded rectangles ----
    // Built from rects + triangle fans that never overlap, so translucent colours blend correctly. No anti-aliasing.
    roundSegs(r: float) => int {
        let n: int = ((r * 0.5) as int) + 4;
        if (n > 16) n = 16;
        return n;
    }

    // quarter disc centred (cx, cy), sweeping from angle a0 to a0 + PI/2 (y grows downward)
    cornerFan(cx: float, cy: float, r: float, a0: float, c: u32) => void {
        let n: int = this.roundSegs(r);
        let i: int = 0;
        while (i < n) {
            let a1: float = a0 + (i as float) * (PI * 0.5) / (n as float);
            let a2: float = a0 + ((i + 1) as float) * (PI * 0.5) / (n as float);
            this.fillTriangle(cx, cy, cx + mcos(a1) * r, cy + msin(a1) * r, cx + mcos(a2) * r, cy + msin(a2) * r, c);
            i = i + 1;
        }
    }

    // quarter ring between radius ri and ro around (cx, cy)
    cornerRing(cx: float, cy: float, ro: float, ri: float, a0: float, c: u32) => void {
        let n: int = this.roundSegs(ro);
        let i: int = 0;
        while (i < n) {
            let a1: float = a0 + (i as float) * (PI * 0.5) / (n as float);
            let a2: float = a0 + ((i + 1) as float) * (PI * 0.5) / (n as float);
            let ox1: float = cx + mcos(a1) * ro; let oy1: float = cy + msin(a1) * ro;
            let ox2: float = cx + mcos(a2) * ro; let oy2: float = cy + msin(a2) * ro;
            let ix1: float = cx + mcos(a1) * ri; let iy1: float = cy + msin(a1) * ri;
            let ix2: float = cx + mcos(a2) * ri; let iy2: float = cy + msin(a2) * ri;
            this.fillTriangle(ox1, oy1, ox2, oy2, ix2, iy2, c);
            this.fillTriangle(ox1, oy1, ix2, iy2, ix1, iy1, c);
            i = i + 1;
        }
    }

    // filled rounded rectangle; r is clamped to half the shorter side (a huge r gives a pill / circle)
    fillRoundRect(x: float, y: float, w: float, hh: float, r0: float, c: u32) => void {
        if (w <= 0.0 || hh <= 0.0) return;
        let r: float = r0;
        let m: float = w;
        if (hh < m) m = hh;
        if (r > m * 0.5) r = m * 0.5;
        if (r < 0.75) { this.fillRect(x, y, w, hh, c); return; }
        this.fillRect(x + r, y, w - 2.0 * r, hh, c);
        this.fillRect(x, y + r, r, hh - 2.0 * r, c);
        this.fillRect(x + w - r, y + r, r, hh - 2.0 * r, c);
        this.cornerFan(x + r, y + r, r, PI, c);
        this.cornerFan(x + w - r, y + r, r, PI * 1.5, c);
        this.cornerFan(x + w - r, y + hh - r, r, 0.0, c);
        this.cornerFan(x + r, y + hh - r, r, PI * 0.5, c);
    }

    // filled rounded rectangle with a radius per corner (top-left, top-right, bottom-right, bottom-left).
    // Radii are scaled down together when two neighbours do not fit a side (CSS rule). Same non-overlapping construction.
    fillRoundRect4(x: float, y: float, w: float, hh: float, tl0: float, tr0: float, br0: float, bl0: float, c: u32) => void {
        if (w <= 0.0 || hh <= 0.0) return;
        let tl: float = tl0; let tr: float = tr0; let br: float = br0; let bl: float = bl0;
        if (tl < 0.0) tl = 0.0; if (tr < 0.0) tr = 0.0; if (br < 0.0) br = 0.0; if (bl < 0.0) bl = 0.0;
        let f: float = 1.0;
        if (tl + tr > w && w / (tl + tr) < f) f = w / (tl + tr);
        if (bl + br > w && w / (bl + br) < f) f = w / (bl + br);
        if (tl + bl > hh && hh / (tl + bl) < f) f = hh / (tl + bl);
        if (tr + br > hh && hh / (tr + br) < f) f = hh / (tr + br);
        tl = tl * f; tr = tr * f; br = br * f; bl = bl * f;
        let ht: float = tl; if (tr > ht) ht = tr;
        let hb: float = bl; if (br > hb) hb = br;
        if (ht < 0.75 && hb < 0.75) { this.fillRect(x, y, w, hh, c); return; }
        if (hh - ht - hb > 0.0) this.fillRect(x, y + ht, w, hh - ht - hb, c);
        if (ht > 0.0 && w - tl - tr > 0.0) this.fillRect(x + tl, y, w - tl - tr, ht, c);
        if (ht - tl > 0.0 && tl > 0.0) this.fillRect(x, y + tl, tl, ht - tl, c);
        if (ht - tr > 0.0 && tr > 0.0) this.fillRect(x + w - tr, y + tr, tr, ht - tr, c);
        if (hb > 0.0 && w - bl - br > 0.0) this.fillRect(x + bl, y + hh - hb, w - bl - br, hb, c);
        if (hb - bl > 0.0 && bl > 0.0) this.fillRect(x, y + hh - hb, bl, hb - bl, c);
        if (hb - br > 0.0 && br > 0.0) this.fillRect(x + w - br, y + hh - hb, br, hb - br, c);
        if (tl > 0.5) this.cornerFan(x + tl, y + tl, tl, PI, c);
        if (tr > 0.5) this.cornerFan(x + w - tr, y + tr, tr, PI * 1.5, c);
        if (br > 0.5) this.cornerFan(x + w - br, y + hh - br, br, 0.0, c);
        if (bl > 0.5) this.cornerFan(x + bl, y + hh - bl, bl, PI * 0.5, c);
    }

    // rounded outline of uniform thickness t (the border box is x, y, w, hh)
    strokeRoundRect(x: float, y: float, w: float, hh: float, r0: float, t: float, c: u32) => void {
        if (w <= 0.0 || hh <= 0.0 || t <= 0.0) return;
        let r: float = r0;
        let m: float = w;
        if (hh < m) m = hh;
        if (r > m * 0.5) r = m * 0.5;
        if (r < 0.75) { this.rect(x, y, w, hh, t, c); return; }
        if (t >= m * 0.5) { this.fillRoundRect(x, y, w, hh, r, c); return; }
        let ri: float = r - t;
        if (ri < 0.0) ri = 0.0;
        this.fillRect(x + r, y, w - 2.0 * r, t, c);
        this.fillRect(x + r, y + hh - t, w - 2.0 * r, t, c);
        this.fillRect(x, y + r, t, hh - 2.0 * r, c);
        this.fillRect(x + w - t, y + r, t, hh - 2.0 * r, c);
        this.cornerRing(x + r, y + r, r, ri, PI, c);
        this.cornerRing(x + w - r, y + r, r, ri, PI * 1.5, c);
        this.cornerRing(x + w - r, y + hh - r, r, ri, 0.0, c);
        this.cornerRing(x + r, y + hh - r, r, ri, PI * 0.5, c);
    }
