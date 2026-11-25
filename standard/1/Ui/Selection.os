// standard/1/Ui/Selection.os
// Page-text selection (drag, double-click word, Ctrl+A), copy to the in-app clipboard (pen.clip) and a right-click menu.
// Text is monospace, so a character position inside a Frag is (x - f.x) / (f.w / f.len). A selection is two (text node, offset
// into node.ctext) points. Painting and copying both walk the tree in document order through Selection.frag().
// Include after Popup.os (Events and Paint use these types). Not compiled/run yet. No `return` inside loop bodies.

let SEL_BIG: int = 1000000000;
let SEL_COLOR: u32 = 0x5B8DEF70;

function isWordCh(c: int) => bool {
    if (c >= 48 && c <= 57) return true;
    if (c >= 65 && c <= 90) return true;
    if (c >= 97 && c <= 122) return true;
    if (c == 95 || c >= 128) return true;
    return false;
}

struct Selection {
    an: Node* = nullptr; ao: int = 0;          // anchor (where the drag started)
    fn: Node* = nullptr; fo: int = 0;          // focus (where the mouse is now)
    dragging: bool = false; moved: bool = false;
    px: float = 0.0; py: float = 0.0;          // mouse position at press
    pn: Node* = nullptr; po: int = 0;          // result of pick()
    // document-order walk state
    cur: Node* = nullptr; inside: bool = false; done: bool = false;
    ns: int = 0; ne: int = 0;                  // selected range of the current node
    lo: int = 0; hi: int = 0;                  // selected range of the current frag (result of frag())
    // copy state
    out: char* = nullptr; lastN: Node* = nullptr; lastY: float = 0.0;

    reset() => void {
        this.an = nullptr; this.ao = 0; this.fn = nullptr; this.fo = 0;
        this.dragging = false; this.moved = false; this.px = 0.0; this.py = 0.0;
        this.pn = nullptr; this.po = 0; this.cur = nullptr; this.inside = false; this.done = false;
        this.ns = 0; this.ne = 0; this.lo = 0; this.hi = 0; this.out = nullptr; this.lastN = nullptr; this.lastY = 0.0;
    }

    clear() => void { this.an = nullptr; this.fn = nullptr; this.ao = 0; this.fo = 0; this.dragging = false; this.moved = false; }

    active() => bool {
        if (this.an == nullptr || this.fn == nullptr) return false;
        if (this.an == this.fn && this.ao == this.fo) return false;
        return true;
    }

    walkStart() => void { this.cur = nullptr; this.inside = false; this.done = false; this.ns = 0; this.ne = 0; }

    // first visit of text node n in document order: work out which part of it is selected
    enterNode(n: Node*) => void {
        this.cur = n; this.ns = 0; this.ne = 0;
        let isA: bool = n == this.an; let isF: bool = n == this.fn;
        if (this.an == this.fn) {
            if (isA) {
                this.ns = this.ao; this.ne = this.fo;
                if (this.ns > this.ne) { this.ns = this.fo; this.ne = this.ao; }
            }
        }
        else if (this.done) { }
        else if (!this.inside) {
            if (isA || isF) {
                if (isA) this.ns = this.ao; else this.ns = this.fo;
                this.ne = SEL_BIG; this.inside = true;
            }
        }
        else {
            if (isA || isF) {
                this.ns = 0;
                if (isA) this.ne = this.ao; else this.ne = this.fo;
                this.inside = false; this.done = true;
            } else { this.ns = 0; this.ne = SEL_BIG; }
        }
    }

    // true when part of frag f of text node n is selected; the range is then in lo..hi (offsets into n.ctext)
    frag(n: Node*, f: Frag*) => bool {
        if (!this.active()) return false;
        if (n != this.cur) this.enterNode(n);
        let a: int = f.from; if (this.ns > a) a = this.ns;
        let b: int = f.from + f.len; if (this.ne < b) b = this.ne;
        if (b > a) { this.lo = a; this.hi = b; return true; }
        return false;
    }

    // text position under the mouse -> pn / po. false when the mouse is not over text.
    pick(root: Node*, mx: float, my: float) => bool {
        let hit: Node* = hitNode(root, mx, my, 0.0, 0.0);
        if (hit == nullptr || hit.kind != NK_TEXT || hit.ctext == nullptr) return false;
        let ox: float = scrollOffX(hit); let oy: float = scrollOffY(hit);
        let f: Frag* = hit.frags; let found: bool = false;
        while (f != nullptr && !found) {
            if (f.len > 0 && mx >= f.x + ox && mx < f.x + ox + f.w && my >= f.y + oy && my < f.y + oy + f.h) {
                found = true;
                let cw: float = f.w / (f.len as float);
                let k: int = ((mx - f.x - ox) / cw + 0.5) as int;
                if (k < 0) k = 0;
                if (k > f.len) k = f.len;
                this.pn = hit; this.po = f.from + k;
            }
            f = f.next;
        }
        return found;
    }

    // select the word around offset off of text node n
    wordAt(n: Node*, off: int) => void {
        let t: char* = n.ctext; let len: int = strLen(t);
        if (len == 0) return;
        let p: int = off; if (p >= len) p = len - 1; if (p < 0) p = 0;
        if (p > 0 && !isWordCh(t[p] as int) && isWordCh(t[p - 1] as int)) p = p - 1;
        let a: int = p; let b: int = p;
        while (a > 0 && isWordCh(t[a - 1] as int)) a = a - 1;
        while (b < len && isWordCh(t[b] as int)) b = b + 1;
        if (b == a) b = a + 1;                       // punctuation: just that character
        this.an = n; this.ao = a; this.fn = n; this.fo = b;
        this.dragging = false; this.moved = false;
    }

    append(n: Node*, f: Frag*, piece: char*) => void {
        let sep: char* = "";
        if (this.lastN != nullptr) {
            if (this.lastN == n) sep = " ";                                  // soft-wrapped line of the same run
            else if (f.y - this.lastY > f.h * 0.5) sep = "\n";               // a different line
        }
        let a: char* = strCat(this.out, sep);
        strFree(this.out);
        let b: char* = strCat(a, piece);
        strFree(a); strFree(piece);
        this.out = b; this.lastN = n; this.lastY = f.y;
    }
}

function selCollect(n: Node*, s: Selection*) => void {
    if (n.st.display == DP_NONE || n.kind == NK_HIDDEN) return;
    if (n.kind == NK_TEXT) {
        if (n.st.hidden || n.ctext == nullptr) return;
        let f: Frag* = n.frags;
        while (f != nullptr) {
            if (s.frag(n, f)) s.append(n, f, strSub(n.ctext, s.lo, s.hi - s.lo));
            f = f.next;
        }
        return;
    }
    let c: Node* = n.first;
    while (c != nullptr) { selCollect(c, s); c = c.next; }
}

// the selected text as a new string (caller owns it); "" when nothing is selected
function selectedText(root: Node*, s: Selection*) => char* {
    s.walkStart();
    s.out = strDup(""); s.lastN = nullptr; s.lastY = 0.0;
    if (s.active()) selCollect(root, s);
    let r: char* = s.out; s.out = nullptr;
    return r;
}

function firstText(n: Node*) => Node* {
    if (n.st.display == DP_NONE || n.kind == NK_HIDDEN) return nullptr;
    if (n.kind == NK_TEXT) {
        if (n.ctext != nullptr && n.frags != nullptr && !n.st.hidden) return n;
        return nullptr;
    }
    let r: Node* = nullptr; let c: Node* = n.first;
    while (c != nullptr && r == nullptr) { r = firstText(c); c = c.next; }
    return r;
}

function lastText(n: Node*) => Node* {
    if (n.st.display == DP_NONE || n.kind == NK_HIDDEN) return nullptr;
    if (n.kind == NK_TEXT) {
        if (n.ctext != nullptr && n.frags != nullptr && !n.st.hidden) return n;
        return nullptr;
    }
    let r: Node* = nullptr; let c: Node* = n.first;
    while (c != nullptr) { let t: Node* = lastText(c); if (t != nullptr) r = t; c = c.next; }
    return r;
}

function selectAllText(root: Node*, s: Selection*) => void {
    let a: Node* = firstText(root); let b: Node* = lastText(root);
    if (a == nullptr || b == nullptr) { s.clear(); return; }
    s.an = a; s.ao = 0; s.fn = b; s.fo = strLen(b.ctext);
    s.dragging = false; s.moved = false;
}

// ---- right-click menu: Copy / Select all ----
let CTX_COUNT: int = 2;

function ctxLabel(i: int) => char* {
    if (i == 0) return "Copy";
    return "Select all";
}

struct CtxMenu {
    isOn: bool = false;
    x: float = 0.0; y: float = 0.0; w: float = 0.0; h: float = 0.0;
    rowH: float = 26.0; count: int = 2; hover: int = 0 - 1;
    grab: bool = false; lastX: float = 0.0; lastY: float = 0.0;

    reset() => void { this.isOn = false; this.hover = 0 - 1; this.grab = false; this.count = CTX_COUNT; this.rowH = 26.0; }
    isOpen() => bool { return this.isOn; }

    show(mx: float, my: float, vw: float, vh: float) => void {
        this.isOn = true; this.count = CTX_COUNT; this.rowH = 26.0;
        this.w = 128.0; this.h = (this.count as float) * this.rowH + 2.0;
        this.x = mx; this.y = my;
        if (vw > 0.0 && this.x + this.w > vw) this.x = maxF(0.0, vw - this.w);
        if (vh > 0.0 && this.y + this.h > vh) this.y = maxF(0.0, vh - this.h);
        this.hover = 0 - 1; this.grab = false; this.lastX = mx; this.lastY = my;
    }

    close() => void { this.isOn = false; this.hover = 0 - 1; this.grab = false; }

    contains(mx: float, my: float) => bool {
        if (!this.isOn) return false;
        return mx >= this.x && mx < this.x + this.w && my >= this.y && my < this.y + this.h;
    }

    rowAt(mx: float, my: float) => int {
        if (!this.contains(mx, my)) return 0 - 1;
        let i: int = ((my - this.y - 1.0) / this.rowH) as int;
        if (my - this.y - 1.0 < 0.0 || i >= this.count) return 0 - 1;
        return i;
    }
}
