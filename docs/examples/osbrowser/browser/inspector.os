// browser/inspector.os  -- devtools window: DOM + Styles, Console, Report, Network, Assets.
// Select by clicking a row, or by clicking anything in the page/toolbar while the inspector is open.
// Reads the Ui tree through Node.first/next (all children, text runs included), firstEl()/nextEl(), Node.attrs + uiapi.os.
//
// Tabs:  DOM      top: the element tree. Elements with children have a  v / >  arrow (click it to collapse / expand),
//                 every opened tag gets its closing tag, a leaf element shows its text inline: <p>Hello</p>.
//                 Nodes deeper than DEFDEPTH start collapsed. Clicking an element on the page expands the tree to it.
//                 bottom: the selected element: its attributes (id, class, style, src, data-bg ...), its text, the
//                 resolved background image (and whether it loaded), then every CSS declaration that hit it, with
//                 status and where it came from. "[x] problems only" (top right) hides everything that was applied.
//        Console  Dbg log (follows the newest line; scroll up to stop following)
//        Report   the whole page at once: every UNSUPPORTED / INVALID property, how often it occurred, one example
//                 value + selector + element. Click a row to jump to that element in the DOM tab. "[rescan]" rebuilds it.
//                 Needs style notes: reload the page once while the inspector is open.
//        Network  every fetch (page, css, img, background): status, bytes, final URL
//        Assets   every <img>/<svg>/CSS background-image and whether it loaded, failed or could not be decoded
//
// The mouse wheel scrolls the area under the pointer (tree or styles panel). Every tab remembers its scroll position.
// Drawing allocates nothing per frame: tree text previews are cached per node (cleared on a new document), the
// Report allocates only when it is rebuilt.
// NEXT: computed box panel (needs Node geometry fields), highlight overlay, style editing, search.
module C = import "standard/1/Canvas.os";
module UI = import "standard/1/Ui.os";
module U = import "util.os";
module A = import "uiapi.os";
module B = import "browser.os";
module Dbg = import "dbg.os";
module Net = import "net/loader.os";                // request log (Network tab)

let LH: int = 16;                // row height   (ADAPT to pen.textHeight)
let TOP: int = 28;               // first content line under the tab bar
let REPMAX: int = 96;            // distinct (property, status) pairs kept in the Report
let COLLMAX: int = 1024;         // nodes whose collapsed state was toggled by hand
let TXTMAX: int = 256;           // cached inline text previews
let DEFDEPTH: int = 6;           // tree nodes at this depth or deeper start collapsed

// tabs
let T_DOM: int = 0;
let T_CON: int = 1;
let T_REP: int = 2;
let T_NET: int = 3;
let T_ASS: int = 4;

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

// copy of raw with whitespace runs collapsed to one space, trimmed, at most maxc characters. One allocation.
function inspFit(raw: char*, maxc: int) => char* {
    let o: char* = U.newStr(maxc + 4);
    let n: int = U.slen(raw);
    let i: int = 0;
    let m: int = 0;
    let sp: bool = true;                         // true at the start: leading whitespace is dropped
    while (i < n && m < maxc) {
        let c: int = raw[i] as int;
        if (c <= 32) {
            if (!sp) { o[m] = 32 as char; m = m + 1; sp = true; }
        }
        else { o[m] = c as char; m = m + 1; sp = false; }
        i = i + 1;
    }
    if (m > 0 && sp) m = m - 1;                  // trailing space
    o[m] = 0 as char;
    return o;
}

// elements without a closing tag
function inspVoid(tag: char*) => bool {
    return inspSame(tag, "img") || inspSame(tag, "br") || inspSame(tag, "hr") || inspSame(tag, "input") ||
           inspSame(tag, "meta") || inspSame(tag, "link") || inspSame(tag, "source") || inspSame(tag, "col") ||
           inspSame(tag, "area") || inspSame(tag, "base") || inspSame(tag, "wbr") || inspSame(tag, "embed");
}

// true for nullptr and for text made only of whitespace
function inspBlank(s: char*) => bool {
    let blank: bool = true;
    if (s != nullptr) {
        let i: int = 0;
        while (blank && (s[i] as int) != 0) {
            if ((s[i] as int) > 32) blank = false;
            i = i + 1;
        }
    }
    return blank;
}

// A child the tree shows: every element, and text runs that are not just whitespace (Node.first / Node.next walk
// ALL children, text runs included; firstEl/nextEl skip the text runs).
function inspShown(c: UI.Node*) => bool {
    let r: bool = true;
    if (c.kind == UI.NK_TEXT) r = !inspBlank(c.text);
    return r;
}

function inspFirstShown(n: UI.Node*) => UI.Node* {
    let c: UI.Node* = n.first;
    while (c != nullptr && !inspShown(c)) c = c.next;
    return c;
}

function inspNextShown(n: UI.Node*) => UI.Node* {
    let c: UI.Node* = n.next;
    while (c != nullptr && !inspShown(c)) c = c.next;
    return c;
}

// f = first shown child. An element with nothing, or with exactly one text run, is drawn on one line: <p>text</p>
function inspLeaf(f: UI.Node*) => bool {
    let r: bool = f == nullptr;
    if (!r) { if (f.kind == UI.NK_TEXT && inspNextShown(f) == nullptr) r = true; }
    return r;
}

struct Inspector {
    win: C.Win* = nullptr;
    pen: C.Pen2D* = nullptr;
    tab: int = 0;                // 0 DOM, 1 Console, 2 Report, 3 Network, 4 Assets
    scroll: int = 0;             // scroll (in lines) of the current tab
    sDom: int = 0;               // saved scroll of the tabs that are not showing
    sCon: int = 0;
    sRep: int = 0;
    sNet: int = 0;
    sAss: int = 0;
    sSty: int = 0;               // scroll of the styles panel (DOM tab, lower half)
    wacc: float = 0.0;           // fractional wheel notches (touchpads)
    follow: bool = true;         // Console sticks to the newest line until you scroll up
    line: int = 0;
    cut: bool = false;           // the tree pass stopped early (rest is far below the visible rows)
    selected: UI.Node* = nullptr;
    reveal: bool = false;        // expand the tree to the selected node and scroll it into view
    rows: UI.Node** = nullptr;   // per visible tree row: node, kind (0 leaf, 1 closing tag, 2 has children), depth
    rowKind: int* = nullptr;
    rowDepth: int* = nullptr;
    nrows: int = 0;
    cw: float = 8.0;
    winW: float = 640.0;
    split: float = 400.0;        // y where the styles panel starts (DOM tab)
    treeRows: int = 20;
    myLast: float = 0.0;
    gen: int = 0;

    // tree state
    coll: UI.Node** = nullptr;   // nodes whose collapsed state differs from the depth default
    ncoll: int = 0;
    txtNode: UI.Node** = nullptr;    // inline text preview cache (round robin)
    txtStr: char** = nullptr;
    ntxt: int = 0;
    txtNext: int = 0;
    lineCnt: int = 0;            // locate(): lines counted so far / line of the selected node
    lineFound: int = 0 - 1;

    // styles panel (virtual lines, scrolled by sSty)
    noteArr: UI.StyleNote** = nullptr;   // the selected node's notes, shown last to first
    crumb: UI.Node** = nullptr;          // breadcrumb ancestors
    pv: int = 0;
    pTop: int = 0;
    pBot: int = 0;
    scratch: char* = nullptr;    // chunk buffer for wrapped text
    selTxtFor: UI.Node* = nullptr;
    selTxt: char* = "";
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
            this.win = app.createWindow("Inspector", 640, 700);
            if (this.win == nullptr) Dbg.error("inspector: createWindow failed");
            else {
                this.pen = this.win.context2d();
                this.rows = U.alloc(8 * 128) as UI.Node**;
                this.rowKind = U.alloc(8 * 128) as int*;
                this.rowDepth = U.alloc(8 * 128) as int*;
                this.coll = U.alloc(8 * COLLMAX) as UI.Node**;
                this.txtNode = U.alloc(8 * TXTMAX) as UI.Node**;
                this.txtStr = U.alloc(8 * TXTMAX) as char**;
                this.scratch = U.newStr(420);
                this.noteArr = U.alloc(8 * 1024) as UI.StyleNote**;
                this.crumb = U.alloc(8 * 32) as UI.Node**;
                this.ncoll = 0;
                this.ntxt = 0;
                this.txtNext = 0;
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

    // called by main.os when something on the page was clicked
    select(n: UI.Node*) => void { this.selected = n; this.reveal = true; }

    // ---- tabs and scrolling ----
    saveScroll() => void {
        if (this.tab == T_DOM) this.sDom = this.scroll;
        else if (this.tab == T_CON) this.sCon = this.scroll;
        else if (this.tab == T_REP) this.sRep = this.scroll;
        else if (this.tab == T_NET) this.sNet = this.scroll;
        else this.sAss = this.scroll;
    }

    loadScroll() => void {
        if (this.tab == T_DOM) this.scroll = this.sDom;
        else if (this.tab == T_CON) this.scroll = this.sCon;
        else if (this.tab == T_REP) this.scroll = this.sRep;
        else if (this.tab == T_NET) this.scroll = this.sNet;
        else this.scroll = this.sAss;
    }

    switchTab(t: int) => void {
        if (t != this.tab) {
            this.saveScroll();
            this.tab = t;
            this.loadScroll();
            if (t == T_CON) this.follow = true;
            if (t == T_REP) this.repDirty = true;
        }
    }

    // d > 0 scrolls down. In the DOM tab the area under the pointer scrolls (tree above, styles panel below).
    nudge(d: int) => void {
        if (this.tab == T_DOM && this.myLast >= this.split) {
            this.sSty = this.sSty + d;
            if (this.sSty < 0) this.sSty = 0;
        }
        else {
            this.scroll = this.scroll + d;
            if (this.scroll < 0) this.scroll = 0;
            if (d < 0) this.follow = false;
        }
    }

    // ---- DOM tree ----
    toggled(n: UI.Node*) => bool {
        let found: bool = false;
        let i: int = 0;
        while (i < this.ncoll && !found) { if (this.coll[i] == n) found = true; i = i + 1; }
        return found;
    }

    toggle(n: UI.Node*) => void {
        let at: int = 0 - 1;
        let i: int = 0;
        while (i < this.ncoll && at < 0) { if (this.coll[i] == n) at = i; i = i + 1; }
        if (at >= 0) { this.coll[at] = this.coll[this.ncoll - 1]; this.ncoll = this.ncoll - 1; }
        else if (this.ncoll < COLLMAX) { this.coll[this.ncoll] = n; this.ncoll = this.ncoll + 1; }
    }

    // default: collapsed from DEFDEPTH down; a hand toggle flips the default
    isColl(n: UI.Node*, depth: int) => bool {
        let r: bool = depth >= DEFDEPTH;
        if (this.toggled(n)) r = !r;
        return r;
    }

    // s itself when it fits in maxc chars, else its first maxc chars + "..." in the scratch buffer (no allocation).
    // The buffer is reused by the next call, so draw the result before calling again.
    trunc(s: char*, maxc: int) => char* {
        let n: int = U.slen(s);
        let out: char* = s;
        if (n > maxc) {
            let o: char* = this.scratch;
            let i: int = 0;
            while (i < maxc) {
                let c: int = s[i] as int;
                if (c < 32) c = 32;
                o[i] = c as char;
                i = i + 1;
            }
            o[i] = 46 as char; o[i + 1] = 46 as char; o[i + 2] = 46 as char; o[i + 3] = 0 as char;
            out = o;
        }
        return out;
    }

    // single-line preview of a text run (n = the text node), computed once per node
    textFor(n: UI.Node*) => char* {
        let idx: int = 0 - 1;
        let i: int = 0;
        while (i < this.ntxt && idx < 0) { if (this.txtNode[i] == n) idx = i; i = i + 1; }
        let out: char* = "";
        if (idx >= 0) out = this.txtStr[idx];
        else {
            let raw: char* = n.text;
            if (raw == nullptr) raw = "";
            out = inspFit(raw, 160);
            let slot: int = this.ntxt;
            if (this.ntxt < TXTMAX) this.ntxt = this.ntxt + 1;
            else { slot = this.txtNext; this.txtNext = (this.txtNext + 1) % TXTMAX; }
            this.txtNode[slot] = n;
            this.txtStr[slot] = out;
        }
        return out;
    }

    drawClose(x: float, y: float, tag: char*) => void {
        this.pen.text(x, y, "</", 0x6A6A7AFF);
        this.pen.text(x + this.cw * 2.0, y, tag, 0x7FB0FFFF);
        this.pen.text(x + this.cw * ((U.slen(tag) + 2) as float), y, ">", 0x6A6A7AFF);
    }

    // one tree line. kind 0 = element on one line (tn = its single text run, or nullptr), 1 = closing tag,
    // 2 = element with children
    emit(n: UI.Node*, depth: int, kind: int, coll: bool, tn: UI.Node*, bottom: int) => void {
        let idx: int = this.line;
        this.line = this.line + 1;
        let k: int = idx - this.scroll;
        let y: int = TOP + k * LH;
        if (k >= 0 && k < 128 && y + LH <= bottom) {
            this.rows[k] = n;
            this.rowKind[k] = kind;
            this.rowDepth[k] = depth;
            this.nrows = k + 1;
            let pen: C.Pen2D* = this.pen;
            let x: float = 8.0 + (depth * 12) as float;
            let yf: float = y as float;
            let tag: char* = A.tagOf(n);
            if (n == this.selected) pen.fillRect(0.0, yf, this.winW, LH as float, 0x3A5BC8FF);
            let cx: float = x + this.cw * 2.0;                      // two cells for the arrow
            if (kind == 1) { this.drawClose(cx, yf, tag); }
            else {
                if (kind == 2) {
                    if (coll) pen.text(x, yf, ">", 0xB0B0C0FF);
                    else pen.text(x, yf, "v", 0xB0B0C0FF);
                }
                pen.text(cx, yf, "<", 0x6A6A7AFF);
                cx = cx + this.cw;
                pen.text(cx, yf, tag, 0x7FB0FFFF);
                cx = cx + this.cw * (U.slen(tag) as float);
                let id: char* = A.attr(n, "id");
                if (U.slen(id) > 0) {
                    let il: float = U.slen(id) as float;
                    pen.text(cx, yf, " id=\"", 0xE0B060FF);
                    pen.text(cx + this.cw * 5.0, yf, id, 0xE0B060FF);
                    pen.text(cx + this.cw * (5.0 + il), yf, "\"", 0xE0B060FF);
                    cx = cx + this.cw * (6.0 + il);
                }
                let cls: char* = A.attr(n, "class");
                if (U.slen(cls) > 0) {
                    let cl: float = U.slen(cls) as float;
                    pen.text(cx, yf, " class=\"", 0x80D090FF);
                    pen.text(cx + this.cw * 8.0, yf, cls, 0x80D090FF);
                    pen.text(cx + this.cw * (8.0 + cl), yf, "\"", 0x80D090FF);
                    cx = cx + this.cw * (9.0 + cl);
                }
                let at: UI.Attr* = n.attrs;                         // every other attribute: name="value"
                while (at != nullptr && cx < this.winW) {
                    if (at.name != nullptr && !inspSame(at.name, "id") && !inspSame(at.name, "class")) {
                        pen.text(cx + this.cw, yf, at.name, 0xB8A0E0FF);
                        cx = cx + this.cw * ((U.slen(at.name) + 1) as float);
                        if (at.value != nullptr && U.slen(at.value) > 0) {
                            let vs: char* = this.trunc(at.value, 48);
                            let vl: float = U.slen(vs) as float;
                            pen.text(cx, yf, "=\"", 0xB8A0E0FF);
                            pen.text(cx + this.cw * 2.0, yf, vs, 0xE0C080FF);
                            pen.text(cx + this.cw * (2.0 + vl), yf, "\"", 0xB8A0E0FF);
                            cx = cx + this.cw * (3.0 + vl);
                        }
                    }
                    at = at.next;
                }
                pen.text(cx, yf, ">", 0x6A6A7AFF);
                cx = cx + this.cw;
                if (kind == 2) {
                    if (coll) {                                     // collapsed: <div>...</div>
                        pen.text(cx, yf, "...", 0x6A6A7AFF);
                        this.drawClose(cx + this.cw * 3.0, yf, tag);
                    }
                }
                else if (!inspVoid(tag)) {                          // leaf: <p>text</p>
                    let tx: char* = "";
                    if (tn != nullptr) tx = this.textFor(tn);
                    if (U.slen(tx) > 0) {
                        pen.text(cx, yf, tx, 0xE8E8F0FF);
                        cx = cx + this.cw * (U.slen(tx) as float);
                    }
                    this.drawClose(cx, yf, tag);
                }
            }
        }
    }

    // a text run between elements: one line of its text (kind 3). Clicking it selects its parent element.
    emitText(t: UI.Node*, depth: int, bottom: int) => void {
        let idx: int = this.line;
        this.line = this.line + 1;
        let k: int = idx - this.scroll;
        let y: int = TOP + k * LH;
        if (k >= 0 && k < 128 && y + LH <= bottom) {
            this.rows[k] = t;
            this.rowKind[k] = 3;
            this.rowDepth[k] = depth;
            this.nrows = k + 1;
            let x: float = 8.0 + (depth * 12) as float;
            this.pen.text(x + this.cw * 2.0, y as float, this.textFor(t), 0xE8E8F0FF);
        }
    }

    row(n: UI.Node*, depth: int, bottom: int) => void {
        if (this.line > this.scroll + 160) { this.cut = true; return; }   // far below the visible rows
        let f: UI.Node* = inspFirstShown(n);
        if (inspLeaf(f)) {
            this.emit(n, depth, 0, false, f, bottom);
        }
        else {
            let coll: bool = this.isColl(n, depth);
            this.emit(n, depth, 2, coll, nullptr, bottom);
            if (!coll) {
                let c: UI.Node* = f;
                while (c != nullptr) {
                    if (c.kind == UI.NK_TEXT) this.emitText(c, depth + 1, bottom);
                    else this.row(c, depth + 1, bottom);
                    c = inspNextShown(c);
                }
                this.emit(n, depth, 1, false, nullptr, bottom);
            }
        }
    }

    // depth of n below root (root = 0), or -1 when n is not inside root
    depthOf(n: UI.Node*, root: UI.Node*) => int {
        let d: int = 0;
        let a: UI.Node* = n;
        let ok: bool = false;
        while (a != nullptr && !ok) {
            if (a == root) ok = true;
            else { a = a.parent; d = d + 1; }
        }
        let r: int = 0 - 1;
        if (ok) r = d;
        return r;
    }

    // counts lines in drawing order until the selected node is reached (no cut-off, used by reveal only)
    locate(n: UI.Node*, depth: int) => void {
        if (n == this.selected && this.lineFound < 0) this.lineFound = this.lineCnt;
        this.lineCnt = this.lineCnt + 1;
        let f: UI.Node* = inspFirstShown(n);
        if (!inspLeaf(f) && !this.isColl(n, depth)) {               // same walk as row()
            let c: UI.Node* = f;
            while (c != nullptr) {
                if (c.kind == UI.NK_TEXT) this.lineCnt = this.lineCnt + 1;
                else this.locate(c, depth + 1);
                c = inspNextShown(c);
            }
            this.lineCnt = this.lineCnt + 1;                        // closing tag line
        }
    }

    // expand every collapsed ancestor of the selected node and scroll the tree so it is visible
    revealNode(b: B.Browser*) => void {
        let root: UI.Node* = b.doc.root;
        let n: UI.Node* = this.selected;
        if (root == nullptr || n == nullptr) return;
        if (this.depthOf(n, root) < 0) return;                      // not part of the page (browser toolbar...)
        let a: UI.Node* = n.parent;
        while (a != nullptr) {
            let d: int = this.depthOf(a, root);
            if (d >= 0 && this.isColl(a, d)) this.toggle(a);
            a = a.parent;
        }
        this.lineCnt = 0;
        this.lineFound = 0 - 1;
        this.locate(root, 0);
        if (this.lineFound >= 0) {
            let cur: int = this.sDom;
            if (this.tab == T_DOM) cur = this.scroll;
            let want: int = cur;
            if (this.lineFound < cur || this.lineFound >= cur + this.treeRows - 1) {
                want = this.lineFound - 3;
                if (want < 0) want = 0;
            }
            if (this.tab == T_DOM) this.scroll = want; else this.sDom = want;
        }
    }

    // ---- styles panel: virtual lines, so the whole panel scrolls as one list ----
    // y of the next virtual line, or -1 when it is scrolled out of the panel
    pyOf() => int {
        let vl: int = this.pv;
        this.pv = this.pv + 1;
        let y: int = this.pTop + (vl - this.sSty) * LH;
        let r: int = 0 - 1;
        if (vl >= this.sSty && y + LH <= this.pBot) r = y;
        return r;
    }

    // copy cnt chars of s from `from` into the scratch buffer (control characters become spaces)
    seg(s: char*, from: int, cnt: int) => char* {
        let o: char* = this.scratch;
        let i: int = 0;
        while (i < cnt && i < 400) {
            let c: int = s[from + i] as int;
            if (c < 32) c = 32;
            o[i] = c as char;
            i = i + 1;
        }
        o[i] = 0 as char;
        return o;
    }

    pLine(label: char*, value: char*, lcol: u32, vcol: u32) => void {
        let y: int = this.pyOf();
        if (y >= 0) {
            this.pen.text(10.0, y as float, label, lcol);
            this.pen.text(10.0 + this.cw * 22.0, y as float, value, vcol);
        }
    }

    // label + value, wrapped over up to maxLines lines
    pWrap(label: char*, s: char*, maxLines: int, lcol: u32, vcol: u32) => void {
        let n: int = U.slen(s);
        let cpl: int = ((this.winW - 16.0 - this.cw * 22.0) / this.cw) as int;
        if (cpl < 8) cpl = 8;
        let off: int = 0;
        let ln: int = 0;
        while (off < n && ln < maxLines) {
            let y: int = this.pyOf();
            let take: int = n - off;
            if (take > cpl) take = cpl;
            if (y >= 0) {
                if (ln == 0) this.pen.text(10.0, y as float, label, lcol);
                this.pen.text(10.0 + this.cw * 22.0, y as float, this.seg(s, off, take), vcol);
            }
            off = off + take;
            ln = ln + 1;
        }
    }

    pAttr(n: UI.Node*, name: char*) => void {
        let v: char* = A.attr(n, name);
        if (U.slen(v) > 0) this.pWrap(name, v, 3, 0x8A8A9AFF, 0xFFFFFFFF);
    }

    // two notes belong to the same rule when source and selector are equal
    sameRule(a: UI.StyleNote*, b: UI.StyleNote*) => bool {
        let r: bool = A.noteSource(a) == A.noteSource(b);
        if (r) {
            let sa: char* = A.noteSel(a);
            let sb: char* = A.noteSel(b);
            if (sa == nullptr || sb == nullptr) r = (sa == nullptr && sb == nullptr);
            else r = inspSame(sa, sb);
        }
        return r;
    }

    // chars of the first class token
    firstClassLen(cls: char*) => int {
        let n: int = 0;
        while ((cls[n] as int) > 32) n = n + 1;
        return n;
    }

    // width in cells of one breadcrumb:  tag.firstclass
    crumbW(a: UI.Node*) => int {
        let w: int = U.slen(A.tagOf(a));
        let cls: char* = A.attr(a, "class");
        if (U.slen(cls) > 0) w = w + 1 + this.firstClassLen(cls);
        return w;
    }

    // html body main section.hero div.background-image   (the selected one highlighted; trimmed on the left to fit)
    crumbs(b: B.Browser*, y: int) => void {
        let pen: C.Pen2D* = this.pen;
        pen.fillRect(0.0, y as float, this.winW, LH as float, 0x2A2A33FF);
        let root: UI.Node* = b.doc.root;
        let n: UI.Node* = this.selected;
        if (root == nullptr || n == nullptr) return;
        if (this.depthOf(n, root) < 0) return;
        let cnt: int = 0;
        let a: UI.Node* = n;
        while (a != nullptr && cnt < 32) {
            this.crumb[cnt] = a;
            cnt = cnt + 1;
            if (a == root) a = nullptr; else a = a.parent;
        }
        let room: int = ((this.winW - 16.0) / this.cw) as int;
        let start: int = cnt - 1;                                    // outermost crumb drawn
        let total: int = 0;
        let i: int = 0;
        while (i < cnt) { total = total + this.crumbW(this.crumb[i]) + 1; i = i + 1; }
        while (start > 0 && total > room) { total = total - this.crumbW(this.crumb[start]) - 1; start = start - 1; }
        let x: float = 8.0;
        let yf: float = y as float;
        i = start;
        while (i >= 0) {
            let c: UI.Node* = this.crumb[i];
            let tag: char* = A.tagOf(c);
            let cls: char* = A.attr(c, "class");
            let w: float = this.crumbW(c) as float;
            if (i == 0) pen.fillRect(x - 3.0, yf, this.cw * w + 6.0, LH as float, 0x3A5BC8FF);
            pen.text(x, yf, tag, 0x7FB0FFFF);
            if (U.slen(cls) > 0) {
                let cl: int = this.firstClassLen(cls);
                pen.text(x + this.cw * (U.slen(tag) as float), yf, ".", 0x6A6A7AFF);
                pen.text(x + this.cw * ((U.slen(tag) + 1) as float), yf, this.seg(cls, 0, cl), 0x80D090FF);
            }
            x = x + this.cw * (w + 1.0);
            i = i - 1;
        }
    }

    panel(top: int, bottom: int) => void {
        let pen: C.Pen2D* = this.pen;
        pen.fillRect(0.0, top as float, this.winW, (bottom - top) as float, 0x23232BFF);
        pen.fillRect(0.0, top as float, this.winW, 1.0, 0x4A4A5AFF);
        let n: UI.Node* = this.selected;
        if (n == nullptr) {
            pen.text(10.0, (top + 6) as float, "click a row (or an element on the page) to inspect", 0x8A8A9AFF);
            return;
        }
        this.pv = 0;
        this.pTop = top + 4;
        this.pBot = bottom;
        let first: UI.StyleNote* = A.notesOf(n);
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

        // header: <tag #id .class>   N unsupported or invalid
        let tag: char* = A.tagOf(n);
        let id: char* = A.attr(n, "id");
        let cls: char* = A.attr(n, "class");
        let y: int = this.pyOf();
        if (y >= 0) {
            let yf: float = y as float;
            pen.text(10.0, yf, "<", 0x6A6A7AFF);
            pen.text(10.0 + this.cw, yf, tag, 0x7FB0FFFF);
            let hx: float = 10.0 + this.cw * ((U.slen(tag) + 2) as float);
            if (U.slen(id) > 0) {
                pen.text(hx, yf, "#", 0x6A6A7AFF);
                pen.text(hx + this.cw, yf, id, 0xE0B060FF);
                hx = hx + this.cw * ((U.slen(id) + 2) as float);
            }
            if (U.slen(cls) > 0) {
                pen.text(hx, yf, ".", 0x6A6A7AFF);
                pen.text(hx + this.cw, yf, cls, 0x80D090FF);
                hx = hx + this.cw * ((U.slen(cls) + 2) as float);
            }
            if (first != nullptr) pen.text(hx + this.cw, yf, this.sumStr, 0xFFA040FF);
        }

        // attributes
        this.pAttr(n, "id");
        this.pAttr(n, "class");
        this.pAttr(n, "style");
        this.pAttr(n, "href");
        let at: UI.Attr* = n.attrs;                                 // every other attribute the node has
        while (at != nullptr) {
            if (at.name != nullptr && !inspSame(at.name, "id") && !inspSame(at.name, "class") && !inspSame(at.name, "style")) {
                if (at.value != nullptr && U.slen(at.value) > 0) this.pWrap(at.name, at.value, 3, 0x8A8A9AFF, 0xFFFFFFFF);
                else this.pLine(at.name, "(empty)", 0x8A8A9AFF, 0x6A6A7AFF);
            }
            at = at.next;
        }

        // text content (the node's own text, flattened)
        if (U.slen(this.selTxt) > 0) this.pWrap("text", this.selTxt, 3, 0x8A8A9AFF, 0xE8E8F0FF);

        // the background image the engine resolved for this element, and whether it loaded
        let bu: char* = n.st.bgUrl;
        if (bu != nullptr) {
            if (U.slen(bu) == 0) this.pLine("bg-image", "EMPTY url(): there is nothing to load", 0x8A8A9AFF, 0xFFA040FF);
            else {
                this.pWrap("bg-image", bu, 3, 0x8A8A9AFF, 0xFFFFFFFF);
                if (n.bgImgId > 0) this.pLine("bg-status", "loaded", 0x8A8A9AFF, 0x80D090FF);
                else this.pLine("bg-status", "NOT LOADED (see the Network and Assets tabs)", 0x8A8A9AFF, 0xFF6060FF);
            }
        }

        // every declaration that targets the element, in cascade order. Two lines each:
        //   STATUS  property value   /   where it came from
        // Needs ui.recordStyles (main.os switches it on while the inspector is open).
        if (first == nullptr) {
            this.pLine("styles", "no style data for this node (reload the page once the inspector is open)", 0x8A8A9AFF, 0xFFA040FF);
        }
        else {
            // Collect the notes, then show them last to first (highest priority on top, like Chrome), one block per rule:
            //   selector {                      source
            //     prop: value;
            //   }
            let nn: int = 0;
            let sn0: UI.StyleNote* = first;
            while (sn0 != nullptr && nn < 1024) { this.noteArr[nn] = sn0; nn = nn + 1; sn0 = A.noteNext(sn0); }
            let shown: int = 0;
            let i: int = nn - 1;
            while (i >= 0) {
                let a0: UI.StyleNote* = this.noteArr[i];
                let j: int = i;                                      // after the scan: index just before the block
                let cntShown: int = 0;
                let same: bool = true;
                while (j >= 0 && same) {
                    let b0: UI.StyleNote* = this.noteArr[j];
                    same = this.sameRule(a0, b0);
                    if (same) {
                        let stb: int = A.noteStatus(b0);
                        if (!this.problemsOnly || stb == A.SN_UNSUPPORTED || stb == A.SN_INVALID) cntShown = cntShown + 1;
                        j = j - 1;
                    }
                }
                if (cntShown > 0) {
                    shown = shown + cntShown;
                    let so: int = A.noteSource(a0);
                    let hdr: char* = A.noteSel(a0);
                    if (so == 2) hdr = "element.style";
                    else if (so == 0) hdr = tag;
                    if (hdr == nullptr) hdr = "(rule)";
                    let yh: int = this.pyOf();
                    if (yh >= 0) {
                        let yhf: float = yh as float;
                        let hs: char* = this.trunc(hdr, 60);
                        pen.text(10.0, yhf, hs, 0xE8E8F0FF);
                        pen.text(10.0 + this.cw * (U.slen(hs) as float), yhf, " {", 0x6A6A7AFF);
                        if (so == 0) pen.text(this.winW - 10.0 - this.cw * 21.0, yhf, "user agent stylesheet", 0x6A6A7AFF);
                    }
                    let k: int = j + 1;                              // declarations in source order
                    while (k <= i) {
                        let nb: UI.StyleNote* = this.noteArr[k];
                        let st: int = A.noteStatus(nb);
                        let prob: bool = (st == A.SN_UNSUPPORTED) || (st == A.SN_INVALID);
                        if (!this.problemsOnly || prob) {
                            let yd: int = this.pyOf();
                            if (yd >= 0) {
                                let ydf: float = yd as float;
                                let prop: char* = A.noteProp(nb);
                                let pcol: u32 = 0x7FC8F0FF;
                                let vcol: u32 = 0xE8E8F0FF;
                                let suffix: char* = "";
                                if (st == A.SN_UNSUPPORTED) { pcol = 0xFFA040FF; suffix = "  (unsupported)"; }
                                else if (st == A.SN_INVALID) { pcol = 0xFF6060FF; vcol = 0xFF6060FF; suffix = "  (invalid value)"; }
                                else if (st == A.SN_OVERRIDDEN) { pcol = 0x6A6A7AFF; vcol = 0x6A6A7AFF; }
                                let x1: float = 10.0 + this.cw * 2.0;
                                let pl: float = U.slen(prop) as float;
                                let maxv: int = ((this.winW - x1 - this.cw * (pl + 20.0)) / this.cw) as int;
                                if (maxv < 12) maxv = 12;
                                let vs: char* = this.trunc(A.noteValue(nb), maxv);
                                let vl: float = U.slen(vs) as float;
                                pen.text(x1, ydf, prop, pcol);
                                pen.text(x1 + this.cw * pl, ydf, ": ", 0x6A6A7AFF);
                                pen.text(x1 + this.cw * (pl + 2.0), ydf, vs, vcol);
                                pen.text(x1 + this.cw * (pl + 2.0 + vl), ydf, ";", 0x6A6A7AFF);
                                if (U.slen(suffix) > 0) pen.text(x1 + this.cw * (pl + 3.0 + vl), ydf, suffix, pcol);
                                if (st == A.SN_OVERRIDDEN) pen.fillRect(x1, ydf + (LH / 2) as float, this.cw * (pl + 3.0 + vl), 1.0, 0x6A6A7AFF);
                            }
                        }
                        k = k + 1;
                    }
                    let yc: int = this.pyOf();
                    if (yc >= 0) pen.text(10.0, yc as float, "}", 0x6A6A7AFF);
                }
                i = j;
            }
            if (shown == 0 && this.problemsOnly) this.pLine("styles", "no unsupported or invalid declarations on this element", 0x8A8A9AFF, 0x80D090FF);
        }
        if (this.sSty > this.pv - 1) { this.sSty = this.pv - 1; if (this.sSty < 0) this.sSty = 0; }
    }

    // ---- Console ----
    consoleView(h: float) => void {
        let n: int = Dbg.length();
        let vis: int = ((h as int) - TOP) / LH;
        let last: int = n - vis;
        if (last < 0) last = 0;
        if (this.follow) this.scroll = last;
        if (this.scroll >= last) { this.scroll = last; this.follow = true; }
        let k: int = 0;
        let i: int = this.scroll;
        while (i < n && (TOP + k * LH + LH) < (h as int)) {
            let s: char* = Dbg.get(i);
            let col: u32 = 0xE8E8F0FF;
            if ((s[1] as int) == 101 && (s[0] as int) == 91) col = 0xFF6060FF;      // "[error]"
            this.pen.text(8.0, (TOP + k * LH) as float, s, col);
            k = k + 1;
            i = i + 1;
        }
    }

    // Network tab: the last requests (newest first): ok/fail, "status  bytes", url. Rows cost no allocation (labels are
    // built once by Net.logNote).
    networkView(h: float) => void {
        let pen: C.Pen2D* = this.pen;
        let n: int = Net.logCount();
        pen.text(10.0, 28.0, "Network: newest first. Every fetch (page, css, img, background) with its final URL", 0x6A6A7AFF);
        if (n == 0) { pen.text(10.0, 28.0 + (LH as float), "no requests yet", 0x8A8A9AFF); return; }
        let k: int = 0;
        let i: int = this.scroll;
        while (i < n && (28 + (k + 1) * LH + LH) < (h as int)) {
            let yf: float = (28 + (k + 1) * LH) as float;
            let col: u32 = 0x80D090FF;
            if (!Net.logOk(i)) col = 0xFF6060FF;
            pen.text(10.0, yf, Net.logLabel(i), col);
            pen.text(130.0, yf, Net.logUrl(i), 0xE8E8F0FF);
            k = k + 1;
            i = i + 1;
        }
    }

    // Assets tab: every <img>/inline <svg> the loader queued (with its slot state) and every CSS background-image
    // found after the cascade (with whether it got an image id).
    assetsView(b: B.Browser*, h: float) => void {
        let pen: C.Pen2D* = this.pen;
        pen.text(10.0, 28.0, "Assets: <img> / <svg> first, then CSS background-image", 0x6A6A7AFF);
        if (b.bgN == 0) pen.text(10.0, 28.0 + (LH as float), "no CSS background-image url() found (an EMPTY url() has nothing to load: see the element in the DOM tab)", 0xFFA040FF);
        let total: int = b.imgN + b.bgN;
        let k: int = 0;
        let i: int = this.scroll;
        while (i < total && (28 + (k + 2) * LH + LH) < (h as int)) {
            let yf: float = (28 + (k + 2) * LH) as float;
            let label: char* = "";
            let col: u32 = 0xE8E8F0FF;
            let url: char* = "";
            if (i < b.imgN) {
                let n: UI.Node* = b.imgQ[i];
                let sl: B.ImgSlot* = &b.slots[i];
                let st: i64 = sl.st.load();
                url = "(inline svg)";
                if (sl.url != nullptr) url = sl.url;
                else if (n.getAttr("data-osb-svg") == nullptr) url = A.attr(n, "src");
                if (n.imgId > 0) { label = "img OK"; col = 0x80D090FF; }
                else if (st == B.K_FAIL) { label = "img FETCH FAILED"; col = 0xFF6060FF; }
                else if (st == B.K_BYTES) { label = "img NOT DECODED"; col = 0xFF6060FF; }
                else if (st == B.K_QUEUED) { label = "img loading"; col = 0xFFA040FF; }
                else if (st == B.K_SKIP) { label = "img skipped"; col = 0x8A8A9AFF; }
                else { label = "img pending"; col = 0xFFA040FF; }
            }
            else {
                let bn: UI.Node* = b.bgQ[i - b.imgN];
                url = bn.st.bgUrl;
                if (url == nullptr) url = "";
                if (bn.bgImgId > 0) { label = "bg OK"; col = 0x80D090FF; }
                else { label = "bg NOT LOADED"; col = 0xFF6060FF; }
            }
            pen.text(10.0, yf, label, col);
            pen.text(150.0, yf, url, 0xE8E8F0FF);
            k = k + 1;
            i = i + 1;
        }
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
                this.selTxtFor = nullptr;
                this.reveal = false;
                this.nrows = 0;
                this.sDom = 0;
                this.sSty = 0;
                if (this.tab == T_DOM) this.scroll = 0;
                this.ncoll = 0;
                this.ntxt = 0;
                this.txtNext = 0;
                this.nrep = 0;
                this.repAll = 0;
                this.repDirty = true;
                this.sumNode = nullptr;
                this.sumStr = nullptr;
            }
            this.winW = win.width() as float;
            let h: float = win.height() as float;
            this.cw = pen.textWidth("M", 1.0);
            let ph: float = h * 0.5;                                               // styles panel: lower half
            if (ph < 190.0) ph = 190.0;
            this.split = h - ph;
            if (this.split < 80.0) this.split = 80.0;
            this.treeRows = ((this.split - 28.0) / (LH as float)) as int - 1;
            this.myLast = win.my;

            // the selected node's flattened text is computed once per selection
            if (this.selected != this.selTxtFor) {
                this.selTxtFor = this.selected;
                this.sSty = 0;
                if (this.selected == nullptr) this.selTxt = "";
                else {
                    let raw: char* = A.textOf(this.selected);
                    if (raw == nullptr) raw = "";
                    this.selTxt = inspFit(raw, 360);
                }
            }
            if (this.reveal) { this.reveal = false; this.revealNode(b); }

            if (win.mousePressed()) {
                let mx: float = win.mx;
                let my: float = win.my;
                if (my < 24.0) {
                    if ((this.tab == T_DOM || this.tab == T_REP) && mx >= this.winW - 150.0) {
                        if (this.tab == T_DOM) { this.problemsOnly = !this.problemsOnly; this.sSty = 0; }
                        else { this.repDirty = true; this.scroll = 0; }
                    }
                    else {
                        if (mx < 70.0) this.switchTab(T_DOM);
                        else if (mx < 150.0) this.switchTab(T_CON);
                        else if (mx < 230.0) this.switchTab(T_REP);
                        else if (mx < 320.0) this.switchTab(T_NET);
                        else this.switchTab(T_ASS);
                    }
                }
                else if (this.tab == T_DOM) {
                    if (my >= 28.0 && my < this.split - (LH as float)) {
                        let k: int = ((my - 28.0) / (LH as float)) as int;
                        if (k >= 0 && k < this.nrows) {
                            let rn: UI.Node* = this.rows[k];
                            if (this.rowKind[k] == 3 && rn.parent != nullptr) this.selected = rn.parent;
                            else this.selected = rn;
                            if (this.rowKind[k] == 2) {                            // the arrow toggles collapse
                                let ax: float = 8.0 + (this.rowDepth[k] * 12) as float;
                                if (mx >= ax - 4.0 && mx < ax + this.cw * 2.0) this.toggle(rn);
                            }
                        }
                    }
                }
                else if (this.tab == T_REP) {
                    let ry: float = my - (28 + 2 * LH) as float;
                    if (ry >= 0.0) {
                        let e: int = this.scroll + (ry / ((2 * LH) as float)) as int;
                        if (e < this.nrep) {
                            this.selected = this.repNode[e];
                            this.problemsOnly = true;
                            this.reveal = true;
                            this.switchTab(T_DOM);
                        }
                    }
                }
            }

            // scrolling: wheel (3 lines a notch, + = rolled up) and keys act on the area under the pointer
            let wy: float = win.wheelY();
            if (wy != 0.0) {
                this.wacc = this.wacc + wy * 3.0;
                let d: int = this.wacc as int;
                this.wacc = this.wacc - (d as float);
                if (d != 0) this.nudge(0 - d);
            }
            if (win.keyRepeat(264)) this.nudge(1);                                 // Down
            if (win.keyRepeat(265)) this.nudge(0 - 1);                             // Up
            if (win.keyRepeat(267)) this.nudge(10);                                // PgDn
            if (win.keyRepeat(266)) this.nudge(0 - 10);                            // PgUp

            pen.fillRect(0.0, 0.0, this.winW, 24.0, 0x2A2A33FF);
            let c0: u32 = 0x8A8A9AFF; let c1: u32 = 0x8A8A9AFF; let c2: u32 = 0x8A8A9AFF;
            let c3: u32 = 0x8A8A9AFF; let c4: u32 = 0x8A8A9AFF;
            if (this.tab == T_DOM) c0 = 0xFFFFFFFF; else if (this.tab == T_CON) c1 = 0xFFFFFFFF;
            else if (this.tab == T_REP) c2 = 0xFFFFFFFF; else if (this.tab == T_NET) c3 = 0xFFFFFFFF; else c4 = 0xFFFFFFFF;
            pen.text(10.0, 4.0, "DOM", c0);
            pen.text(80.0, 4.0, "Console", c1);
            pen.text(160.0, 4.0, "Report", c2);
            pen.text(240.0, 4.0, "Network", c3);
            pen.text(330.0, 4.0, "Assets", c4);
            if (this.tab == T_DOM) {
                if (this.problemsOnly) pen.text(this.winW - 150.0, 4.0, "[x] problems only", 0xFFA040FF);
                else pen.text(this.winW - 150.0, 4.0, "[ ] problems only", 0x8A8A9AFF);
            }
            else if (this.tab == T_REP) {
                pen.text(this.winW - 150.0, 4.0, "[rescan]", 0x8A8A9AFF);
            }

            if (this.tab == T_DOM) {
                this.line = 0;
                this.nrows = 0;
                this.cut = false;
                let splitI: int = this.split as int;
                if (b.doc.root != nullptr) this.row(b.doc.root, 0, splitI - LH);
                if (!this.cut && this.scroll > this.line - 1) {                    // scrolled past the end (a branch was collapsed)
                    this.scroll = this.line - 1;
                    if (this.scroll < 0) this.scroll = 0;
                }
                this.crumbs(b, splitI - LH);
                this.panel(splitI, h as int);
            } else if (this.tab == T_CON) {
                this.consoleView(h);
            } else if (this.tab == T_REP) {
                if (this.repDirty) { this.scan(b); this.repDirty = false; }
                this.reportView(h);
            } else if (this.tab == T_NET) {
                this.networkView(h);
            } else {
                this.assetsView(b, h);
            }
        }
    }
}
