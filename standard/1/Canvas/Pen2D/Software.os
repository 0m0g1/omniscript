    // ---- software rasteriser ----
    flushSoftware() => void {
        let n: u64 = this.cmds.len;
        if (this.swBuf == nullptr || n == 0) { this.reset(); return; }
        let ci: u64 = 0;
        while (ci < n) {
            let d: DrawCmd = this.cmds.data[ci];
            if (d.prog == 0) {
                let cx0: int = 0; let cy0: int = 0; let cx1: int = this.swW; let cy1: int = this.swH;
                if (d.clipOn != 0) {
                    cx0 = clampi(d.cx, 0, this.swW); cy0 = clampi(d.cy, 0, this.swH);
                    cx1 = clampi(d.cx + d.cw, 0, this.swW); cy1 = clampi(d.cy + d.ch, 0, this.swH);
                }
                let tris: u32 = d.count / 3;
                let t: u32 = 0;
                while (t < tris) {
                    let o: u32 = d.first + t * 3;
                    this.swTri(d.tex, this.idx.data[o], this.idx.data[o + 1], this.idx.data[o + 2], cx0, cy0, cx1, cy1);
                    t = t + 1;
                }
            }
            ci = ci + 1;
        }
        this.reset();
    }

    swTri(tex: u32, i0: u32, i1: u32, i2: u32, cx0: int, cy0: int, cx1: int, cy1: int) => void {
        let vd: float* = this.verts.data;
        let a: u64 = (i0 as u64) * 8; let b: u64 = (i1 as u64) * 8; let c: u64 = (i2 as u64) * 8;
        let x0: float = vd[a]; let y0: float = vd[a + 1];
        let x1: float = vd[b]; let y1: float = vd[b + 1];
        let x2: float = vd[c]; let y2: float = vd[c + 1];
        let area: float = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
        if (area == 0.0) return;
        let fminx: float = x0; if (x1 < fminx) fminx = x1; if (x2 < fminx) fminx = x2;
        let fmaxx: float = x0; if (x1 > fmaxx) fmaxx = x1; if (x2 > fmaxx) fmaxx = x2;
        let fminy: float = y0; if (y1 < fminy) fminy = y1; if (y2 < fminy) fminy = y2;
        let fmaxy: float = y0; if (y1 > fmaxy) fmaxy = y1; if (y2 > fmaxy) fmaxy = y2;
        if (fmaxx < 0.0 || fmaxy < 0.0) return;
        if (fminx < 0.0) fminx = 0.0;
        if (fminy < 0.0) fminy = 0.0;
        let minx: int = clampi(fminx as int, cx0, cx1);
        let miny: int = clampi(fminy as int, cy0, cy1);
        let maxx: int = clampi((fmaxx as int) + 1, cx0, cx1);
        let maxy: int = clampi((fmaxy as int) + 1, cy0, cy1);
        let slot: int = (tex as int) - 1;
        if (slot < 0 || slot >= MAX_IMAGES || this.imgUsed[slot] == 0) return;
        let tp: u8* = this.imgPx[slot];
        let tw: int = this.imgW[slot]; let th: int = this.imgH[slot];
        let inv: float = 1.0 / area;
        let y: int = miny;
        while (y < maxy) {
            let py: float = (y as float) + 0.5;
            let x: int = minx;
            while (x < maxx) {
                let px: float = (x as float) + 0.5;
                let w0: float = ((x1 - px) * (y2 - py) - (x2 - px) * (y1 - py)) * inv;
                let w1: float = ((x2 - px) * (y0 - py) - (x0 - px) * (y2 - py)) * inv;
                let w2: float = 1.0 - w0 - w1;
                if (w0 >= 0.0 && w1 >= 0.0 && w2 >= 0.0) {
                    let u: float = w0 * vd[a + 2] + w1 * vd[b + 2] + w2 * vd[c + 2];
                    let v: float = w0 * vd[a + 3] + w1 * vd[b + 3] + w2 * vd[c + 3];
                    let tx: int = clampi((u * (tw as float)) as int, 0, tw - 1);
                    let ty: int = clampi((v * (th as float)) as int, 0, th - 1);
                    let ti: int = (ty * tw + tx) * 4;
                    let cr: float = w0 * vd[a + 4] + w1 * vd[b + 4] + w2 * vd[c + 4];
                    let cg: float = w0 * vd[a + 5] + w1 * vd[b + 5] + w2 * vd[c + 5];
                    let cb: float = w0 * vd[a + 6] + w1 * vd[b + 6] + w2 * vd[c + 6];
                    let ca: float = w0 * vd[a + 7] + w1 * vd[b + 7] + w2 * vd[c + 7];
                    // u8 -> int -> float everywhere: u8 -> float directly is suspect on this compiler
                    let t0: int = tp[ti] as int; let t1: int = tp[ti + 1] as int;
                    let t2: int = tp[ti + 2] as int; let t3: int = tp[ti + 3] as int;
                    let sa: float = (t3 as float) / 255.0 * ca;
                    let di: int = (y * this.swW + x) * 4;
                    let isa: float = 1.0 - sa;
                    let sr: float = (t0 as float) * cr;
                    let sg: float = (t1 as float) * cg;
                    let sb: float = (t2 as float) * cb;
                    let d0: int = this.swBuf[di] as int; let d1: int = this.swBuf[di + 1] as int;
                    let d2: int = this.swBuf[di + 2] as int; let d3: int = this.swBuf[di + 3] as int;
                    // route through int and clamp: float -> u8 directly is suspect on this compiler
                    let rv: int = (sr * sa + (d0 as float) * isa + 0.5) as int;
                    let gv: int = (sg * sa + (d1 as float) * isa + 0.5) as int;
                    let bv: int = (sb * sa + (d2 as float) * isa + 0.5) as int;
                    let av: int = ((sa + (d3 as float) / 255.0 * isa) * 255.0 + 0.5) as int;
                    if (rv > 255) rv = 255; if (rv < 0) rv = 0;
                    if (gv > 255) gv = 255; if (gv < 0) gv = 0;
                    if (bv > 255) bv = 255; if (bv < 0) bv = 0;
                    if (av > 255) av = 255; if (av < 0) av = 0;
                    this.swBuf[di] = rv as u8;
                    this.swBuf[di + 1] = gv as u8;
                    this.swBuf[di + 2] = bv as u8;
                    this.swBuf[di + 3] = av as u8;
                }
                x = x + 1;
            }
            y = y + 1;
        }
    }
