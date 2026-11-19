    // ---- text (8x16 glyphs * scale; '\n' starts a new line) ----
    text(x: float, y: float, s: char*, c: u32) => void { this.textScaled(x, y, s, c, 1.0); }

    textScaled(x: float, y: float, s: char*, c: u32, scale: float) => void {
        let gw: float = (F.GLYPH_W as float) * scale;
        let gh: float = (F.GLYPH_H as float) * scale;
        let px: float = x; let py: float = y;
        let aw: float = F.ATLAS_W as float; let ah: float = F.ATLAS_H as float;
        let i: int = 0;
        let more: bool = true;
        while (more) {
            let ch: int = s[i] as int;
            if (ch == 0) { more = false; }
            else {
                if (ch == 10) { px = x; py = py + gh; }
                else {
                    if (ch < 32 || ch > 126) ch = 63;
                    if (ch != 32) {
                        let gi: int = ch - 32;
                        // inset the UVs by a hair so NEAREST never picks a texel from the neighbouring glyph
                        let eu: float = 0.02 / aw; let ev: float = 0.02 / ah;
                        let u0: float = ((gi % 16) * F.GLYPH_W) as float / aw + eu;
                        let v0: float = ((gi / 16) * F.GLYPH_H) as float / ah + ev;
                        let u1: float = u0 - eu + (F.GLYPH_W as float) / aw - eu;
                        let v1: float = v0 - ev + (F.GLYPH_H as float) / ah - ev;
                        // snap each glyph to whole pixels (width = difference of snapped edges, so no gaps)
                        let sx0: float = snap(px); let sy0: float = snap(py);
                        let sx1: float = snap(px + gw); let sy1: float = snap(py + gh);
                        this.quad(sx0, sy0, sx1 - sx0, sy1 - sy0, u0, v0, u1, v1, c, this.atlas, 0);
                    }
                    px = px + gw;
                }
                i = i + 1;
            }
        }
    }

    textWidth(s: char*, scale: float) => float {
        let n: int = 0; let best: int = 0;
        let i: int = 0; let more: bool = true;
        while (more) {
            let ch: int = s[i] as int;
            if (ch == 0) { more = false; }
            else {
                if (ch == 10) { n = 0; } else { n = n + 1; if (n > best) best = n; }
                i = i + 1;
            }
        }
        return (best * F.GLYPH_W) as float * scale;
    }
    textHeight(scale: float) => float { return (F.GLYPH_H as float) * scale; }
