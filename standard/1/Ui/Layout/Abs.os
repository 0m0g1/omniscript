    // standard/1/Ui/Layout/Abs.os   (fragment: pasted INSIDE `struct Layout { ... }` by Block.os)
    // position: absolute / fixed. Called at the end of placeBlock for every positioned node (and the root):
    // layoutAbsKids(cb, n) walks n's static descendants and places each absolute child against its containing
    // block `cb` (nearest positioned ancestor); fixed children use the viewport. Relative offsets are in placeBlock.
    // left/right/top/bottom both set + width/height auto = stretch. With no offsets the box sits at the top-left of
    // the containing block (NEXT: true static position). An open <dialog> is centred in its containing block.

    layoutAbsKids(cb: Node*, n: Node*) => void {
        let c: Node* = n.first;
        while (c != nullptr) {
            if (c.st.display != DP_NONE && c.kind != NK_TEXT) {
                if (c.st.position == PS_ABSOLUTE) { this.placeAbs(c, cb, false); }
                else if (c.st.position == PS_FIXED) { this.placeAbs(c, cb, true); }
                else if (c.st.position == PS_STATIC) { this.layoutAbsKids(cb, c); }   // relative children handle their own
            }
            c = c.next;
        }
    }

    placeAbs(c: Node*, cb: Node*, fixed: bool) => void {
        let cx: float = 0.0; let cy: float = 0.0; let cw: float = this.vw; let ch: float = this.vh;
        if (!fixed) {
            cx = cb.box.x + cb.box.bl; cy = cb.box.y + cb.box.bt;
            cw = cb.box.w - cb.box.bl - cb.box.br; ch = cb.box.h - cb.box.bt - cb.box.bb;
        }
        let st: Style* = &c.st;
        this.resolveBox(c, cw);
        let b: Box* = &c.box;
        let hasL: bool = st.left.u != LU_AUTO; let hasR: bool = st.right.u != LU_AUTO;
        let hasT: bool = st.top.u != LU_AUTO;  let hasB: bool = st.bottom.u != LU_AUTO;
        let l: float = this.resolve(&st.left, cw, 0.0); let r: float = this.resolve(&st.right, cw, 0.0);
        let t: float = this.resolve(&st.top, ch, 0.0);  let bo: float = this.resolve(&st.bottom, ch, 0.0);
        let shrink: bool = st.width.u == LU_AUTO;
        if (hasL && hasR && st.width.u == LU_AUTO) { c.forceW = maxF(cw - l - r - b.ml - b.mr, 0.0); shrink = false; }
        if (hasT && hasB && st.height.u == LU_AUTO) { c.forceH = maxF(ch - t - bo - b.mt - b.mb, 0.0); }
        this.placeBlock(c, 0.0, 0.0, cw, shrink);
        c.forceW = UNSET; c.forceH = UNSET;
        let mw: float = b.w + b.ml + b.mr; let mh: float = b.h + b.mt + b.mb;
        let fx: float = cx; let fy: float = cy;
        if (hasL) fx = cx + l; else if (hasR) fx = cx + cw - r - mw;
        if (hasT) fy = cy + t; else if (hasB) fy = cy + ch - bo - mh;
        if (c.row.spec == SP_DIALOG && !hasL && !hasR && !hasT && !hasB) {
            fx = cx + (cw - mw) * 0.5; fy = cy + (ch - mh) * 0.5;
        }
        this.translate(c, fx - (b.x - b.ml), fy - (b.y - b.mt));
    }
