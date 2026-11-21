// browser/inspector.os  -- devtools window: DOM tree (elements), attributes of the selected node, Console tab.
// Select by clicking a row, or by clicking anything in the page/toolbar while the inspector is open.
// Reads the Ui tree through firstEl()/nextEl()/getAttr() + uiapi.os only. Zero per-frame allocation.
// NEXT: computed box/style panel (needs Node geometry fields), text nodes, highlight overlay, tree collapse,
//       style editing, network tab, mouse wheel scroll, search.
module C = import "standard/1/Canvas.os";
module UI = import "standard/1/Ui.os";
module U = import "browser/util.os";
module A = import "browser/uiapi.os";
module B = import "browser/browser.os";
module Dbg = import "browser/dbg.os";

let LH: int = 16;                // row height   (ADAPT to pen.textHeight)

struct Inspector {
    win: C.Win* = nullptr;
    pen: C.Pen2D* = nullptr;
    tab: int = 0;                // 0 DOM, 1 Console
    scroll: int = 0;
    line: int = 0;
    selected: UI.Node* = nullptr;
    rows: UI.Node** = nullptr;
    nrows: int = 0;
    cw: float = 8.0;
    winW: float = 560.0;
    gen: int = 0;

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

    // call once per frame between app.frame() and app.present()
    update(b: B.Browser*) => void {
        if (this.isOpen()) {
            let win: C.Win* = this.win;
            let pen: C.Pen2D* = this.pen;
            if (this.gen != b.gen) { this.gen = b.gen; this.selected = nullptr; this.nrows = 0; this.scroll = 0; }   // new document: old nodes are gone
            this.winW = win.width() as float;
            let h: float = win.height() as float;
            this.cw = pen.textWidth("M", 1.0);

            if (win.mousePressed()) {
                let mx: float = win.mx;
                let my: float = win.my;
                if (my < 24.0) { if (mx < 70.0) this.tab = 0; else this.tab = 1; this.scroll = 0; }
                else if (this.tab == 0) {
                    let k: int = ((my - 28.0) / (LH as float)) as int;
                    if (k >= 0 && k < this.nrows) this.selected = this.rows[k];
                }
            }
            if (win.keyRepeat(264)) this.scroll = this.scroll + 1;                 // Down
            if (win.keyRepeat(265) && this.scroll > 0) this.scroll = this.scroll - 1;   // Up
            if (win.keyRepeat(267)) this.scroll = this.scroll + 10;                // PgDn
            if (win.keyRepeat(266)) { this.scroll = this.scroll - 10; if (this.scroll < 0) this.scroll = 0; }

            pen.fillRect(0.0, 0.0, this.winW, 24.0, 0x2A2A33FF);
            let c0: u32 = 0x8A8A9AFF; let c1: u32 = 0x8A8A9AFF;
            if (this.tab == 0) c0 = 0xFFFFFFFF; else c1 = 0xFFFFFFFF;
            pen.text(10.0, 4.0, "DOM", c0);
            pen.text(80.0, 4.0, "Console", c1);

            if (this.tab == 0) {
                this.line = 0;
                this.nrows = 0;
                let bottom: int = (h as int) - 150;
                if (b.doc.root != nullptr) this.row(b.doc.root, 0, 28, bottom);
                this.details(h - 150.0, 150.0);
            } else {
                this.consoleView(h);
            }
        }
    }
}
