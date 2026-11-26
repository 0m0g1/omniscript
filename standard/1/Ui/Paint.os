// standard/1/Ui/Paint.os
// Node tree -> display list. The display list is the seam between Ui and its backends: Cmd{rect, text, border,
// clip push/pop, image, list marker}. Backends (Backend/Gui.os, Backend/Tui.os) execute it; nothing here touches Pen2D.
// Paint order per node: background, border, marker/native control, children (non-positioned first, then positioned by
// z-index), scrollbar. overflow != visible pushes a clip on the padding box and offsets children by the scroll position.
// Scrollbar: thumb is sbBarW() wide at the right edge of the padding box; Events.update lets you drag it or click the track.
// border-radius: CM_FILL / CM_BORDER carry Cmd.radius, Gui draws them with Pen2D.fillRoundRect / strokeRoundRect.
// NEXT: per-side border colours, fixed elements ignoring ancestor scroll.

struct DisplayList {
    cmds: Cmd* = nullptr; n: int = 0; cap: int = 0;

    clear() => void { this.n = 0; }

    add(op: int) => Cmd* {
        if (this.n >= this.cap) {
            let ncap: int = this.cap * 2;
            if (ncap < 256) ncap = 256;
            let nc: Cmd* = CMem.malloc((ncap as u64) * sizeof(Cmd)) as Cmd*;
            let i: int = 0;
            while (i < this.n) { nc[i] = this.cmds[i]; i = i + 1; }
            if (this.cmds != nullptr) CMem.free(this.cmds as void*);
            this.cmds = nc; this.cap = ncap;
        }
        let c: Cmd* = &this.cmds[this.n];
        this.n = this.n + 1;
        c.op = op; c.x = 0.0; c.y = 0.0; c.w = 0.0; c.h = 0.0; c.color = 0;
        c.text = nullptr; c.tfrom = 0; c.tlen = 0; c.size = 16.0; c.flags = 0;
        c.bt = 0.0; c.br = 0.0; c.bb = 0.0; c.bl = 0.0; c.radius = 0.0;
        return c;
    }

    free() => void {
        if (this.cmds != nullptr) CMem.free(this.cmds as void*);
        this.cmds = nullptr; this.n = 0; this.cap = 0;
    }
}

// list marker text for CM_MARK (shared by the backends). Returns its length.
function markerText(buf: char*, style: int, idx: int) => int {
    let n: int = 0;
    if (style == LS_DECIMAL) {
        let digits: int = 1; let t: int = idx;
        while (t >= 10) { t = t / 10; digits = digits + 1; }
        let k: int = digits - 1; let v: int = idx;
        while (k >= 0) { buf[k] = (48 + v % 10) as char; v = v / 10; k = k - 1; }
        n = digits;
        buf[n] = 46 as char; n = n + 1;
    } else {
        let c: int = 45;                                   // disc '-'
        if (style == LS_CIRCLE) c = 111;
        else if (style == LS_SQUARE) c = 43;
        buf[0] = c as char; n = 1;
    }
    buf[n] = 0 as char;
    return n;
}

// ---- scrollbar geometry: Painter draws it and Events hit-tests it with the same numbers ----
function sbBarW() => float { return 10.0; }            // GUI scrollbar width in px (wide enough to grab)
function sbMinThumb() => float { return 16.0; }        // shortest thumb in px
function sbThumbLen(ch: float, sm: float, minLen: float) => float {
    let t: float = maxF(ch * ch / (ch + sm), minLen);
    if (t > ch) t = ch;
    return t;
}

// rightmost edge (absolute, unscrolled) of the in-flow descendants of n; nested scrollers/clips only count their own box
function contentRight(n: Node*) => float {
    let r: float = 0.0;
    let c: Node* = n.first;
    while (c != nullptr) {
        if (c.st.display != DP_NONE && c.kind != NK_HIDDEN) {
            if (c.kind == NK_TEXT) {
                let f: Frag* = c.frags;
                while (f != nullptr) { r = maxF(r, f.x + f.w); f = f.next; }
            } else if (c.st.position == PS_STATIC) {
                r = maxF(r, c.box.x + c.box.w);
                if (c.st.overflow == OV_VISIBLE) r = maxF(r, contentRight(c));
            }
        }
        c = c.next;
    }
    return r;
}

struct Painter {
    dl: DisplayList* = nullptr; m: Metrics* = nullptr; tui: bool = false;
    pop: Popup* = nullptr;             // the open <select> list (Events.pop); painted last, above everything
    sel: Selection* = nullptr;         // page-text selection (Events.sel)
    ctx: CtxMenu* = nullptr;           // right-click menu (Events.ctx)

    fill(x: float, y: float, w: float, h: float, color: u32) => void {
        if ((color & 0xFF) == 0 || w <= 0.0 || h <= 0.0) return;
        let c: Cmd* = this.dl.add(CM_FILL);
        c.x = x; c.y = y; c.w = w; c.h = h; c.color = color;
    }

    // rounded fill (CM_FILL with radius); radius <= 0 is the plain rectangle
    fillR(x: float, y: float, w: float, h: float, color: u32, radius: float) => void {
        if ((color & 0xFF) == 0 || w <= 0.0 || h <= 0.0) return;
        let c: Cmd* = this.dl.add(CM_FILL);
        c.x = x; c.y = y; c.w = w; c.h = h; c.color = color; c.radius = radius;
    }

    text(x: float, y: float, s: char*, from: int, len: int, size: float, color: u32, flags: int) => void {
        let c: Cmd* = this.dl.add(CM_TEXT);
        c.x = x; c.y = y; c.text = s; c.tfrom = from; c.tlen = len; c.size = size; c.color = color; c.flags = flags;
        c.h = size * this.m.lineH / this.m.cellH;
    }

    skipChild(p: Node*, c: Node*) => bool {
        if (c.st.display == DP_NONE || c.kind == NK_HIDDEN) return true;
        if (p.row.spec == SP_DETAILS && !p.open && c.row.spec != SP_SUMMARY) return true;
        return false;
    }

    paint(root: Node*) => void {
        this.dl.clear();
        if (this.sel != nullptr) this.sel.walkStart();                    // selection is resolved in paint order (= document order)
        this.paintNode(root, 0.0, 0.0, 1.0);
        this.paintPopup();
        this.paintCtx();
    }

    // right-click menu, above the page and any select list
    paintCtx() => void {
        let c: CtxMenu* = this.ctx;
        if (c == nullptr || this.tui) return;
        if (!c.isOpen()) return;
        this.fill(c.x, c.y, c.w, c.h, 0x6A6A7AFF);
        this.fill(c.x + 1.0, c.y + 1.0, c.w - 2.0, c.h - 2.0, 0x2A2A33FF);
        let i: int = 0;
        while (i < c.count) {
            let ry: float = c.y + 1.0 + (i as float) * c.rowH;
            let on: bool = true;
            if (i == 0) on = this.sel != nullptr && this.sel.active();       // Copy needs a selection
            if (i == c.hover && on) this.fill(c.x + 1.0, ry, c.w - 2.0, c.rowH, DEF_ACCENT);
            let col: u32 = DEF_TEXT;
            if (!on) col = DEF_DIM;
            let lbl: char* = ctxLabel(i);
            this.text(c.x + 10.0, ry + (c.rowH - this.m.lineH) * 0.5, lbl, 0, strLen(lbl), this.m.cellH, col, 0);
            i = i + 1;
        }
    }

    // <select> dropdown: after the whole tree and every POPCLIP, so nothing clips or covers it. Labels are slices of the option
    // text (the display list keeps pointers until the backend has run). Pen2D widgets under it are hidden by ctlHideUnder.
    paintPopup() => void {
        let p: Popup* = this.pop;
        if (p == nullptr || this.tui) return;
        if (!p.isOpen()) return;
        let o: Node* = p.owner;
        let size: float = o.st.fontPx;
        let th: float = size * this.m.lineH / this.m.cellH;
        this.fill(p.x, p.y, p.w, p.h, 0x6A6A7AFF);                       // 1px border
        this.fill(p.x + 1.0, p.y + 1.0, p.w - 2.0, p.h - 2.0, 0x2A2A33FF);
        let pc: Cmd* = this.dl.add(CM_PUSHCLIP);
        pc.x = p.x + 1.0; pc.y = p.y + 1.0; pc.w = p.w - 2.0; pc.h = p.h - 2.0;
        let thumbW: float = 0.0;
        if (p.hasThumb()) thumbW = sbBarW();
        let i: int = p.top;
        while (i < p.top + p.rows && i < p.count) {
            let ry: float = p.rowY(i);
            let dis: bool = p.disabledAt(i);
            if (i == p.hover && !dis) this.fill(p.x + 1.0, ry, p.w - 2.0 - thumbW, p.rowH, DEF_ACCENT);
            let col: u32 = DEF_TEXT;
            if (dis) col = DEF_DIM;
            let flags: int = 0;
            if (i == o.selIndex) flags = TF_BOLD;
            this.text(p.x + 7.0, ry + (p.rowH - th) * 0.5, selLabelBase(o, i), selSpan(o, i, 0), selSpan(o, i, 1), size, col, flags);
            i = i + 1;
        }
        if (p.hasThumb()) this.fill(p.x + p.w - 1.0 - thumbW, p.y + p.thumbPos(), thumbW, p.thumbLen(), 0x88889AFF);
        this.dl.add(CM_POPCLIP);
    }

    paintText(n: Node*, ox: float, oy: float, op: float) => void {
        let st: Style* = &n.st;
        if (st.hidden || n.ctext == nullptr) return;
        let flags: int = 0;
        if (st.bold) flags = flags | TF_BOLD;
        if (st.italic) flags = flags | TF_ITALIC;
        if (st.underline) flags = flags | TF_UNDERLINE;
        if (st.strike) flags = flags | TF_STRIKE;
        let pbg: u32 = 0;
        let p: Node* = n.parent;
        if (p != nullptr && p.st.display == DP_INLINE && (p.st.bg & 0xFF) != 0) pbg = p.st.bg;
        let f: Frag* = n.frags;
        while (f != nullptr) {
            if (pbg != 0) this.fill(f.x + ox, f.y + oy, f.w, f.h, fadeColor(pbg, op));
            if (this.sel != nullptr && f.len > 0 && this.sel.frag(n, f)) {
                let cw: float = f.w / (f.len as float);
                this.fill(f.x + ox + ((this.sel.lo - f.from) as float) * cw, f.y + oy, ((this.sel.hi - this.sel.lo) as float) * cw, f.h, SEL_COLOR);
            }
            this.text(f.x + ox, f.y + oy, n.ctext, f.from, f.len, st.fontPx, fadeColor(st.color, op), flags);
            f = f.next;
        }
    }

    // controls without a widget: progress/meter always, everything in a terminal
    paintControl(n: Node*, x: float, y: float, op: float) => void {
        let b: Box* = &n.box; let st: Style* = &n.st;
        if (n.ctl == CK_PROGRESS || n.ctl == CK_METER) {
            let span: float = n.max - n.min;
            let frac: float = 0.0;
            if (span > 0.0) frac = clampF((n.value - n.min) / span, 0.0, 1.0);
            this.fill(x, y, b.w, b.h, fadeColor(0x33333FFF, op));
            let col: u32 = DEF_ACCENT;
            if (n.ctl == CK_METER) col = 0x4CAF50FF;
            this.fill(x, y, b.w * frac, b.h, fadeColor(col, op));
        }
        else if (this.tui) {
            this.fill(x, y, b.w, b.h, fadeColor(0x333340FF, op));
            let s: char* = ctlCaption(n);
            if (n.ctl == CK_CHECK) { s = "[ ]"; if (n.checked) s = "[x]"; }
            else if (n.ctl == CK_RADIO) { s = "( )"; if (n.checked) s = "(o)"; }
            else if (n.ctl == CK_RANGE) { s = "<--o-->"; }
            else if (n.ctl == CK_TEXT || n.ctl == CK_TEXTAREA || n.ctl == CK_STUB) { s = n.text; if (s == nullptr) s = ""; }
            this.text(x, y, s, 0, strLen(s), st.fontPx, fadeColor(st.color, op), 0);
        }
    }

    // CSS background-image: the loaded image (n.bgImgId, set by the host loader) sized by background-size and placed by
    // background-position inside the padding box, clipped to it. No tiling: it is drawn once.
    paintBgImage(n: Node*, x: float, y: float, op: float) => void {
        let b: Box* = &n.box; let st: Style* = &n.st;
        let px: float = x + b.bl; let py: float = y + b.bt;
        let pw: float = b.w - b.bl - b.br; let ph: float = b.h - b.bt - b.bb;
        if (pw <= 0.0 || ph <= 0.0) return;
        let iw: float = n.bgImgW * this.m.unit; let ih: float = n.bgImgH * this.m.unit;
        let dw: float = iw; let dh: float = ih;
        if (st.bgSize == 1 || st.bgSize == 2) {
            let sx: float = pw / n.bgImgW; let sy: float = ph / n.bgImgH;
            let sc: float = maxF(sx, sy);
            if (st.bgSize == 2) sc = minF(sx, sy);
            dw = n.bgImgW * sc; dh = n.bgImgH * sc;
        }
        let dx: float = px + (pw - dw) * st.bgFx; let dy: float = py + (ph - dh) * st.bgFy;
        let clip: bool = dx < px || dy < py || dx + dw > px + pw || dy + dh > py + ph;
        if (clip) { let pc: Cmd* = this.dl.add(CM_PUSHCLIP); pc.x = px; pc.y = py; pc.w = pw; pc.h = ph; }
        let im: Cmd* = this.dl.add(CM_IMAGE);
        im.x = dx; im.y = dy; im.w = dw; im.h = dh; im.tfrom = n.bgImgId; im.color = fadeColor(0xFFFFFFFF, op);
        if (clip) this.dl.add(CM_POPCLIP);
    }

    paintNode(n: Node*, ox: float, oy: float, op0: float) => void {
        if (n.st.display == DP_NONE || n.kind == NK_HIDDEN) return;
        let st: Style* = &n.st;
        let op: float = op0 * st.opacity;
        if (n.kind == NK_TEXT) { this.paintText(n, ox, oy, op); return; }
        let b: Box* = &n.box;
        let x: float = b.x + ox; let y: float = b.y + oy;

        if (!st.hidden) {
            let rad: float = st.radius * this.m.unit;
            // outer box-shadow with no blur = a hard copy of the box behind it (offset + spread)
            if (st.shOn && !st.shInset && st.shBlur <= 0.0 && st.shSpread >= 0.0) {
                let srad: float = rad; if (srad > 0.0) srad = srad + st.shSpread;
                this.fillR(x + st.shX - st.shSpread, y + st.shY - st.shSpread, b.w + 2.0 * st.shSpread, b.h + 2.0 * st.shSpread, fadeColor(st.shColor, op), srad);
            }
            this.fillR(x, y, b.w, b.h, fadeColor(st.bg, op), rad);
            if (st.bgUrl != nullptr && n.bgImgId > 0 && n.bgImgW > 0.0 && n.bgImgH > 0.0) this.paintBgImage(n, x, y, op);
            if (st.bt > 0.0 || st.br > 0.0 || st.bb > 0.0 || st.bl > 0.0) {
                let c: Cmd* = this.dl.add(CM_BORDER);
                c.x = x; c.y = y; c.w = b.w; c.h = b.h; c.color = fadeColor(st.borderColor, op);
                c.bt = st.bt; c.br = st.br; c.bb = st.bb; c.bl = st.bl; c.radius = rad;
            }
            // inset box-shadow with no offset and no blur = a ring of `spread` width just inside the border
            if (st.shOn && st.shInset && st.shBlur <= 0.0 && st.shX == 0.0 && st.shY == 0.0 && st.shSpread > 0.0) {
                let rg: Cmd* = this.dl.add(CM_BORDER);
                rg.x = x + b.bl; rg.y = y + b.bt; rg.w = b.w - b.bl - b.br; rg.h = b.h - b.bt - b.bb;
                rg.color = fadeColor(st.shColor, op);
                rg.bt = st.shSpread; rg.br = st.shSpread; rg.bb = st.shSpread; rg.bl = st.shSpread;
                rg.radius = maxF(rad - maxF(b.bt, b.bl), 0.0);
            }
            if (n.row.spec == SP_LI && st.listStyle != LS_NONE) {
                let idx: int = 1; let ps: Node* = n.prevEl();
                while (ps != nullptr) { if (ps.row.spec == SP_LI) idx = idx + 1; ps = ps.prevEl(); }
                let style: int = st.listStyle;
                if (n.parent != nullptr && n.parent.row.spec == SP_OL && style == LS_DISC) style = LS_DECIMAL;
                let mk: Cmd* = this.dl.add(CM_MARK);
                mk.x = x; mk.y = y + b.bt + b.pt; mk.size = st.fontPx; mk.color = fadeColor(st.color, op);
                mk.tfrom = style; mk.tlen = idx;
            }
            if (n.row.spec == SP_SUMMARY) {
                let s: char* = ">";
                if (n.parent != nullptr && n.parent.open) s = "v";
                this.text(x + b.bl + this.m.cellW * 0.5, y + b.bt + b.pt, s, 0, 1, st.fontPx, fadeColor(st.color, op), 0);
            }
            if (n.kind == NK_CONTROL) {
                this.paintControl(n, x, y, op);
            }
            else if (n.kind == NK_REPLACED) {
                if (n.imgId > 0) {
                    let im: Cmd* = this.dl.add(CM_IMAGE);
                    im.x = x + b.bl + b.pl; im.y = y + b.bt + b.pt; im.w = b.cw; im.h = b.ch; im.tfrom = n.imgId;
                    im.color = fadeColor(0xFFFFFFFF, op);
                } else {
                    this.fill(x, y, b.w, b.h, fadeColor(0x2A2A33FF, op));        // placeholder: the tag name in a dim box
                    let lbl: char* = n.getAttr("alt");                           // alt text if present, else the tag name
                    if (lbl == nullptr || strLen(lbl) == 0) lbl = n.tag;
                    if (lbl != nullptr) this.text(x + b.bl + b.pl, y + b.bt + b.pt, lbl, 0, strLen(lbl), st.fontPx, fadeColor(DEF_DIM, op), 0);
                }
            }
        }

        let clip: bool = st.overflow != OV_VISIBLE;
        let cox: float = ox; let coy: float = oy;
        if (clip && (st.overflow == OV_AUTO || st.overflow == OV_SCROLL)) {
            // measure the content width, and pull the scroll position back in range (window grew, content shrank)
            n.contentW = contentRight(n) - b.cx;
            let mx0: float = n.scrollMaxX(); let my0: float = n.scrollMax();
            if (n.scrollX > mx0) n.scrollX = mx0;
            if (n.scrollX < 0.0) n.scrollX = 0.0;
            if (n.scrollY > my0) n.scrollY = my0;
            if (n.scrollY < 0.0) n.scrollY = 0.0;
        }
        if (clip) {
            let pc: Cmd* = this.dl.add(CM_PUSHCLIP);
            pc.x = x + b.bl; pc.y = y + b.bt; pc.w = b.w - b.bl - b.br; pc.h = b.h - b.bt - b.bb;
            cox = ox - n.scrollX; coy = oy - n.scrollY;
        }
        this.paintChildren(n, cox, coy, op);
        if (clip) {
            let sm: float = n.scrollMax();
            if (st.overflow != OV_HIDDEN && sm > 0.0 && b.ch > 0.0) {
                let bar: float = sbBarW(); let mn: float = sbMinThumb();
                if (this.tui) { bar = this.m.cellW * 0.5; mn = this.m.cellH; }        // terminal: one thin cell column
                let thumbH: float = sbThumbLen(b.ch, sm, mn);
                let thumbY: float = y + b.bt + (b.ch - thumbH) * (n.scrollY / sm);
                this.fill(x + b.w - b.br - bar, thumbY, bar, thumbH, fadeColor(0x88889AFF, op));
            }
            let smx: float = n.scrollMaxX();
            if (st.overflow != OV_HIDDEN && smx > 0.0 && b.cw > 0.0) {
                let hbar: float = sbBarW(); let hmn: float = sbMinThumb();
                if (this.tui) { hbar = this.m.cellH; hmn = this.m.cellW; }
                let track: float = b.cw;
                if (sm > 0.0 && b.ch > 0.0) track = track - sbBarW();       // leave the corner to the vertical bar
                let thumbW: float = sbThumbLen(track, smx, hmn);
                let thumbX: float = x + b.bl + (track - thumbW) * (n.scrollX / smx);
                this.fill(thumbX, y + b.h - b.bb - hbar, thumbW, hbar, fadeColor(0x88889AFF, op));
            }
            this.dl.add(CM_POPCLIP);
        }
    }

    paintChildren(n: Node*, ox: float, oy: float, op: float) => void {
        let c: Node* = n.first;
        while (c != nullptr) {
            if (!this.skipChild(n, c) && (c.kind == NK_TEXT || c.st.position == PS_STATIC)) this.paintNode(c, ox, oy, op);
            c = c.next;
        }
        let lastZ: int = 0; let started: bool = false; let more: bool = true;
        while (more) {
            let found: bool = false; let best: int = 0;
            c = n.first;
            while (c != nullptr) {
                if (c.kind != NK_TEXT && c.st.position != PS_STATIC && !this.skipChild(n, c)) {
                    let z: int = c.st.zIndex;
                    if ((!started || z > lastZ) && (!found || z < best)) { best = z; found = true; }
                }
                c = c.next;
            }
            if (!found) { more = false; }
            else {
                c = n.first;
                while (c != nullptr) {
                    if (c.kind != NK_TEXT && c.st.position != PS_STATIC && !this.skipChild(n, c) && c.st.zIndex == best) this.paintNode(c, ox, oy, op);
                    c = c.next;
                }
                lastZ = best; started = true;
            }
        }
    }
}
