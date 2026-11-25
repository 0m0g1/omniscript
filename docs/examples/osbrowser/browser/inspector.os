// browser/inspector.os  -- devtools window: DOM tree, Console, Styles (per element), Report (whole page).
// Select by clicking a row, or by clicking anything in the page/toolbar while the inspector is open.
// Reads the Ui tree through firstEl()/nextEl()/getAttr() + uiapi.os only.
//
// Tabs:  DOM      element tree + attributes of the selected node
//        Console  Dbg log
//        Styles   every declaration that hit the selected element, with status and where it came from.
//                 "[x] problems only" (top right) hides everything that was applied.
//        Report   the whole page at once: every UNSUPPORTED / INVALID property, how often it occurred, one example
//                 value + selector + element. Click a row to jump to that element in Styles. "[rescan]" rebuilds it.
//                 Needs style notes: reload the page once while the inspector is open.
//
// Drawing allocates nothing per frame. The Report allocates only when it is rebuilt (rescan / new document).
// NEXT: computed box/style panel (needs Node geometry fields), text nodes, highlight overlay, tree collapse,
//       style editing, network tab, mouse wheel scroll, search.
module C = import "standard/1/Canvas.os";
module UI = import "standard/1/Ui.os";
module U = import "util.os";
module A = import "uiapi.os";
module B = import "browser.os";
module Dbg = import "dbg.os";

let LH: int = 16;                // row height   (ADAPT to pen.textHeight)
let REPMAX: int = 96;            // distinct (property, status) pairs kept in the Report

// string equality on char* (no dependency on what util.os offers)
function inspSame(a: char*, b: char*) => bool {
    let i: int = 0;
    let eq: bool = true;
    let go: bool = true;
    while (go) {
        let x: int = a[i] as int;
        let y: int = b[i] as int;
        if (x != y) { eq = false; go = false; }
        else if (x == 0) { go = false; }
        else { i = i + 1; }
    }
    return eq;
}

struct Inspector {
    win: C.Win* = nullptr;
    pen: C.Pen2D* = nullptr;
    tab: int = 0;                // 0 DOM, 1 Console, 2 Styles, 3 Report
    scroll: int = 0;
    line: int = 0;
    selected: UI.Node* = nullptr;
    rows: UI.Node** = nullptr;
    nrows: int = 0;
    cw: float = 8.0;
    winW: float = 560.0;
    gen: int = 0;

    // Styles tab
    problemsOnly: bool = false;
    sumNode: UI.Node* = nullptr;
    sumBad: int = 0;
    sumStr: char* = nullptr;

    // Report tab (parallel arrays, REPMAX entries, sorted by count descending after a scan)
    repDirty: bool = true;
    nrep: int = 0;
    repDecl: int = 0;            // problem declarations seen
    repAll: int = 0;             // style notes seen (0 = recordStyles was off for this document)
    repName: char** = nullptr;
    repVal: char** = nullptr;
    repSrc: char** = nullptr;
    repLbl: char** = nullptr;
    repNode: UI.Node** = nullptr;
    repCnt: int* = nullptr;
    repSt: int* = nullptr;
    repSum: char* = nullptr;

    isOpen() => bool {
        let open: bool = this.win != nullptr;
        if (open && !this.win.isOpen) { this.win = nullptr; this.pen = nullptr; open = false; }
        return open;
    }

    // ADAPT: assumes Canvas.createWindow can be called while the frame loop is running.
    open(app: C.Canvas*) => void {
        if (!this.isOpen()) {
            this.win = app.createWindow("Inspector", 560, 680);
            if (this.win == nullptr) Dbg.error("inspector: createWindow failed");
            else {
                this.pen = this.win.context2d();
                this.rows = U.alloc(8 * 128) as UI.Node**;
                this.repName = U.alloc(8 * REPMAX) as char**;
                this.repVal = U.alloc(8 * REPMAX) as char**;
                this.repSrc = U.alloc(8 * REPMAX) as char**;
                this.repLbl = U.alloc(8 * REPMAX) as char**;
                this.repNode = U.alloc(8 * REPMAX) as UI.Node**;
                this.repCnt = U.alloc(8 * REPMAX) as int*;
                this.repSt = U.alloc(8 * REPMAX) as int*;
                this.nrep = 0;
                this.repAll = 0;
                this.repDirty = true;
            }
        }
    }

    close() => void { if (this.isOpen()) this.win.close(); }
    select(n: UI.Node*) => void { this.selected = n; }

    row(n: UI.Node*, depth: int, top: int, bottom: int) => void {
        let idx: int = this.line;
        this.line = this.line + 1;
        let k: int = idx - this.scroll;
        let y: int = top + k * LH;
        if (k >= 0 && k < 128 && y + LH <= bottom) {
            this.rows[k] = n;
            this.nrows = k + 1;
            let x: float = 8.0 + (depth * 12) as float;
            let yf: float = y as float;
            let pen: C.Pen2D* = this.pen;
            let tag: char* = A.tagOf(n);
            let id: char* = A.attr(n, "id");
            let cls: char* = A.attr(n, "class");
            if (n == this.selected) pen.fillRect(0.0, yf, this.winW, LH as float, 0x3A5BC8FF);
            pen.text(x, yf, "<", 0x6A6A7AFF);
            pen.text(x + this.cw, yf, tag, 0x7FB0FFFF);
            let ax: float = x + this.cw * ((U.slen(tag) + 2) as float);
            if (U.slen(id) > 0) {
                pen.text(ax, yf, "#", 0x6A6A7AFF);
                pen.text(ax + this.cw, yf, id, 0xE0B060FF);
                ax = ax + this.cw * ((U.slen(id) + 2) as float);
            }
            if (U.slen(cls) > 0) {
                pen.text(ax, yf, ".", 0x6A6A7AFF);
                pen.text(ax + this.cw, yf, cls, 0x80D090FF);
            }
        }
        let c: UI.Node* = n.firstEl();
        while (c != nullptr) { this.row(c, depth + 1, top, bottom); c = c.nextEl(); }
    }

    // draws "name  value" at line `slot` of the details panel when the attribute is present
    attrLine(n: UI.Node*, name: char*, top: float, slot: int) => int {
        let v: char* = A.attr(n, name);
        let used: int = slot;
        if (U.slen(v) > 0) {
            let y: float = top + 22.0 + (slot * LH) as float;
            this.pen.text(10.0, y, name, 0x8A8A9AFF);
            this.pen.text(90.0, y, v, 0xFFFFFFFF);
            used = slot + 1;
        }
        return used;
    }

    details(top: float, h: float) => void {
        let pen: C.Pen2D* = this.pen;
        pen.fillRect(0.0, top, this.winW, h, 0x23232BFF);
        let n: UI.Node* = this.selected;
        if (n == nullptr) { pen.text(10.0, top + 6.0, "click a row (or an element on the page) to inspect", 0x8A8A9AFF); }
        else {
            pen.text(10.0, top + 4.0, "<", 0x6A6A7AFF);
            pen.text(10.0 + this.cw, top + 4.0, A.tagOf(n), 0x7FB0FFFF);
            let s: int = 0;
            s = this.attrLine(n, "id", top, s);
            s = this.attrLine(n, "class", top, s);
            s = this.attrLine(n, "style", top, s);
            s = this.attrLine(n, "href", top, s);
            s = this.attrLine(n, "src", top, s);
            s = this.attrLine(n, "type", top, s);
            s = this.attrLine(n, "value", top, s);
        }
    }

    consoleView(h: float) => void {
        let n: int = Dbg.length();
        let k: int = 0;
        let i: int = this.scroll;
        while (i < n && (28 + k * LH + LH) < (h as int)) {
            let s: char* = Dbg.get(i);
            let col: u32 = 0xE8E8F0FF;
            if ((s[1] as int) == 101 && (s[0] as int) == 91) col = 0xFF6060FF;      // "[error]"
            this.pen.text(8.0, (28 + k * LH) as float, s, col);
            k = k + 1;
            i = i + 1;
        }
    }

    // Styles tab: every declaration that targets the selected element, in cascade order, with what the Ui did with it.
    // Two lines per declaration:  STATUS  property value   /   where it came from.
    // Needs ui.recordStyles (main.os switches it on while the inspector is open).
    stylesView(h: float) => void {
        let pen: C.Pen2D* = this.pen;
        let n: UI.Node* = this.selected;
        if (n == nullptr) { pen.text(10.0, 32.0, "click an element (on the page or in the DOM tab) to see its CSS", 0x8A8A9AFF); return; }
        let head: float = 28.0;
        let tag: char* = A.tagOf(n);
        pen.text(10.0, head, "<", 0x6A6A7AFF);
        pen.text(10.0 + this.cw, head, tag, 0x7FB0FFFF);

        // id and class on the second header line
        let id: char* = A.attr(n, "id");
        let cls: char* = A.attr(n, "class");
        let ix: float = 10.0;
        let hy: float = head + (LH as float);
        if (U.slen(id) > 0) {
            pen.text(ix, hy, "#", 0x6A6A7AFF);
            pen.text(ix + this.cw, hy, id, 0xE0B060FF);
            ix = ix + this.cw * ((U.slen(id) + 2) as float);
        }
        if (U.slen(cls) > 0) {
            pen.text(ix, hy, ".", 0x6A6A7AFF);
            pen.text(ix + this.cw, hy, cls, 0x80D090FF);
        }

        let first: UI.StyleNote* = A.notesOf(n);
        if (first == nullptr) {
            pen.text(10.0, head + (2 * LH) as float, "no style data for this node (reload the page once the inspector is open)", 0x8A8A9AFF);
            return;
        }
        // count unsupported / invalid
        let bad: int = 0;
        let cnt: UI.StyleNote* = first;
        while (cnt != nullptr) {
            let st0: int = A.noteStatus(cnt);
            if (st0 == A.SN_UNSUPPORTED || st0 == A.SN_INVALID) bad = bad + 1;
            cnt = A.noteNext(cnt);
        }
        // the summary string is rebuilt only when the node or the count changes (no per-frame allocation)
        if (this.sumStr == nullptr || this.sumNode != n || this.sumBad != bad) {
            this.sumNode = n;
            this.sumBad = bad;
            this.sumStr = U.cat(U.fromInt(bad), " unsupported or invalid");
        }
        pen.text(10.0 + this.cw * ((U.slen(tag) + 4) as float), head, this.sumStr, 0xFFA040FF);

        let y0: int = 28 + 2 * LH;
        let px: float = 10.0 + this.cw * 15.0;
        let i: int = 0;
        let k: int = 0;
        let sn: UI.StyleNote* = first;
        while (sn != nullptr) {
            let st: int = A.noteStatus(sn);
            let prob: bool = (st == A.SN_UNSUPPORTED) || (st == A.SN_INVALID);
            if (!this.problemsOnly || prob) {
                if (i >= this.scroll) {
                    let y: int = y0 + k * 2 * LH;
                    if (y + 2 * LH <= (h as int)) {
                        let col: u32 = 0xE8E8F0FF;
                        let label: char* = "applied";
                        if (st == A.SN_UNSUPPORTED) { col = 0xFFA040FF; label = "UNSUPPORTED"; }
                        else if (st == A.SN_INVALID) { col = 0xFF6060FF; label = "INVALID VALUE"; }
                        else if (st == A.SN_OVERRIDDEN) { col = 0x6A6A7AFF; label = "overridden"; }
                        let yf: float = y as float;
                        let prop: char* = A.noteProp(sn);
                        pen.text(10.0, yf, label, col);
                        pen.text(px, yf, prop, col);
                        pen.text(px + this.cw * ((U.slen(prop) + 2) as float), yf, A.noteValue(sn), col);
                        let src: char* = A.noteSel(sn);
                        let so: int = A.noteSource(sn);
                        if (so == 0) src = "browser default";
                        else if (so == 2) src = "style attribute";
                        if (src != nullptr) pen.text(px, yf + (LH as float), src, 0x8A8A9AFF);
                        k = k + 1;
                    }
                }
                i = i + 1;
            }
            sn = A.noteNext(sn);
        }
        if (k == 0 && this.problemsOnly) pen.text(10.0, (y0 as float), "no unsupported or invalid declarations on this element", 0x80D090FF);
    }

    // ---- Report: scan the whole document for declarations the Ui could not apply ----
    swapRep(a: int, b: int) => void {
        let tn: char* = this.repName[a]; this.repName[a] = this.repName[b]; this.repName[b] = tn;
        let tv: char* = this.repVal[a]; this.repVal[a] = this.repVal[b]; this.repVal[b] = tv;
        let ts: char* = this.repSrc[a]; this.repSrc[a] = this.repSrc[b]; this.repSrc[b] = ts;
        let tp: UI.Node* = this.repNode[a]; this.repNode[a] = this.repNode[b]; this.repNode[b] = tp;
        let tc: int = this.repCnt[a]; this.repCnt[a] = this.repCnt[b]; this.repCnt[b] = tc;
        let tt: int = this.repSt[a]; this.repSt[a] = this.repSt[b]; this.repSt[b] = tt;
    }

    // strings are copied (U.cat(x, "")): the notes belong to the Ui and are freed on the next restyle
    addRep(n: UI.Node*, sn: UI.StyleNote*, st: int) => void {
        let name: char* = A.noteProp(sn);
        let found: int = 0 - 1;
        let i: int = 0;
        while (i < this.nrep && found < 0) {
            if (this.repSt[i] == st && inspSame(this.repName[i], name)) found = i;
            i = i + 1;
        }
        if (found >= 0) {
            this.repCnt[found] = this.repCnt[found] + 1;
        }
        else if (this.nrep < REPMAX) {
            let e: int = this.nrep;
            let src: char* = A.noteSel(sn);
            let so: int = A.noteSource(sn);
            if (so == 0) src = "browser default";
            else if (so == 2) src = "style attribute";
            if (src == nullptr) src = "";
            this.repName[e] = U.cat(name, "");
            this.repVal[e] = U.cat(A.noteValue(sn), "");
            this.repSrc[e] = U.cat(src, "");
            this.repNode[e] = n;
            this.repCnt[e] = 1;
            this.repSt[e] = st;
            this.nrep = e + 1;
        }
    }

    scanNode(n: UI.Node*) => void {
        let sn: UI.StyleNote* = A.notesOf(n);
        while (sn != nullptr) {
            this.repAll = this.repAll + 1;
            let st: int = A.noteStatus(sn);
            if (st == A.SN_UNSUPPORTED || st == A.SN_INVALID) {
                this.repDecl = this.repDecl + 1;
                this.addRep(n, sn, st);
            }
            sn = A.noteNext(sn);
        }
        let c: UI.Node* = n.firstEl();
        while (c != nullptr) { this.scanNode(c); c = c.nextEl(); }
    }

    scan(b: B.Browser*) => void {
        this.nrep = 0;
        this.repDecl = 0;
        this.repAll = 0;
        if (b.doc.root != nullptr) this.scanNode(b.doc.root);
        // insertion sort, most frequent first
        let i: int = 1;
        while (i < this.nrep) {
            let j: int = i;
            while (j > 0 && this.repCnt[j] > this.repCnt[j - 1]) { this.swapRep(j, j - 1); j = j - 1; }
            i = i + 1;
        }
        // labels and summary are built once here, not while drawing
        let e: int = 0;
        while (e < this.nrep) {
            this.repLbl[e] = U.cat(U.fromInt(this.repCnt[e]), "x");
            e = e + 1;
        }
        this.repSum = U.cat(U.cat(U.fromInt(this.nrep), " problem properties, "), U.cat(U.fromInt(this.repDecl), " declarations"));
    }

    reportView(h: float) => void {
        let pen: C.Pen2D* = this.pen;
        if (this.repAll == 0) {
            pen.text(10.0, 32.0, "no style data: reload the page while the inspector is open,", 0xFFA040FF);
            pen.text(10.0, 32.0 + (LH as float), "then click [rescan]", 0xFFA040FF);
            return;
        }
        pen.text(10.0, 28.0, this.repSum, 0xFFA040FF);
        pen.text(10.0, 28.0 + (LH as float), "click a row to inspect the first element that hit it", 0x6A6A7AFF);
        let y0: int = 28 + 2 * LH;
        let nx: float = 60.0 + this.cw * 15.0;
        let k: int = 0;
        let e: int = this.scroll;
        while (e < this.nrep && (y0 + k * 2 * LH + 2 * LH) <= (h as int)) {
            let yf: float = (y0 + k * 2 * LH) as float;
            let col: u32 = 0xFFA040FF;
            let label: char* = "UNSUPPORTED";
            if (this.repSt[e] == A.SN_INVALID) { col = 0xFF6060FF; label = "INVALID VALUE"; }
            let name: char* = this.repName[e];
            pen.text(10.0, yf, this.repLbl[e], 0xFFFFFFFF);
            pen.text(60.0, yf, label, col);
            pen.text(nx, yf, name, 0xFFFFFFFF);
            pen.text(nx + this.cw * ((U.slen(name) + 2) as float), yf, this.repVal[e], 0x8A8A9AFF);
            let tag: char* = A.tagOf(this.repNode[e]);
            pen.text(24.0, yf + (LH as float), tag, 0x7FB0FFFF);
            pen.text(24.0 + this.cw * ((U.slen(tag) + 2) as float), yf + (LH as float), this.repSrc[e], 0x8A8A9AFF);
            k = k + 1;
            e = e + 1;
        }
    }

    // call once per frame between app.frame() and app.present()
    update(b: B.Browser*) => void {
        if (this.isOpen()) {
            let win: C.Win* = this.win;
            let pen: C.Pen2D* = this.pen;
            if (this.gen != b.gen) {                                               // new document: old nodes are gone
                this.gen = b.gen;
                this.selected = nullptr;
                this.nrows = 0;
                this.scroll = 0;
                this.nrep = 0;
                this.repAll = 0;
                this.repDirty = true;
                this.sumNode = nullptr;
                this.sumStr = nullptr;
            }
            this.winW = win.width() as float;
            let h: float = win.height() as float;
            this.cw = pen.textWidth("M", 1.0);

            if (win.mousePressed()) {
                let mx: float = win.mx;
                let my: float = win.my;
                if (my < 24.0) {
                    if (this.tab >= 2 && mx >= this.winW - 150.0) {
                        if (this.tab == 2) { this.problemsOnly = !this.problemsOnly; this.scroll = 0; }
                        else { this.repDirty = true; this.scroll = 0; }
                    }
                    else {
                        if (mx < 70.0) this.tab = 0; else if (mx < 150.0) this.tab = 1; else if (mx < 230.0) this.tab = 2; else this.tab = 3;
                        this.scroll = 0;
                        if (this.tab == 3) this.repDirty = true;
                    }
                }
                else if (this.tab == 0) {
                    let k: int = ((my - 28.0) / (LH as float)) as int;
                    if (k >= 0 && k < this.nrows) this.selected = this.rows[k];
                }
                else if (this.tab == 3) {
                    let ry: float = my - (28 + 2 * LH) as float;
                    if (ry >= 0.0) {
                        let e: int = this.scroll + (ry / ((2 * LH) as float)) as int;
                        if (e < this.nrep) {
                            this.selected = this.repNode[e];
                            this.problemsOnly = true;
                            this.tab = 2;
                            this.scroll = 0;
                        }
                    }
                }
            }
            if (win.keyRepeat(264)) this.scroll = this.scroll + 1;                 // Down
            if (win.keyRepeat(265) && this.scroll > 0) this.scroll = this.scroll - 1;   // Up
            if (win.keyRepeat(267)) this.scroll = this.scroll + 10;                // PgDn
            if (win.keyRepeat(266)) { this.scroll = this.scroll - 10; if (this.scroll < 0) this.scroll = 0; }

            pen.fillRect(0.0, 0.0, this.winW, 24.0, 0x2A2A33FF);
            let c0: u32 = 0x8A8A9AFF; let c1: u32 = 0x8A8A9AFF; let c2: u32 = 0x8A8A9AFF; let c3: u32 = 0x8A8A9AFF;
            if (this.tab == 0) c0 = 0xFFFFFFFF; else if (this.tab == 1) c1 = 0xFFFFFFFF; else if (this.tab == 2) c2 = 0xFFFFFFFF; else c3 = 0xFFFFFFFF;
            pen.text(10.0, 4.0, "DOM", c0);
            pen.text(80.0, 4.0, "Console", c1);
            pen.text(160.0, 4.0, "Styles", c2);
            pen.text(240.0, 4.0, "Report", c3);
            if (this.tab == 2) {
                if (this.problemsOnly) pen.text(this.winW - 150.0, 4.0, "[x] problems only", 0xFFA040FF);
                else pen.text(this.winW - 150.0, 4.0, "[ ] problems only", 0x8A8A9AFF);
            }
            else if (this.tab == 3) {
                pen.text(this.winW - 150.0, 4.0, "[rescan]", 0x8A8A9AFF);
            }

            if (this.tab == 0) {
                this.line = 0;
                this.nrows = 0;
                let bottom: int = (h as int) - 150;
                if (b.doc.root != nullptr) this.row(b.doc.root, 0, 28, bottom);
                this.details(h - 150.0, 150.0);
            } else if (this.tab == 1) {
                this.consoleView(h);
            } else if (this.tab == 2) {
                this.stylesView(h);
            } else {
                if (this.repDirty) { this.scan(b); this.repDirty = false; }
                this.reportView(h);
            }
        }
    }
}
