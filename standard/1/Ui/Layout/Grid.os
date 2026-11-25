    // standard/1/Ui/Layout/Grid.os   (fragment: pasted INSIDE `struct Layout { ... }` by Block.os)
    // Stopgap grid: grid-template-columns tracks (px, %, fr, auto = 1fr) and gap; items fill cells left to right, row by row.
    // A row is as tall as its tallest item; align-items works as in flex (stretch by default). Items with an explicit width keep it.
    // Tracks live on the Style (gn, g0..g11). Without grid-template-columns there is one full-width column.
    // NEXT: grid-template-rows, grid-column / grid-row spans, auto-fill / auto-fit, named areas, row-gap vs column-gap, justify-items.

    gridColW(st: Style*, i: int, cw: float, gap: float) => float {
        let cols: int = st.gn;
        if (cols < 1) return cw;
        let fixed: float = 0.0; let frSum: float = 0.0;
        let j: int = 0; let t: Len;
        while (j < cols) {
            gridTrack(st, j, &t);
            if (t.u == LU_PX) fixed = fixed + t.v;
            else if (t.u == LU_PCT) fixed = fixed + t.v * cw / 100.0;
            else frSum = frSum + t.v;
            j = j + 1;
        }
        let free: float = cw - gap * ((cols - 1) as float) - fixed;
        if (free < 0.0) free = 0.0;
        gridTrack(st, i, &t);
        let w: float = 0.0;
        if (t.u == LU_PX) w = t.v;
        else if (t.u == LU_PCT) w = t.v * cw / 100.0;
        else if (frSum > 0.0) w = free * t.v / frSum;
        return w;
    }

    gridContent(n: Node*) => float {
        let st: Style* = &n.st; let b: Box* = &n.box;
        let gap: float = st.gap;
        let cols: int = st.gn;
        if (cols < 1) cols = 1;
        let y: float = 0.0;
        let rowStart: Node* = this.itemFrom(n.first);
        while (rowStart != nullptr) {
            // 1. lay out one row of up to `cols` items at the origin, find the row height
            let rowH: float = 0.0; let k: int = 0;
            let e: Node* = rowStart;
            while (k < cols && e != nullptr) {
                let w: float = this.gridColW(st, k, b.cw, gap);
                this.resolveBox(e, b.cw);
                if (e.st.width.u == LU_AUTO) {
                    let inner: float = w - e.box.ml - e.box.mr;
                    if (inner < 0.0) inner = 0.0;
                    e.forceW = inner;
                }
                this.placeBlock(e, 0.0, 0.0, b.cw, false);
                e.forceW = UNSET;
                rowH = maxF(rowH, e.box.h + e.box.mt + e.box.mb);
                e = this.itemFrom(e.next); k = k + 1;
            }
            // 2. align and move them into their cells
            let it: Node* = rowStart; let m: int = 0; let x: float = 0.0;
            while (m < k) {
                let al: int = this.alignOf(it, st);
                let outerH: float = it.box.h + it.box.mt + it.box.mb;
                let cpos: float = 0.0;
                if (al == AL_END) cpos = rowH - outerH;
                else if (al == AL_CENTER) cpos = (rowH - outerH) * 0.5;
                else if (al == AL_STRETCH) { this.stretchCross(it, true, rowH, b.cw); }
                this.translate(it, b.cx + x - (it.box.x - it.box.ml), b.cy + y + cpos - (it.box.y - it.box.mt));
                x = x + this.gridColW(st, m, b.cw, gap) + gap;
                it = this.itemFrom(it.next); m = m + 1;
            }
            y = y + rowH;
            if (e != nullptr) y = y + gap;
            rowStart = e;
        }
        return y;
    }
