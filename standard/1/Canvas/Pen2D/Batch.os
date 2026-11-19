    // ---- batching core ----
    // Starts (or continues) a draw command for this state.
    ensure(tex: u32, prog: u32, rw: float, rh: float) => void {
        let need: bool = true;
        let n: u64 = this.cmds.len;
        if (n > 0 && prog == 0) {
            let last: DrawCmd = this.cmds.data[n - 1];
            let sameClip: bool = true;
            if (this.clipDepth == 0) { if (last.clipOn != 0) sameClip = false; }
            else {
                if (last.clipOn == 0) sameClip = false;
                else if (last.cx != this.curClipX() || last.cy != this.curClipY() || last.cw != this.curClipW() || last.ch != this.curClipH()) sameClip = false;
            }
            if (last.tex == tex && last.prog == 0 && sameClip) need = false;
        }
        if (need) {
            let d: DrawCmd;
            d.tex = tex; d.prog = prog; d.first = this.idx.len as u32; d.count = 0;
            d.rw = rw; d.rh = rh;
            d.clipOn = 0; d.cx = 0; d.cy = 0; d.cw = 0; d.ch = 0;
            if (this.clipDepth > 0) {
                d.clipOn = 1; d.cx = this.curClipX(); d.cy = this.curClipY(); d.cw = this.curClipW(); d.ch = this.curClipH();
            }
            this.cmds.push(d);
        }
    }

    vert(x: float, y: float, u: float, v: float, c: u32) => void {
        this.verts.push(x); this.verts.push(y); this.verts.push(u); this.verts.push(v);
        this.verts.push(chan(c, 24)); this.verts.push(chan(c, 16)); this.verts.push(chan(c, 8)); this.verts.push(chan(c, 0));
        this.vcount = this.vcount + 1;
    }

    tri(a: u32, b: u32, c: u32, base: u32) => void {
        this.idx.push(base + a); this.idx.push(base + b); this.idx.push(base + c);
    }

    grow(k: u32) => void {
        let last: DrawCmd* = &this.cmds.data[this.cmds.len - 1];
        last.count = last.count + k;
    }

    quad(x: float, y: float, w: float, hh: float, u0: float, v0: float, u1: float, v1: float, c: u32, tex: u32, prog: u32) => void {
        this.ensure(tex, prog, w, hh);
        let base: u32 = this.vcount;
        this.vert(x, y, u0, v0, c);
        this.vert(x + w, y, u1, v0, c);
        this.vert(x + w, y + hh, u1, v1, c);
        this.vert(x, y + hh, u0, v1, c);
        this.tri(0, 1, 2, base); this.tri(0, 2, 3, base);
        this.grow(6);
    }

    whiteU() => float { return ((F.WHITE_X + 4) as float) / (F.ATLAS_W as float); }
    whiteV() => float { return ((F.WHITE_Y + 8) as float) / (F.ATLAS_H as float); }
