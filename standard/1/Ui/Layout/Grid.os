    // standard/1/Ui/Layout/Grid.os   (fragment: pasted INSIDE `struct Layout { ... }` by Block.os)
    // Stopgap grid: grid-template-columns tracks (px, %, fr, auto = 1fr) and gap; items fill cells left to right, row by row; grid-column start / span is honoured (sequential placement, no `dense`).
    // A row is as tall as its tallest item; align-items works as in flex (stretch by default). Items with an explicit width keep it.
    // Tracks live on the Style (gn, g0..g11). Without grid-template-columns there is one full-width column.
    // NEXT: grid-template-rows, grid-row spans, auto-fill / auto-fit, named areas, row-gap vs column-gap, justify-items.

    // grid-column of an item: gridLine = 1-based line index (negative counts from the end, -1 = last line),
    // gridSpan = number of columns (1..cols), gridStart = 0-based start column or -1 when auto-placed.
    gridLine(v: int, cols: int) => int {
        if (v < 0) return cols + 2 + v;
        return v;
    }
    gridSpan(e: Node*, cols: int) => int {
        let s: Style* = &e.st;
        let sp: int = 1;
        if (s.gsp > 0) sp = s.gsp;
        else if (s.gcs != 0 && s.gce != 0) { sp = this.gridLine(s.gce, cols) - this.gridLine(s.gcs, cols); if (sp < 1) sp = 1; }
        if (sp > cols) sp = cols;
        return sp;
    }
    gridStart(e: Node*, cols: int) => int {
        let s: Style* = &e.st;
        if (s.gcs == 0) return 0 - 1;
        let c: int = this.gridLine(s.gcs, cols) - 1;
        if (c < 0) c = 0;
        if (c > cols - 1) c = cols - 1;
        return c;
    }

    // max-content width of every column into out[0..cols): the widest single-column item placed in it (fixed px tracks keep
    // their size; items spanning several columns are ignored). Placement = same rules as gridContent.
    // Returns the grid's preferred width: the columns plus the gaps.
    gridColPrefs(n: Node*, out: float*) => float {
        let st: Style* = &n.st;
        let cols: int = st.gn;
        if (cols < 1) cols = 1;
        let i: int = 0;
        while (i < cols) { out[i] = 0.0; i = i + 1; }
        let c: Node* = this.itemFrom(n.first); let cc: int = 0;
        while (c != nullptr) {
            let sp: int = this.gridSpan(c, cols);
            let s0: int = this.gridStart(c, cols);
            let col: int = cc;
            if (s0 >= 0) { col = s0; if (col + sp > cols) sp = cols - col; }
            else if (cc + sp > cols) col = 0;
            if (sp == 1) { let w: float = this.outerPref(c); if (w > out[col]) out[col] = w; }
            cc = col + sp;
            if (cc >= cols) cc = 0;
            c = this.itemFrom(c.next);
        }
        let total: float = 0.0; let t: Len;
        i = 0;
        while (i < cols) {
            if (st.gn >= 1) { gridTrack(st, i, &t); if (t.u == LU_PX) out[i] = t.v; }
            total = total + out[i]; i = i + 1;
        }
        return total + st.gap * ((cols - 1) as float);
    }

    gridPref(n: Node*) => float {
        let pr: float* = CMem.malloc(96) as float*;
        let total: float = this.gridColPrefs(n, pr);
        CMem.free(pr as void*);
        return total;
    }

    // column widths of the grid into tw[0..cols) for a content width of cw.
    // Fixed tracks keep their size, the rest is shared by weight (auto counts as 1fr). When cw equals the content width
    // (a shrink-to-fit grid, e.g. a flex item), `auto` tracks take their max-content width instead of an equal share.
    gridTracks(n: Node*, cw: float, tw: float*) => void {
        let st: Style* = &n.st;
        let gap: float = st.gap;
        let cols: int = st.gn;
        if (cols < 1) { tw[0] = cw; return; }
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
        j = 0;
        while (j < cols) {
            gridTrack(st, j, &t);
            let w: float = 0.0;
            if (t.u == LU_PX) w = t.v;
            else if (t.u == LU_PCT) w = t.v * cw / 100.0;
            else if (frSum > 0.0) w = free * t.v / frSum;
            tw[j] = w;
            j = j + 1;
        }
        let pr: float* = CMem.malloc(96) as float*;
        let total: float = this.gridColPrefs(n, pr);
        if (total > 0.0 && cw >= total - 1.0 && cw <= total + 1.0) {      // only when cw IS the content width (shrink-to-fit); a narrower box wraps its text instead
            j = 0;
            while (j < cols) {
                gridTrack(st, j, &t);
                if (t.u != LU_PX && t.u != LU_PCT) tw[j] = pr[j];
                j = j + 1;
            }
        }
        CMem.free(pr as void*);
    }

    gridContent(n: Node*) => float {
        let st: Style* = &n.st; let b: Box* = &n.box;
        let gap: float = st.gap;
        let cols: int = st.gn;
        if (cols < 1) cols = 1;
        let y: float = 0.0;
        let tw: float* = CMem.malloc(96) as float*;
        let c0: float* = CMem.malloc(96) as float*;       // start column of the k-th item of the current row
        let spn: float* = CMem.malloc(96) as float*;      // its column span
        this.gridTracks(n, b.cw, tw);
        let rowStart: Node* = this.itemFrom(n.first);
        while (rowStart != nullptr) {
            // 1. pick the items of one row (grid-column start / span, else next free cells), lay each out at the origin, find the row height
            let rowH: float = 0.0; let k: int = 0; let cc: int = 0; let full: bool = false;
            let e: Node* = rowStart;
            while (e != nullptr && !full) {
                let sp: int = this.gridSpan(e, cols);
                let s0: int = this.gridStart(e, cols);
                let col: int = cc;
                if (s0 >= 0) { col = s0; if (col + sp > cols) sp = cols - col; }
                let brk: bool = false;
                if (k > 0 && s0 >= 0 && col < cc) brk = true;          // explicit column already passed: next row
                if (k > 0 && s0 < 0 && cc + sp > cols) brk = true;     // does not fit in the rest of the row
                if (brk) { full = true; }
                else {
                    let w: float = gap * ((sp - 1) as float);
                    let q: int = col;
                    while (q < col + sp) { w = w + tw[q]; q = q + 1; }
                    this.resolveBox(e, b.cw);
                    if (e.st.width.u == LU_AUTO) {
                        let inner: float = w - e.box.ml - e.box.mr;
                        if (inner < 0.0) inner = 0.0;
                        e.forceW = inner;
                    }
                    this.placeBlock(e, 0.0, 0.0, b.cw, false);
                    e.forceW = UNSET;
                    rowH = maxF(rowH, e.box.h + e.box.mt + e.box.mb);
                    c0[k] = col as float; spn[k] = sp as float;
                    cc = col + sp; k = k + 1;
                    e = this.itemFrom(e.next);
                    if (cc >= cols) full = true;
                }
            }
            // 2. align and move them into their cells
            let it: Node* = rowStart; let m: int = 0;
            while (m < k) {
                let al: int = this.alignOf(it, st);
                let outerH: float = it.box.h + it.box.mt + it.box.mb;
                let cpos: float = 0.0;
                if (al == AL_END) cpos = rowH - outerH;
                else if (al == AL_CENTER) cpos = (rowH - outerH) * 0.5;
                else if (al == AL_STRETCH) { this.stretchCross(it, true, rowH, b.cw); }
                let x: float = 0.0; let q2: int = 0; let cm: int = c0[m] as int;
                while (q2 < cm) { x = x + tw[q2] + gap; q2 = q2 + 1; }
                this.translate(it, b.cx + x - (it.box.x - it.box.ml), b.cy + y + cpos - (it.box.y - it.box.mt));
                it = this.itemFrom(it.next); m = m + 1;
            }
            y = y + rowH;
            if (e != nullptr) y = y + gap;
            rowStart = e;
        }
        CMem.free(tw as void*); CMem.free(c0 as void*); CMem.free(spn as void*);
        return y;
    }
