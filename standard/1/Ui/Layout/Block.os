// standard/1/Ui/Layout/Block.os
// Layout engine: ONE struct, several display families. This file holds the shared core + block and inline flow;
// Flex.os, Grid.os, Table.os and Abs.os are fragments pasted INSIDE the struct body at the end (so they can call each other).
// All coordinates are absolute document coordinates in layout units (Metrics). Entry point: layoutRoot().
//
//   placeBlock(n, x, y, cbw, shrink)  (x, y) = top-left of n's MARGIN box, cbw = containing block content width.
//                                     Resolves the box model, picks the width, lays out the content by display,
//                                     picks the height, sets n.box. forceW / forceH (flex, table, abs) override sizes.
//   Inline flow: consecutive inline-level children of a block form a run; text is broken into lines with
//   white-space handling, atomic inline boxes (inline-block, controls, replaced) sit on the line, text-align applied.
// Known gaps (NEXT): margin collapsing between parent and first/last child, floats, % heights, colspan/rowspan,
// word breaking of over-long words, justify, inline-element padding/borders, grid.

// one atomic inline box on the current line (kept typed, so no void* -> Node* cast is needed)
struct AtomRef { node: Node* = nullptr; next: AtomRef* = nullptr; }

struct LineCtx {
    x0: float = 0.0; cw: float = 0.0; x: float = 0.0; y: float = 0.0;
    lineH: float = 0.0; strut: float = 0.0; trail: float = 0.0;
    count: int = 0; lineNo: int = 0; lastSp: bool = true; wrap: bool = true; ta: int = 0;
    head: Frag* = nullptr; tail: Frag* = nullptr; first: Frag* = nullptr;
    afirst: AtomRef* = nullptr; atail: AtomRef* = nullptr;
}

struct Layout {
    meas: Measure* = nullptr; m: Metrics* = nullptr;
    vw: float = 0.0; vh: float = 0.0; rootNode: Node* = nullptr;
    iw: float = 0.0; ih: float = 0.0;          // scratch result of intrinsic()
    runY: float = 0.0;                          // scratch result of inlineRun()

    // ---- small helpers ----
    adv(px: float) => float { return this.meas.advance(px); }
    lh(st: Style*) => float {
        if (st.lineH > 0.0) return st.lineH;
        return this.meas.lineHeight(st.fontPx);
    }
    resolve(l: Len*, base: float, fb: float) => float {
        if (l.u == LU_PX) return l.v;
        if (l.u == LU_PCT) return l.v * base / 100.0;
        return fb;
    }
    attrNum(n: Node*, name: char*, def: float) => float {
        let s: char* = n.getAttr(name);
        if (s == nullptr) return def;
        return parseFloatStr(s, def);
    }

    resolveBox(n: Node*, cbw: float) => void {
        let st: Style* = &n.st; let b: Box* = &n.box;
        b.autoL = st.ml.u == LU_AUTO; b.autoR = st.mr.u == LU_AUTO;
        b.ml = this.resolve(&st.ml, cbw, 0.0); b.mr = this.resolve(&st.mr, cbw, 0.0);
        b.mt = this.resolve(&st.mt, cbw, 0.0); b.mb = this.resolve(&st.mb, cbw, 0.0);
        b.pl = this.resolve(&st.pl, cbw, 0.0); b.pr = this.resolve(&st.pr, cbw, 0.0);
        b.pt = this.resolve(&st.pt, cbw, 0.0); b.pb = this.resolve(&st.pb, cbw, 0.0);
        b.bl = st.bl; b.br = st.br; b.bt = st.bt; b.bb = st.bb;
    }

    inFlow(c: Node*) => bool {
        if (c.st.display == DP_NONE) return false;
        if (c.kind == NK_HIDDEN) return false;
        if (c.st.position == PS_ABSOLUTE || c.st.position == PS_FIXED) return false;
        let p: Node* = c.parent;
        if (p != nullptr && p.row.spec == SP_DETAILS && !p.open && c.row.spec != SP_SUMMARY) return false;
        return true;
    }

    isBlockLevel(c: Node*) => bool {
        if (c.kind == NK_TEXT) return false;
        let d: int = c.st.display;
        return d == DP_BLOCK || d == DP_FLEX || d == DP_TABLE || d == DP_GRID || d == DP_TABLE_ROW || d == DP_TABLE_CELL || d == DP_TABLE_GROUP;
    }

    // block-level margin collapsing between siblings: both positive -> larger, both negative -> most negative, else sum
    collapse(a: float, b: float) => float {
        if (a >= 0.0 && b >= 0.0) return maxF(a, b);
        if (a < 0.0 && b < 0.0) return minF(a, b);
        return a + b;
    }

    newFrag() => Frag* {
        let f: Frag* = CMem.malloc(sizeof(Frag)) as Frag*;
        CMem.memset(f as void*, 0, sizeof(Frag));
        return f;
    }

    // move a laid-out subtree
    translate(n: Node*, dx: float, dy: float) => void {
        n.box.x = n.box.x + dx; n.box.y = n.box.y + dy; n.box.cx = n.box.cx + dx; n.box.cy = n.box.cy + dy;
        let f: Frag* = n.frags;
        while (f != nullptr) { f.x = f.x + dx; f.y = f.y + dy; f = f.next; }
        let c: Node* = n.first;
        while (c != nullptr) { this.translate(c, dx, dy); c = c.next; }
    }

    // ---- text preparation: n.ctext = n.text with white-space applied ----
    prepText(n: Node*) => void {
        strFree(n.ctext); n.ctext = nullptr;
        if (n.text == nullptr) return;
        let len: int = strLen(n.text);
        let out: char* = CMem.malloc((len + 1) as u64) as char*;
        let ws: int = n.st.whiteSpace;
        let k: int = 0; let i: int = 0; let prevSp: bool = false;
        while (i < len) {
            let ch: int = n.text[i] as int;
            if (ws == WS_PRE || ws == WS_PREWRAP) {
                if (ch == 9) ch = 32;
                if (ch != 13) { out[k] = ch as char; k = k + 1; }
            } else {
                let sp: bool = ch == 32 || ch == 9 || ch == 13 || ch == 10;
                if (ch == 10 && ws == WS_PRELINE) { out[k] = 10 as char; k = k + 1; prevSp = true; }
                else if (sp) { if (!prevSp) { out[k] = 32 as char; k = k + 1; prevSp = true; } }
                else { out[k] = ch as char; k = k + 1; prevSp = false; }
            }
            i = i + 1;
        }
        out[k] = 0 as char;
        n.ctext = out;
    }

    // ---- intrinsic sizes (controls, replaced elements) -> this.iw / this.ih ----
    intrinsic(n: Node*) => void {
        let m: Metrics* = this.m;
        let adv: float = this.adv(n.st.fontPx); let lh: float = this.lh(&n.st); let u: float = m.unit;
        let w: float = 0.0; let h: float = 0.0; let ctl: int = n.ctl;
        let padX: float = m.cellW * 3.0; let padY: float = m.cellH * 0.75; let padT: float = m.cellW * 1.5;
        if (m.unit < 0.5) { padX = 2.0 * m.cellW; padY = 0.0; padT = 0.0; }     // terminal: whole cells
        if (n.kind == NK_CONTROL) {
            if (ctl == CK_BUTTON || ctl == CK_SELECT) {
                if (ctl == CK_SELECT) selectSync(n);
                w = (strLen(ctlCaption(n)) as float) * adv + padX;
                if (ctl == CK_SELECT) w = w + adv * 2.0;
                h = lh + padY;
            }
            else if (ctl == CK_TEXT || ctl == CK_STUB) {
                w = this.attrNum(n, "size", 20.0) * adv + m.cellW * 1.5; h = lh + padY;
            }
            else if (ctl == CK_TEXTAREA) {
                w = this.attrNum(n, "cols", 20.0) * adv + padT; h = this.attrNum(n, "rows", 2.0) * lh + padY;
            }
            else if (ctl == CK_CHECK || ctl == CK_RADIO) { w = m.cellH * 1.375; h = w; }
            else if (ctl == CK_RANGE) { w = m.cellW * 16.0; h = m.cellH * 1.25; }
            else if (ctl == CK_PROGRESS || ctl == CK_METER) { w = m.cellW * 20.0; h = m.cellH; }
        } else {
            // width/height attributes win over the decoded size (browser behaviour); a single attribute keeps the natural aspect ratio
            let aw: float = this.attrNum(n, "width", 0.0); let ah: float = this.attrNum(n, "height", 0.0);
            let nw: float = n.imgW; let nh: float = n.imgH;
            if (aw > 0.0 && ah > 0.0) { w = aw * u; h = ah * u; }
            else if (aw > 0.0) {
                w = aw * u;
                if (nw > 0.0 && nh > 0.0) h = aw * nh / nw * u;
            }
            else if (ah > 0.0) {
                h = ah * u;
                if (nw > 0.0 && nh > 0.0) w = ah * nw / nh * u;
            }
            else {
                if (nw > 0.0) w = nw * u;
                if (nh > 0.0) h = nh * u;
            }
            if (w <= 0.0 && h <= 0.0) {
                if (n.row.spec == SP_IMG) { w = 24.0 * u; h = 24.0 * u; }       // placeholder until an image is attached
                else { w = 300.0 * u; h = 150.0 * u; }
                if (n.tag != nullptr && strEq(n.tag, "audio")) h = 32.0 * u;
            }
        }
        this.iw = w; this.ih = h;
    }

    // ---- preferred (max-content) widths, used for shrink-to-fit ----
    textPref(n: Node*) => float {
        this.prepText(n);
        if (n.ctext == nullptr) return 0.0;
        let best: int = 0; let cur: int = 0; let i: int = 0;
        while (n.ctext[i] as int != 0) {
            if (n.ctext[i] as int == 10) { cur = 0; } else { cur = cur + 1; if (cur > best) best = cur; }
            i = i + 1;
        }
        return (best as float) * this.adv(n.st.fontPx);
    }

    prefW(n: Node*) => float {
        if (n.kind == NK_CONTROL || n.kind == NK_REPLACED) { this.intrinsic(n); return this.iw; }
        if (n.kind == NK_TEXT) return this.textPref(n);
        if (n.st.display == DP_FLEX) return this.flexPref(n);
        if (n.st.display == DP_TABLE) return this.tablePref(n);
        let best: float = 0.0; let run: float = 0.0;
        let c: Node* = n.first;
        while (c != nullptr) {
            if (this.inFlow(c)) {
                if (c.row.spec == SP_BR) { best = maxF(best, run); run = 0.0; }
                else if (this.isBlockLevel(c)) { best = maxF(best, run); run = 0.0; best = maxF(best, this.outerPref(c)); }
                else { run = run + this.outerPref(c); }
            }
            c = c.next;
        }
        return maxF(best, run);
    }

    // margin-box preferred width of c (percent edges count as 0)
    outerPref(c: Node*) => float {
        let st: Style* = &c.st;
        let hp: float = this.resolve(&st.pl, 0.0, 0.0) + this.resolve(&st.pr, 0.0, 0.0) + st.bl + st.br;
        let mg: float = this.resolve(&st.ml, 0.0, 0.0) + this.resolve(&st.mr, 0.0, 0.0);
        let w: float = 0.0;
        if (st.width.u == LU_PX) { w = st.width.v; if (st.boxSizing == 0) w = w + hp; }
        else { w = this.prefW(c) + hp; }
        return w + mg;
    }

    // ---- the box ----
    placeBlock(n: Node*, x: float, y: float, cbw: float, shrink0: bool) => void {
        let st: Style* = &n.st;
        this.resolveBox(n, cbw);
        let b: Box* = &n.box;
        let hpad: float = b.pl + b.pr + b.bl + b.br;
        let vpad: float = b.pt + b.pb + b.bt + b.bb;
        let shrink: bool = shrink0;
        if (st.display == DP_TABLE && st.width.u == LU_AUTO) shrink = true;

        // width (border box)
        let w: float = 0.0; let fixed: bool = false;
        if (n.forceW >= 0.0) { w = n.forceW; fixed = true; }
        else if (st.width.u != LU_AUTO) {
            w = this.resolve(&st.width, cbw, 0.0);
            if (st.boxSizing == 0) w = w + hpad;
            fixed = true;
        }
        else if (n.kind == NK_CONTROL || n.kind == NK_REPLACED) { w = this.prefW(n) + hpad; }
        else if (shrink) {
            w = this.prefW(n) + hpad;
            let avail: float = cbw - b.ml - b.mr;
            if (avail > 0.0 && w > avail) w = avail;
        }
        else { w = cbw - b.ml - b.mr; }
        if (n.forceW < 0.0) {
            if (st.maxW.u != LU_NONE) {
                let mx: float = this.resolve(&st.maxW, cbw, w); if (st.boxSizing == 0) mx = mx + hpad;
                if (w > mx) w = mx;
            }
            if (st.minW.u != LU_AUTO) {
                let mn: float = this.resolve(&st.minW, cbw, 0.0); if (st.boxSizing == 0) mn = mn + hpad;
                if (w < mn) w = mn;
            }
        }
        if (w < hpad) w = hpad;

        // auto margins centre fixed/shrunk boxes
        if (n.forceW < 0.0 && cbw > 0.0 && (fixed || shrink) && (b.autoL || b.autoR)) {
            let free: float = cbw - w - b.ml - b.mr;
            if (free > 0.0) {
                if (b.autoL && b.autoR) { b.ml = b.ml + free * 0.5; b.mr = b.mr + free * 0.5; }
                else if (b.autoL) { b.ml = b.ml + free; }
                else { b.mr = b.mr + free; }
            }
        }

        b.x = x + b.ml; b.y = y + b.mt; b.w = w;
        b.cx = b.x + b.bl + b.pl; b.cy = b.y + b.bt + b.pt;
        b.cw = w - hpad; if (b.cw < 0.0) b.cw = 0.0;

        // height
        let defH: float = UNSET;
        if (n.forceH >= 0.0) defH = n.forceH;
        else if (st.height.u == LU_PX) { defH = st.height.v; if (st.boxSizing == 0) defH = defH + vpad; }
        // aspect-ratio: an auto height follows the width (replaced elements and controls keep their intrinsic size)
        if (defH < 0.0 && st.aspect > 0.0 && n.forceH < 0.0 && st.height.u != LU_PX && n.kind != NK_CONTROL && n.kind != NK_REPLACED) {
            if (st.boxSizing == 1) defH = w / st.aspect;
            else defH = (w - hpad) / st.aspect + vpad;
        }
        // replaced element with a CSS width and auto height: the height follows the decoded aspect ratio
        // (without this an inline <svg style="width:13px"> keeps its natural height and is stretched into a thin strip)
        if (defH < 0.0 && n.kind == NK_REPLACED && n.forceH < 0.0 && st.height.u == LU_AUTO && n.imgW > 0.0 && n.imgH > 0.0
            && (fixed || n.forceW >= 0.0) && this.attrNum(n, "height", 0.0) <= 0.0) {
            defH = (w - hpad) * n.imgH / n.imgW + vpad;
        }
        b.ch = UNSET;
        if (defH >= 0.0) b.ch = maxF(defH - vpad, 0.0);

        let chh: float = this.layoutContent(n);
        let hh: float = chh + vpad;
        if (defH >= 0.0) hh = defH;
        else if (n.kind == NK_CONTROL || n.kind == NK_REPLACED) { this.intrinsic(n); hh = this.ih + vpad; }
        if (n.forceH < 0.0) {
            if (st.maxH.u == LU_PX) { let mxh: float = st.maxH.v; if (st.boxSizing == 0) mxh = mxh + vpad; if (hh > mxh) hh = mxh; }
            if (st.minH.u == LU_PX) { let mnh: float = st.minH.v; if (st.boxSizing == 0) mnh = mnh + vpad; if (hh < mnh) hh = mnh; }
        }
        b.h = hh; b.ch = maxF(hh - vpad, 0.0); b.contentH = chh;

        // positioned descendants, then the relative offset (which moves the whole subtree, abs kids included)
        if (st.position != PS_STATIC || n == this.rootNode) this.layoutAbsKids(n, n);
        if (st.position == PS_RELATIVE) {
            let dx: float = 0.0; let dy: float = 0.0;
            if (st.left.u != LU_AUTO) dx = this.resolve(&st.left, cbw, 0.0);
            else if (st.right.u != LU_AUTO) dx = 0.0 - this.resolve(&st.right, cbw, 0.0);
            if (st.top.u != LU_AUTO) dy = this.resolve(&st.top, 0.0, 0.0);
            else if (st.bottom.u != LU_AUTO) dy = 0.0 - this.resolve(&st.bottom, 0.0, 0.0);
            if (dx != 0.0 || dy != 0.0) this.translate(n, dx, dy);
        }
    }

    // content height (n.box content rect is already set)
    layoutContent(n: Node*) => float {
        if (n.kind == NK_CONTROL || n.kind == NK_REPLACED) { this.intrinsic(n); return this.ih; }
        if (n.kind == NK_TEXT) return this.textBox(n);
        let d: int = n.st.display;
        if (d == DP_FLEX) return this.flexContent(n);
        if (d == DP_GRID) return this.gridContent(n);
        if (d == DP_TABLE) return this.tableContent(n);
        return this.blockContent(n);       // block, inline-block, table-cell, groups
    }

    blockContent(n: Node*) => float {
        let b: Box* = &n.box;
        let y: float = b.cy; let pend: float = 0.0;
        let c: Node* = n.first;
        while (c != nullptr) {
            if (!this.inFlow(c)) { c = c.next; }
            else if (this.isBlockLevel(c)) {
                this.resolveBox(c, b.cw);
                let gap: float = this.collapse(pend, c.box.mt);
                this.placeBlock(c, b.cx, y + gap - c.box.mt, b.cw, false);
                y = c.box.y + c.box.h;
                pend = c.box.mb;
                c = c.next;
            }
            else {
                y = y + pend; pend = 0.0;
                c = this.inlineRun(n, c, y, false);
                y = this.runY;
            }
        }
        return y + pend - b.cy;
    }

    // a lone text node used as a box (flex item, table cell child)
    textBox(n: Node*) => float {
        this.inlineRun(n, n, n.box.cy, true);
        return this.runY - n.box.cy;
    }

    // ---- inline flow ----
    // Lays out inline-level nodes from `start` until a block-level sibling. Result y in this.runY; returns that sibling.
    inlineRun(parent: Node*, start: Node*, y: float, single: bool) => Node* {
        let b: Box* = &parent.box;
        let lc: LineCtx;
        lc.x0 = b.cx; lc.cw = b.cw; lc.x = b.cx; lc.y = y; lc.lineH = 0.0; lc.trail = 0.0;
        lc.count = 0; lc.lineNo = 0; lc.lastSp = true;
        lc.ta = parent.st.textAlign; lc.strut = this.lh(&parent.st);
        lc.wrap = parent.st.whiteSpace != WS_NOWRAP && parent.st.whiteSpace != WS_PRE;
        lc.head = nullptr; lc.tail = nullptr; lc.first = nullptr; lc.afirst = nullptr; lc.atail = nullptr;
        let c: Node* = start; let stop: bool = false;
        if (single) {
            this.inlineItem(start, &lc);
            c = nullptr;
        } else {
            while (c != nullptr && !stop) {
                if (!this.inFlow(c)) { c = c.next; }
                else if (this.isBlockLevel(c)) { stop = true; }
                else { this.inlineItem(c, &lc); c = c.next; }
            }
        }
        this.endLine(&lc, false);
        // atomic frags only live for this run; text frags belong to their node
        let f: Frag* = lc.head;
        while (f != nullptr) { let nx: Frag* = f.lnext; if (f.atom != nullptr) CMem.free(f as void*); f = nx; }
        this.runY = lc.y;
        return c;
    }

    linkFrag(lc: LineCtx*, f: Frag*) => void {
        f.lnext = nullptr;
        if (lc.tail != nullptr) lc.tail.lnext = f; else lc.head = f;
        lc.tail = f;
        if (lc.first == nullptr) lc.first = f;
    }

    inlineItem(n: Node*, lc: LineCtx*) => void {
        if (n.kind == NK_TEXT) { this.inlineText(n, lc); }
        else if (n.row.spec == SP_BR) { this.endLine(lc, true); }
        else if (n.row.spec == SP_WBR) { }
        else if (n.st.display == DP_INLINE && n.kind == NK_BOX) {
            this.resolveBox(n, lc.cw);
            let b: Box* = &n.box;
            if (b.pl > 0.0 || b.pr > 0.0 || b.bl > 0.0 || b.br > 0.0) {
                this.inlineAtomic(n, lc);      // padded/bordered inline: give it a box of its own
            }
            else {
                let c: Node* = n.first;
                while (c != nullptr) { if (this.inFlow(c)) this.inlineItem(c, lc); c = c.next; }
            }
        }
        else { this.inlineAtomic(n, lc); }
    }

    inlineText(n: Node*, lc: LineCtx*) => void {
        n.clearFrags();
        this.prepText(n);
        if (n.ctext == nullptr) return;
        let s: char* = n.ctext; let len: int = strLen(s);
        let st: Style* = &n.st;
        let adv: float = this.adv(st.fontPx); let th: float = this.lh(st);
        let pre: bool = st.whiteSpace == WS_PRE || st.whiteSpace == WS_PREWRAP;
        let wrap: bool = st.whiteSpace == WS_NORMAL || st.whiteSpace == WS_PREWRAP || st.whiteSpace == WS_PRELINE;
        let cur: Frag* = nullptr; let ntail: Frag* = nullptr;
        let i: int = 0;
        while (i < len) {
            let ch: int = s[i] as int;
            if (ch == 10) { this.endLine(lc, true); cur = nullptr; i = i + 1; }
            else if (ch == 32 && !pre) {
                if (!lc.lastSp) { lc.x = lc.x + adv; lc.lastSp = true; lc.trail = adv; }
                i = i + 1;
            }
            else {
                let j: int = i;
                while (j < len && s[j] as int != 10 && (pre || s[j] as int != 32)) j = j + 1;
                let ww: float = ((j - i) as float) * adv;
                if (wrap && lc.count > 0 && lc.x + ww > lc.x0 + lc.cw + 0.01) { this.endLine(lc, false); cur = nullptr; }
                if (cur == nullptr || cur.line != lc.lineNo) {
                    cur = this.newFrag();
                    cur.x = lc.x; cur.y = lc.y; cur.h = th; cur.from = i; cur.len = j - i; cur.line = lc.lineNo;
                    if (ntail != nullptr) ntail.next = cur; else n.frags = cur;
                    ntail = cur;
                    this.linkFrag(lc, cur);
                } else { cur.len = j - cur.from; }
                cur.w = lc.x + ww - cur.x;
                lc.x = lc.x + ww; lc.count = lc.count + 1;
                lc.lineH = maxF(lc.lineH, th); lc.lastSp = false; lc.trail = 0.0;
                i = j;
            }
        }
    }

    inlineAtomic(n: Node*, lc: LineCtx*) => void {
        this.placeBlock(n, lc.x, lc.y, lc.cw, true);
        let b: Box* = &n.box;
        let mw: float = b.w + b.ml + b.mr; let mh: float = b.h + b.mt + b.mb;
        if (lc.wrap && lc.count > 0 && lc.x + mw > lc.x0 + lc.cw + 0.01) {
            this.endLine(lc, false);
            this.translate(n, lc.x - (b.x - b.ml), lc.y - (b.y - b.mt));
        }
        let f: Frag* = this.newFrag();
        f.atom = n as void*;
        f.x = lc.x; f.y = lc.y; f.w = mw; f.h = mh; f.line = lc.lineNo;
        this.linkFrag(lc, f);
        let ar: AtomRef* = CMem.malloc(sizeof(AtomRef)) as AtomRef*;
        ar.node = n; ar.next = nullptr;
        if (lc.atail != nullptr) lc.atail.next = ar; else lc.afirst = ar;
        lc.atail = ar;
        lc.x = lc.x + mw; lc.count = lc.count + 1;
        lc.lineH = maxF(lc.lineH, mh); lc.lastSp = false; lc.trail = 0.0;
    }

    // finish the current line: text-align, bottom-align items, advance y
    endLine(lc: LineCtx*, force: bool) => void {
        if (lc.count == 0 && !force) return;
        let h: float = maxF(lc.lineH, lc.strut);
        let used: float = lc.x - lc.x0 - lc.trail;
        let free: float = lc.cw - used;
        let off: float = 0.0;
        if (lc.ta == TA_CENTER) off = free * 0.5;
        else if (lc.ta == TA_RIGHT) off = free;
        if (off < 0.0) off = 0.0;
        let f: Frag* = lc.first;
        let ar: AtomRef* = lc.afirst;
        while (f != nullptr) {
            let ny: float = lc.y + (h - f.h);
            if (f.atom != nullptr && ar != nullptr) { this.translate(ar.node, off, ny - f.y); ar = ar.next; }
            f.x = f.x + off; f.y = ny;
            f = f.lnext;
        }
        let rr: AtomRef* = lc.afirst;
        while (rr != nullptr) { let rn: AtomRef* = rr.next; CMem.free(rr as void*); rr = rn; }
        lc.afirst = nullptr; lc.atail = nullptr;
        lc.first = nullptr;
        lc.y = lc.y + h; lc.x = lc.x0; lc.lineH = 0.0; lc.trail = 0.0;
        lc.count = 0; lc.lineNo = lc.lineNo + 1; lc.lastSp = true;
    }

    // ---- entry point ----
    // fillWindow: the root is forced to the viewport height (apps). false: it grows with its content (documents).
    layoutRoot(root: Node*, w: float, h: float, fillWindow: bool) => void {
        this.vw = w; this.vh = h; this.rootNode = root;
        root.forceW = w;
        root.forceH = UNSET;
        if (fillWindow) root.forceH = h;
        this.placeBlock(root, 0.0, 0.0, w, false);
        root.forceW = UNSET; root.forceH = UNSET;
        root.dirtyLayout = false;
    }

    include "standard/1/Ui/Layout/Flex.os";
    include "standard/1/Ui/Layout/Grid.os";
    include "standard/1/Ui/Layout/Table.os";
    include "standard/1/Ui/Layout/Abs.os";
}
