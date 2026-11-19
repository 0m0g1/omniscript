    // ---- clipping ----
    curClipX() => int { return this.clipStack[(this.clipDepth - 1) * 4]; }
    curClipY() => int { return this.clipStack[(this.clipDepth - 1) * 4 + 1]; }
    curClipW() => int { return this.clipStack[(this.clipDepth - 1) * 4 + 2]; }
    curClipH() => int { return this.clipStack[(this.clipDepth - 1) * 4 + 3]; }

    // Nested clips intersect. Max depth 16.
    pushClip(x: int, y: int, cw: int, chh: int) => void {
        if (this.clipDepth >= 16) return;
        let x0: int = x; let y0: int = y; let x1: int = x + cw; let y1: int = y + chh;
        if (this.clipDepth > 0) {
            let px0: int = this.curClipX(); let py0: int = this.curClipY();
            let px1: int = px0 + this.curClipW(); let py1: int = py0 + this.curClipH();
            if (px0 > x0) x0 = px0;
            if (py0 > y0) y0 = py0;
            if (px1 < x1) x1 = px1;
            if (py1 < y1) y1 = py1;
        }
        if (x1 < x0) x1 = x0;
        if (y1 < y0) y1 = y0;
        let o: int = this.clipDepth * 4;
        this.clipStack[o] = x0; this.clipStack[o + 1] = y0; this.clipStack[o + 2] = x1 - x0; this.clipStack[o + 3] = y1 - y0;
        this.clipDepth = this.clipDepth + 1;
    }
    popClip() => void { if (this.clipDepth > 0) this.clipDepth = this.clipDepth - 1; }
