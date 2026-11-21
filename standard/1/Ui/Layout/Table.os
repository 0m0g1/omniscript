    // standard/1/Ui/Layout/Table.os   (fragment: pasted INSIDE `struct Layout { ... }` by Block.os)
    // Auto table layout: rows come from <tr> children and from <thead>/<tbody>/<tfoot> children; column width = widest
    // cell (max-content), shrunk proportionally when the table is wider than its container, stretched proportionally
    // when the table has an explicit width. Cells are laid out at the column width and stretched to the row height.
    // <caption> is laid out as a block above the rows. No border-spacing / border-collapse.
    // NEXT: colspan/rowspan, <col> widths, vertical-align, min-content column minimums, border-collapse.

    tableCountRows(n: Node*) => int {
        let cnt: int = 0;
        let c: Node* = n.first;
        while (c != nullptr) {
            if (this.inFlow(c)) {
                if (c.st.display == DP_TABLE_ROW) cnt = cnt + 1;
                else if (c.st.display == DP_TABLE_GROUP) {
                    c.box.w = 0.0; c.box.h = 0.0;
                    let r: Node* = c.first;
                    while (r != nullptr) { if (this.inFlow(r) && r.st.display == DP_TABLE_ROW) cnt = cnt + 1; r = r.next; }
                }
            }
            c = c.next;
        }
        return cnt;
    }

    tableCollect(n: Node*, rows: Node**) => void {
        let i: int = 0; let c: Node* = n.first;
        while (c != nullptr) {
            if (this.inFlow(c)) {
                if (c.st.display == DP_TABLE_ROW) { rows[i] = c; i = i + 1; }
                else if (c.st.display == DP_TABLE_GROUP) {
                    let r: Node* = c.first;
                    while (r != nullptr) {
                        if (this.inFlow(r) && r.st.display == DP_TABLE_ROW) { rows[i] = r; i = i + 1; }
                        r = r.next;
                    }
                }
            }
            c = c.next;
        }
    }

    isCell(c: Node*) => bool { return this.inFlow(c) && c.st.display == DP_TABLE_CELL; }

    tableCols(rows: Node**, nrows: int) => int {
        let ncols: int = 0; let i: int = 0;
        while (i < nrows) {
            let cnt: int = 0; let c: Node* = rows[i].first;
            while (c != nullptr) { if (this.isCell(c)) cnt = cnt + 1; c = c.next; }
            if (cnt > ncols) ncols = cnt;
            i = i + 1;
        }
        return ncols;
    }

    tableColPref(rows: Node**, nrows: int, colW: float*, ncols: int) => void {
        let j: int = 0;
        while (j < ncols) { colW[j] = 0.0; j = j + 1; }
        let i: int = 0;
        while (i < nrows) {
            let col: int = 0; let c: Node* = rows[i].first;
            while (c != nullptr) {
                if (this.isCell(c)) { colW[col] = maxF(colW[col], this.outerPref(c)); col = col + 1; }
                c = c.next;
            }
            i = i + 1;
        }
    }

    tablePref(n: Node*) => float {
        let nrows: int = this.tableCountRows(n);
        if (nrows == 0) return 0.0;
        let rows: Node** = CMem.malloc((nrows as u64) * 8) as Node**;
        this.tableCollect(n, rows);
        let ncols: int = this.tableCols(rows, nrows);
        let colW: float* = CMem.malloc((ncols as u64) * 8 + 8) as float*;
        this.tableColPref(rows, nrows, colW, ncols);
        let sum: float = 0.0; let j: int = 0;
        while (j < ncols) { sum = sum + colW[j]; j = j + 1; }
        CMem.free(colW as void*); CMem.free(rows as void*);
        return sum;
    }

    tableContent(n: Node*) => float {
        let b: Box* = &n.box;
        let y: float = b.cy;
        // captions
        let cap: Node* = n.first;
        while (cap != nullptr) {
            if (this.inFlow(cap) && cap.tag != nullptr && strEq(cap.tag, "caption")) {
                this.resolveBox(cap, b.cw);
                this.placeBlock(cap, b.cx, y, b.cw, false);
                y = cap.box.y + cap.box.h + cap.box.mb;
            }
            cap = cap.next;
        }
        let nrows: int = this.tableCountRows(n);
        if (nrows == 0) return y - b.cy;
        let rows: Node** = CMem.malloc((nrows as u64) * 8) as Node**;
        this.tableCollect(n, rows);
        let ncols: int = this.tableCols(rows, nrows);
        let colW: float* = CMem.malloc((ncols as u64) * 8 + 8) as float*;
        this.tableColPref(rows, nrows, colW, ncols);

        let sum: float = 0.0; let j: int = 0;
        while (j < ncols) { sum = sum + colW[j]; j = j + 1; }
        let f: float = 1.0;
        if (sum > b.cw && sum > 0.0) f = b.cw / sum;
        else if (n.st.width.u != LU_AUTO && sum < b.cw && sum > 0.0) f = b.cw / sum;
        j = 0; sum = 0.0;
        while (j < ncols) { colW[j] = colW[j] * f; sum = sum + colW[j]; j = j + 1; }

        let i: int = 0;
        while (i < nrows) {
            let r: Node* = rows[i];
            let x: float = b.cx; let rowH: float = 0.0; let col: int = 0;
            let c: Node* = r.first;
            while (c != nullptr) {
                if (this.isCell(c)) {
                    c.forceW = colW[col];
                    this.placeBlock(c, x, y, colW[col], false);
                    c.forceW = UNSET;
                    rowH = maxF(rowH, c.box.h);
                    x = x + colW[col]; col = col + 1;
                }
                c = c.next;
            }
            c = r.first;
            while (c != nullptr) {
                if (this.isCell(c)) {
                    let vp: float = c.box.pt + c.box.pb + c.box.bt + c.box.bb;
                    c.box.h = rowH; c.box.ch = maxF(rowH - vp, 0.0);
                }
                c = c.next;
            }
            r.box.x = b.cx; r.box.y = y; r.box.w = sum; r.box.h = rowH;
            r.box.cx = b.cx; r.box.cy = y; r.box.cw = sum; r.box.ch = rowH;
            let g: Node* = r.parent;
            if (g != n && g.st.display == DP_TABLE_GROUP) {
                if (g.box.w == 0.0) { g.box.x = b.cx; g.box.y = y; }
                g.box.w = sum; g.box.h = (y + rowH) - g.box.y;
                g.box.cx = g.box.x; g.box.cy = g.box.y; g.box.cw = g.box.w; g.box.ch = g.box.h;
            }
            y = y + rowH;
            i = i + 1;
        }
        CMem.free(colW as void*); CMem.free(rows as void*);
        return y - b.cy;
    }
