// browser/inspector.os  -- F12 devtools window: DOM tree (click a row, or click the page, to select),
// computed box info for the selected node, and a Console tab (console.log + engine errors).
// Zero per-frame allocation: text is drawn in pieces, numbers go through one scratch buffer.
// NEXT: style editing, network tab, element hover pick, tree collapse, mouse wheel scroll, search.
module C = import "standard/1/Canvas.os";
module U = import "browser/util.os";
module D = import "browser/dom/dom.os";
module B = import "browser/browser.os";
module Dbg = import "browser/dbg.os";

let LH: int = 16;                // row height   (ADAPT to pen.textHeight)

function itoaBuf(n: int, out: char*) => void {
    let v: int = n;
    let p: int = 0;
    if (v < 0) { out[p] = 45 as char; p = p + 1; v = 0 - v; }
    let digits: int = 0;
    let tmp: int = v;
    if (tmp == 0) digits = 1;
    while (tmp > 0) { digits = digits + 1; tmp = tmp / 10; }
    let k: int = digits - 1;
    while (k >= 0) { out[p + k] = (48 + v % 10) as char; v = v / 10; k = k - 1; }
    out[p + digits] = 0 as char;
}

struct Inspector {
    win: C.Win* = nullptr;
    pen: C.Pen2D* = nullptr;
    tab: int = 0;                // 0 DOM, 1 Console
    scroll: int = 0;
    line: int = 0;
    selected: D.Node* = nullptr;
    rows: D.Node** = nullptr;
    nrows: int = 0;
    cw: float = 8.0;
    winW: float = 560.0;
    num: char* = nullptr;

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
                this.rows = U.alloc(8 * 128) as D.Node**;
                this.num = U.newStr(24);
            }
        }
    }

    close() => void { if (this.isOpen()) this.win.close(); }

    // outline of the selected element drawn on the page window
    highlight(pen: C.Pen2D*, top: float, scrollY: float) => void {
        if (this.isOpen() && this.selected != nullptr && this.selected.kind == 1 && this.selected.hasBounds) {
            let n: D.Node* = this.selected;
            pen.rect(n.x, n.y - scrollY + top, n.w, n.h, 1.0, 0xFF4060FF);
        }
    }

    row(n: D.Node*, depth: int, top: int, bottom: int) => void {
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
            if (n == this.selected) pen.fillRect(0.0, yf, this.winW, LH as float, 0x3A5BC8FF);
            if (n.kind == 2) { pen.text(x, yf, n.text, 0x8A8A9AFF); }
            else {
                pen.text(x, yf, "<", 0x6A6A7AFF);
                pen.text(x + this.cw, yf, n.tag, 0x7FB0FFFF);
                let ax: float = x + this.cw * ((U.slen(n.tag) + 2) as float);
                if (U.slen(n.id) > 0) {
                    pen.text(ax, yf, "#", 0x6A6A7AFF);
                    pen.text(ax + this.cw, yf, n.id, 0xE0B060FF);
                    ax = ax + this.cw * ((U.slen(n.id) + 2) as float);
                }
                if (U.slen(n.cls) > 0) {
                    pen.text(ax, yf, ".", 0x6A6A7AFF);
                    pen.text(ax + this.cw, yf, n.cls, 0x80D090FF);
                }
            }
        }
        let c: D.Node* = n.first;
        while (c != nullptr) { this.row(c, depth + 1, top, bottom); c = c.next; }
    }

    kv(label: char*, v: float, cx: float, y: float) => void {
        itoaBuf(v as int, this.num);
        this.pen.text(cx, y, label, 0x8A8A9AFF);
        this.pen.text(cx + 90.0, y, this.num, 0xFFFFFFFF);
    }

    details(top: float, h: float) => void {
        let pen: C.Pen2D* = this.pen;
        pen.fillRect(0.0, top, this.winW, h, 0x23232BFF);
        let n: D.Node* = this.selected;
        if (n == nullptr || n.kind != 1) { pen.text(10.0, top + 6.0, "click a node (or the page) to inspect", 0x8A8A9AFF); }
        else {
            pen.text(10.0, top + 4.0, "<", 0x6A6A7AFF);
            pen.text(10.0 + this.cw, top + 4.0, n.tag, 0x7FB0FFFF);
            let y: float = top + 24.0;
            let l: float = LH as float;
            this.kv("x", n.x, 10.0, y);            this.kv("font-size", n.style.fontSize, 270.0, y);
            this.kv("y", n.y, 10.0, y + l);        this.kv("margin", n.style.margin, 270.0, y + l);
            this.kv("width", n.w, 10.0, y + 2.0 * l);   this.kv("padding", n.style.padding, 270.0, y + 2.0 * l);
            this.kv("height", n.h, 10.0, y + 3.0 * l);  this.kv("border", n.style.border, 270.0, y + 3.0 * l);
            this.kv("display", n.style.display as float, 10.0, y + 4.0 * l);
            pen.text(270.0, y + 4.0 * l, "color", 0x8A8A9AFF);
            pen.fillRect(360.0, y + 4.0 * l, 14.0, 12.0, n.style.color);
            if (n.style.bgOn) pen.fillRect(380.0, y + 4.0 * l, 14.0, 12.0, n.style.bg);
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
