    // standard/1/Ui/Layout/Flex.os   (fragment: pasted INSIDE `struct Layout { ... }` by Block.os)
    // Flexbox: direction (+reverse), wrap (rows), gap, grow, shrink, basis, align-items/self, justify-content.
    // Items are the in-flow children; non-blank text children are items too (a text node can be laid out as a box).
    // Sizes live on the nodes while laying out: fbase (hypothetical main size), fmain (final), fcross (margin-box cross).
    // Items are laid out once at the origin and then translated into place. Column items are laid out a second time
    // only when their final height differs from the measured one, which keeps nested flex trees from going exponential.
    // NEXT: column wrap, align-content, min-content auto minimum sizes, baseline alignment.

    flexItem(c: Node*) => bool {
        if (!this.inFlow(c)) return false;
        if (c.kind == NK_TEXT) return !isBlank(c.text);
        return true;
    }

    itemFrom(c: Node*) => Node* {
        let p: Node* = c;
        while (p != nullptr && !this.flexItem(p)) p = p.next;
        return p;
    }

    alignOf(c: Node*, cst: Style*) => int {
        if (c.st.alignSelf != AL_AUTO) return c.st.alignSelf;
        return cst.alignItems;
    }

    mainMargins(c: Node*, rowDir: bool) => float {
        if (rowDir) return c.box.ml + c.box.mr;
        return c.box.mt + c.box.mb;
    }

    clampMain(c: Node*, v: float, rowDir: bool) => float {
        let cs: Style* = &c.st; let b: Box* = &c.box;
        let minL: Len* = &cs.minH; let maxL: Len* = &cs.maxH; let pad: float = b.pt + b.pb + b.bt + b.bb;
        if (rowDir) { minL = &cs.minW; maxL = &cs.maxW; pad = b.pl + b.pr + b.bl + b.br; }
        let r: float = v;
        if (maxL.u == LU_PX) { let mx: float = maxL.v; if (cs.boxSizing == 0) mx = mx + pad; if (r > mx) r = mx; }
        if (minL.u == LU_PX) { let mn: float = minL.v; if (cs.boxSizing == 0) mn = mn + pad; if (r < mn) r = mn; }
        return r;
    }

    measureH(c: Node*, cw: float, stretch: bool) => float {
        this.placeBlock(c, 0.0, 0.0, cw, !stretch);
        return c.box.h;
    }

    flexBase(c: Node*, rowDir: bool, mainAvail: float, defMain: bool, cw: float, stretch: bool) => void {
        let cs: Style* = &c.st;
        this.resolveBox(c, cw);
        let b: Box* = &c.box;
        let hp: float = b.pl + b.pr + b.bl + b.br; let vp: float = b.pt + b.pb + b.bt + b.bb;
        let padM: float = vp;
        if (rowDir) padM = hp;
        let base: float = 0.0; let have: bool = false;
        c.flexMeasured = false;
        if (cs.basis.u == LU_PX) { base = cs.basis.v; have = true; if (cs.boxSizing == 0) base = base + padM; }
        else if (cs.basis.u == LU_PCT && defMain) { base = cs.basis.v * mainAvail / 100.0; have = true; }
        if (!have) {
            let ml: Len* = &cs.height;
            if (rowDir) ml = &cs.width;
            if (ml.u == LU_PX) { base = ml.v; if (cs.boxSizing == 0) base = base + padM; have = true; }
            else if (ml.u == LU_PCT && rowDir) { base = ml.v * cw / 100.0; have = true; }
        }
        if (!have) {
            if (rowDir) { base = this.prefW(c) + hp; }
            else { base = this.measureH(c, cw, stretch); c.flexMeasured = true; }
        }
        c.fbase = this.clampMain(c, base, rowDir);
    }

    flexLayoutItem(c: Node*, rowDir: bool, cw: float, cst: Style*) => void {
        let b: Box* = &c.box;
        let stretch: bool = this.alignOf(c, cst) == AL_STRETCH;
        if (rowDir) {
            c.forceW = c.fmain;
            this.placeBlock(c, 0.0, 0.0, cw, false);
            c.forceW = UNSET;
            c.fcross = b.h + b.mt + b.mb;
        } else {
            let need: bool = true;
            let d: float = c.box.h - c.fmain;
            if (d < 0.0) d = 0.0 - d;
            if (c.flexMeasured && d < 0.01) need = false;
            if (need) {
                c.forceH = c.fmain;
                this.placeBlock(c, 0.0, 0.0, cw, !stretch);
                c.forceH = UNSET;
            }
            c.fcross = b.w + b.ml + b.mr;
        }
        c.flexMeasured = false;
    }

    // row items with height:auto grow to the line's cross size
    stretchCross(c: Node*, rowDir: bool, lineCross: float, cw: float) => void {
        if (rowDir && c.st.height.u == LU_AUTO) {
            let want: float = lineCross - c.box.mt - c.box.mb;
            let have: float = c.box.h;
            if (want > have + 0.01) {
                if (c.st.display == DP_FLEX && c.kind == NK_BOX) {
                    let ox: float = c.box.x - c.box.ml; let oy: float = c.box.y - c.box.mt;
                    c.forceW = c.box.w; c.forceH = want;
                    this.placeBlock(c, ox, oy, cw, false);
                    c.forceW = UNSET; c.forceH = UNSET;
                } else {
                    c.box.h = want; c.box.ch = c.box.ch + (want - have);
                }
            }
        }
    }

    flexContent(n: Node*) => float {
        let st: Style* = &n.st; let b: Box* = &n.box;
        let rowDir: bool = st.flexDir == FD_ROW || st.flexDir == FD_ROW_REV;
        let rev: bool = st.flexDir == FD_ROW_REV || st.flexDir == FD_COL_REV;
        let gap: float = st.gap;
        let defMain: bool = rowDir || b.ch >= 0.0;
        let mainAvail: float = b.cw;
        if (!rowDir) mainAvail = b.ch;
        let crossDef: bool = !rowDir || b.ch >= 0.0;
        let crossSize: float = b.cw;
        if (rowDir) crossSize = b.ch;
        let wrap: bool = st.wrap && rowDir;

        // 1. hypothetical main sizes
        let c: Node* = this.itemFrom(n.first);
        while (c != nullptr) {
            let al: int = this.alignOf(c, st);
            this.flexBase(c, rowDir, mainAvail, defMain, b.cw, al == AL_STRETCH);
            c = this.itemFrom(c.next);
        }

        // 2. lines
        let crossPos: float = 0.0; let mainTotal: float = 0.0; let lineNo: int = 0;
        let ls: Node* = this.itemFrom(n.first);
        while (ls != nullptr) {
            let sum: float = 0.0; let cnt: int = 0; let tg: float = 0.0; let ts: float = 0.0;
            let e: Node* = ls; let go: bool = true;
            while (e != nullptr && go) {
                let outer: float = e.fbase + this.mainMargins(e, rowDir);
                let add: float = outer;
                if (cnt > 0) add = add + gap;
                if (wrap && defMain && cnt > 0 && sum + add > mainAvail + 0.01) { go = false; }
                else {
                    sum = sum + add; cnt = cnt + 1;
                    tg = tg + e.st.grow; ts = ts + e.st.shrink * e.fbase;
                    e = this.itemFrom(e.next);
                }
            }
            // e is now the first item of the next line

            // 3. flexible lengths
            let free: float = 0.0;
            if (defMain) free = mainAvail - sum;
            let used: float = 0.0; let it: Node* = ls; let k: int = 0;
            while (k < cnt) {
                let fm: float = it.fbase;
                if (free > 0.0 && tg > 0.0) fm = fm + free * it.st.grow / tg;
                else if (free < 0.0 && ts > 0.0) fm = fm + free * it.st.shrink * it.fbase / ts;
                fm = this.clampMain(it, fm, rowDir);
                if (fm < 0.0) fm = 0.0;
                it.fmain = fm;
                used = used + fm + this.mainMargins(it, rowDir);
                it = this.itemFrom(it.next); k = k + 1;
            }
            let gaps: float = 0.0;
            if (cnt > 1) gaps = gap * ((cnt - 1) as float);
            let remain: float = 0.0;
            if (defMain) remain = mainAvail - used - gaps;
            if (remain < 0.0) remain = 0.0;

            // 4. lay out items, find the line's cross size
            let lineCross: float = 0.0;
            it = ls; k = 0;
            while (k < cnt) {
                this.flexLayoutItem(it, rowDir, b.cw, st);
                lineCross = maxF(lineCross, it.fcross);
                it = this.itemFrom(it.next); k = k + 1;
            }
            if (!rowDir) lineCross = b.cw;
            else if (e == nullptr && lineNo == 0 && crossDef) lineCross = crossSize;

            // 5. justify + align + final positions
            let off: float = 0.0; let between: float = 0.0;
            let jc: int = st.justify;
            if (jc == JC_END) off = remain;
            else if (jc == JC_CENTER) off = remain * 0.5;
            else if (jc == JC_BETWEEN) { if (cnt > 1) between = remain / ((cnt - 1) as float); }
            else if (jc == JC_AROUND) { between = remain / (cnt as float); off = between * 0.5; }
            else if (jc == JC_EVENLY) { between = remain / ((cnt + 1) as float); off = between; }
            let lineMain: float = used + gaps;
            if (defMain) lineMain = mainAvail;
            let pos: float = off;
            it = ls; k = 0;
            while (k < cnt) {
                let outerMain: float = it.fmain + this.mainMargins(it, rowDir);
                let mpos: float = pos;
                if (rev) mpos = lineMain - pos - outerMain;
                let al: int = this.alignOf(it, st);
                let cpos: float = 0.0;
                if (al == AL_END) cpos = lineCross - it.fcross;
                else if (al == AL_CENTER) cpos = (lineCross - it.fcross) * 0.5;
                else if (al == AL_STRETCH) { this.stretchCross(it, rowDir, lineCross, b.cw); }
                let fx: float = b.cx + crossPos + cpos; let fy: float = b.cy + mpos;
                if (rowDir) { fx = b.cx + mpos; fy = b.cy + crossPos + cpos; }
                this.translate(it, fx - (it.box.x - it.box.ml), fy - (it.box.y - it.box.mt));
                pos = pos + outerMain + gap + between;
                it = this.itemFrom(it.next); k = k + 1;
            }

            mainTotal = maxF(mainTotal, used + gaps);
            crossPos = crossPos + lineCross;
            if (e != nullptr) crossPos = crossPos + gap;
            ls = e; lineNo = lineNo + 1;
        }
        if (rowDir) return crossPos;
        return mainTotal;
    }

    // max-content width of a flex container
    flexPref(n: Node*) => float {
        let rowDir: bool = n.st.flexDir == FD_ROW || n.st.flexDir == FD_ROW_REV;
        let sum: float = 0.0; let mx: float = 0.0; let cnt: int = 0;
        let c: Node* = this.itemFrom(n.first);
        while (c != nullptr) {
            let w: float = this.outerPref(c);
            sum = sum + w; mx = maxF(mx, w); cnt = cnt + 1;
            c = this.itemFrom(c.next);
        }
        if (rowDir) { if (cnt > 1) sum = sum + n.st.gap * ((cnt - 1) as float); return sum; }
        return mx;
    }
