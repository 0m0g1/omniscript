    // ---- text (8x16 glyphs * scale; '\n' starts a new line; UTF-8 input, the atlas only has ASCII 32..126 so other code points are folded to ASCII look-alikes) ----
    text(x: float, y: float, s: char*, c: u32) => void { this.textScaled(x, y, s, c, 1.0); }

    // byte value 0..255 of s[i] (char may be signed)
    u8at(s: char*, i: int) => int { return (((s[i] as int) % 256) + 256) % 256; }

    // decodes the UTF-8 code point at s[i]; *len = bytes used (1..4; an invalid byte counts as 1 and gives 63)
    cpAt(s: char*, i: int, len: int*) => int {
        let b0: int = this.u8at(s, i);
        let cp: int = b0; let n: int = 1;
        if (b0 >= 128) {
            cp = 63;
            if (b0 >= 194 && b0 <= 223) {
                let b1: int = this.u8at(s, i + 1);
                if (b1 >= 128 && b1 <= 191) { cp = (b0 - 192) * 64 + (b1 - 128); n = 2; }
            }
            else if (b0 >= 224 && b0 <= 239) {
                let b1: int = this.u8at(s, i + 1);
                if (b1 >= 128 && b1 <= 191) {
                    let b2: int = this.u8at(s, i + 2);
                    if (b2 >= 128 && b2 <= 191) { cp = (b0 - 224) * 4096 + (b1 - 128) * 64 + (b2 - 128); n = 3; }
                }
            }
            else if (b0 >= 240 && b0 <= 244) {
                let b1: int = this.u8at(s, i + 1);
                if (b1 >= 128 && b1 <= 191) {
                    let b2: int = this.u8at(s, i + 2);
                    if (b2 >= 128 && b2 <= 191) {
                        let b3: int = this.u8at(s, i + 3);
                        if (b3 >= 128 && b3 <= 191) { cp = (b0 - 240) * 262144 + (b1 - 128) * 4096 + (b2 - 128) * 64 + (b3 - 128); n = 4; }
                    }
                }
            }
        }
        *len = n;
        return cp;
    }

    // ASCII look-alike for a code point the atlas does not have (nullptr = draw cp itself; "" = draw nothing)
    foldText(cp: int) => char* {
        if (cp >= 32 && cp <= 126) return nullptr;
        if (cp == 9 || cp == 160 || cp == 8194 || cp == 8195 || cp == 8201 || cp == 8239) return " ";
        if (cp == 13 || cp == 8203 || cp == 65279 || cp == 173) return "";
        if (cp == 169) return "(c)";
        if (cp == 174) return "(R)";
        if (cp == 8482) return "TM";
        if (cp == 8216 || cp == 8217 || cp == 8242) return "'";
        if (cp == 8220 || cp == 8221 || cp == 8243) return "''";
        if (cp == 8211 || cp == 8212 || cp == 8722 || cp == 8208) return "-";
        if (cp == 8230) return "...";
        if (cp == 8226 || cp == 183) return "*";
        if (cp == 215) return "x";
        if (cp == 8594) return "->";
        if (cp == 8592) return "<-";
        if (cp == 171) return "<<";
        if (cp == 187) return ">>";
        return "?";
    }

    foldLen(rep: char*) => int {
        let k: int = 0; let go: bool = true;
        while (go) { let c0: int = rep[k] as int; if (c0 == 0) go = false; else k = k + 1; }
        return k;
    }

    textScaled(x: float, y: float, s: char*, c: u32, scale: float) => void {
        let gw: float = (F.GLYPH_W as float) * scale;
        let gh: float = (F.GLYPH_H as float) * scale;
        let px: float = x; let py: float = y;
        let aw: float = F.ATLAS_W as float; let ah: float = F.ATLAS_H as float;
        let i: int = 0;
        let more: bool = true;
        while (more) {
            let ul: int = 1;
            let cp: int = this.cpAt(s, i, &ul);
            if (cp == 0) { more = false; }
            else {
                if (cp == 10) { px = x; py = py + gh; }
                else {
                    let rep: char* = this.foldText(cp);
                    let ri: int = 0; let draw: bool = true;
                    while (draw) {
                        let ch: int = cp;
                        if (rep != nullptr) { ch = rep[ri] as int; }
                        if (ch == 0) { draw = false; }
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
                            ri = ri + 1;
                            if (rep == nullptr) draw = false;
                        }
                    }
                }
                i = i + ul;
            }
        }
    }

    textWidth(s: char*, scale: float) => float {
        let n: int = 0; let best: int = 0;
        let i: int = 0; let more: bool = true;
        while (more) {
            let ul: int = 1;
            let cp: int = this.cpAt(s, i, &ul);
            if (cp == 0) { more = false; }
            else {
                if (cp == 10) { n = 0; }
                else {
                    let rep: char* = this.foldText(cp);
                    if (rep == nullptr) n = n + 1; else n = n + this.foldLen(rep);
                    if (n > best) best = n;
                }
                i = i + ul;
            }
        }
        return (best * F.GLYPH_W) as float * scale;
    }
    textHeight(scale: float) => float { return (F.GLYPH_H as float) * scale; }
