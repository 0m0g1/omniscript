// standard/1/Ui/Events.os
// Hit test (deepest node, z-order + clip + scroll aware), hover, active, focus, click, keys, wheel, and the event queue.
// Two ways to consume events, both work today:
//   1. polling:   while (ui.poll(&ev)) { if (ev.kind == EV_CLICK && ev.node == ok) ... }
//   2. flags:     node.clicked / pressed / changed / submitted are true for the frame an event happened
//                 (clicked also bubbles to ancestors). Controls keep the Pen2D names: value, checked, text.
// Per-node handlers (node.on(EV_CLICK, fn, user)) are stored and looked up with bubbling; the one missing piece is
// the call itself, see Events.invoke (the calling convention for function pointers is not in the uploaded Canvas files).
// State written to nodes (hover, focus, active, checked) is what :hover/:focus/:active/:checked CSS reads.
// Focus order for wrapped controls is Pen2D's (creation order). Other focusables: tabindex, <a href>, <summary>.

// ---- hit testing ----
function hitSkip(p: Node*, c: Node*) => bool {
    if (c.st.display == DP_NONE || c.kind == NK_HIDDEN) return true;
    if (p.row.spec == SP_DETAILS && !p.open && c.row.spec != SP_SUMMARY) return true;
    return false;
}

function isPositioned(c: Node*) => bool { return c.kind != NK_TEXT && c.st.position != PS_STATIC; }

function hitChildren(n: Node*, x: float, y: float, ox: float, oy: float) => Node* {
    let res: Node* = nullptr;
    let zCur: int = 0; let have: bool = false; let more: bool = true;
    while (more && res == nullptr) {                       // positioned children, highest z first
        let found: bool = false; let best: int = 0;
        let c: Node* = n.first;
        while (c != nullptr) {
            if (isPositioned(c) && !hitSkip(n, c)) {
                let z: int = c.st.zIndex;
                if ((!have || z < zCur) && (!found || z > best)) { best = z; found = true; }
            }
            c = c.next;
        }
        if (!found) { more = false; }
        else {
            let d: Node* = n.last;
            while (d != nullptr && res == nullptr) {
                if (isPositioned(d) && !hitSkip(n, d) && d.st.zIndex == best) res = hitNode(d, x, y, ox, oy);
                d = d.prev;
            }
            zCur = best; have = true;
        }
    }
    let e: Node* = n.last;                                  // then normal flow, last child on top
    while (e != nullptr && res == nullptr) {
        if (!isPositioned(e) && !hitSkip(n, e)) res = hitNode(e, x, y, ox, oy);
        e = e.prev;
    }
    return res;
}

function hitNode(n: Node*, x: float, y: float, ox: float, oy: float) => Node* {
    if (n.st.display == DP_NONE || n.kind == NK_HIDDEN) return nullptr;
    let res: Node* = nullptr;
    if (n.kind == NK_TEXT) {
        if (!n.st.hidden) {
            let f: Frag* = n.frags;
            while (f != nullptr) {
                if (x >= f.x + ox && x < f.x + ox + f.w && y >= f.y + oy && y < f.y + oy + f.h) res = n;
                f = f.next;
            }
        }
        return res;
    }
    let b: Box* = &n.box;
    let bx: float = b.x + ox; let by: float = b.y + oy;
    let inside: bool = x >= bx && x < bx + b.w && y >= by && y < by + b.h;
    let clip: bool = n.st.overflow != OV_VISIBLE;
    let cox: float = ox; let coy: float = oy;
    if (clip) { cox = ox - n.scrollX; coy = oy - n.scrollY; }
    if (!clip || inside) res = hitChildren(n, x, y, cox, coy);
    if (res == nullptr && inside && !n.st.hidden) res = n;
    return res;
}

function elementOf(n: Node*) => Node* {
    let e: Node* = n;
    while (e != nullptr && e.kind == NK_TEXT) e = e.parent;
    return e;
}

function isFocusable(n: Node*) => bool {
    if (n.disabled) return false;
    if (n.kind == NK_CONTROL && n.wd != nullptr) return true;
    if (n.hasTab) return true;
    if (n.row.spec == SP_A && n.hasAttr("href")) return true;
    if (n.row.spec == SP_SUMMARY) return true;
    return false;
}

function resetFlags(n: Node*) => void {
    n.clicked = false; n.pressed = false; n.changed = false; n.submitted = false;
    let c: Node* = n.first;
    while (c != nullptr) { resetFlags(c); c = c.next; }
}

// ---- scrollbars ----
// Box coordinates are absolute; only the scroll offsets of clipping ancestors move a node on screen.
function scrollOffX(n: Node*) => float {
    let o: float = 0.0;
    let p: Node* = n.parent;
    while (p != nullptr) { if (p.st.overflow != OV_VISIBLE) o = o - p.scrollX; p = p.parent; }
    return o;
}

function scrollOffY(n: Node*) => float {
    let o: float = 0.0;
    let p: Node* = n.parent;
    while (p != nullptr) { if (p.st.overflow != OV_VISIBLE) o = o - p.scrollY; p = p.parent; }
    return o;
}

// same condition Painter uses to draw the thumb
function hasScrollbar(n: Node*) => bool {
    if (n.kind == NK_TEXT || n.st.overflow == OV_VISIBLE || n.st.overflow == OV_HIDDEN) return false;
    return n.scrollMax() > 0.0 && n.box.ch > 0.0;
}

// innermost scrollable ancestor of `from` whose scrollbar column is under (mx, my), or nullptr
function scrollbarAt(from: Node*, mx: float, my: float) => Node* {
    let res: Node* = nullptr;
    let a: Node* = from;
    while (a != nullptr && res == nullptr) {
        if (hasScrollbar(a)) {
            let b: Box* = &a.box;
            let x: float = b.x + scrollOffX(a) + b.w - b.br - sbBarW();
            let y: float = b.y + scrollOffY(a) + b.bt;
            if (mx >= x && mx < x + sbBarW() && my >= y && my < y + b.ch) res = a;
        }
        a = a.parent;
    }
    return res;
}

// cursor shape for the element under the mouse: 0 arrow, 1 I-beam (text boxes), 2 hand (links, buttons, summary)
function cursorKindOf(n: Node*) => int {
    let k: int = 0;
    let done: bool = false;
    let a: Node* = n;
    while (a != nullptr && !done) {
        if (a.kind == NK_CONTROL) {
            done = true;
            if (!a.disabled) {
                if (a.ctl == CK_TEXT || a.ctl == CK_TEXTAREA || a.ctl == CK_STUB) k = 1;
                else if (a.ctl == CK_BUTTON || a.ctl == CK_CHECK || a.ctl == CK_RADIO) k = 2;
            }
        }
        else if (a.row.spec == SP_A && a.hasAttr("href")) { k = 2; done = true; }
        else if (a.row.spec == SP_SUMMARY) { k = 2; done = true; }
        a = a.parent;
    }
    return k;
}

// horizontal scrollbar: track length (the corner is left to the vertical bar when both show)
function hTrackLen(n: Node*) => float {
    let t: float = n.box.cw;
    if (n.scrollMax() > 0.0 && n.box.ch > 0.0) t = t - sbBarW();
    return t;
}

function hasHBar(n: Node*) => bool {
    if (n.kind == NK_TEXT) return false;
    if (n.st.overflow != OV_AUTO && n.st.overflow != OV_SCROLL) return false;
    return n.scrollMaxX() > 0.0 && n.box.cw > 0.0;
}

// innermost scrollable ancestor of `from` whose horizontal scrollbar is under (mx, my), or nullptr
function hbarAt(from: Node*, mx: float, my: float) => Node* {
    let res: Node* = nullptr;
    let a: Node* = from;
    while (a != nullptr && res == nullptr) {
        if (hasHBar(a)) {
            let b: Box* = &a.box;
            let x: float = b.x + scrollOffX(a) + b.bl;
            let y: float = b.y + scrollOffY(a) + b.h - b.bb - sbBarW();
            if (mx >= x && mx < x + hTrackLen(a) && my >= y && my < y + sbBarW()) res = a;
        }
        a = a.parent;
    }
    return res;
}

struct Events {
    win: C.Win* = nullptr; pen: C.Pen2D* = nullptr;
    sbH: bool = false;                 // the scrollbar being dragged is the horizontal one
    cursor: int = 0;                   // cursor shape wanted this frame (Ui.frame passes it to the window)
    sbN: Node* = nullptr;              // scrollbar being dragged (nullptr when none)
    sbGrab: float = 0.0;               // distance from the thumb's top to where it was grabbed
    hot: Node* = nullptr;              // element under the mouse
    down: Node* = nullptr;             // element the mouse went down on
    focus: Node* = nullptr;
    q: Event* = nullptr; qHead: int = 0; qCount: int = 0;
    root: Node* = nullptr;

    init(win: C.Win*, pen: C.Pen2D*) => void {
        this.win = win; this.pen = pen;
        this.q = CMem.malloc((EV_QCAP as u64) * sizeof(Event)) as Event*;
        this.qHead = 0; this.qCount = 0;
        this.hot = nullptr; this.down = nullptr; this.focus = nullptr;
    }

    handlerOf(n: Node*, kind: int) => Handler* {
        let h: Handler* = &n.onClick;
        if (kind == EV_HOVER) h = &n.onHover;
        else if (kind == EV_KEY) h = &n.onKey;
        else if (kind == EV_INPUT) h = &n.onInput;
        else if (kind == EV_CHANGE) h = &n.onChange;
        else if (kind == EV_FOCUS) h = &n.onFocus;
        else if (kind == EV_BLUR) h = &n.onBlur;
        return h;
    }

    // NEXT: this is the only missing piece for per-node handlers. Call h.fn with (node, event, h.user) using the
    // language's function-pointer call syntax. Until then use ev polling or the node flags.
    invoke(h: Handler*, n: Node*, ev: Event*) => void { }

    push(kind: int, node: Node*) => Event* {
        let slot: int = (this.qHead + this.qCount) % EV_QCAP;
        if (this.qCount >= EV_QCAP) { this.qHead = (this.qHead + 1) % EV_QCAP; this.qCount = this.qCount - 1; slot = (this.qHead + this.qCount) % EV_QCAP; }
        let e: Event* = &this.q[slot];
        e.kind = kind; e.node = node; e.key = 0; e.ch = 0;
        e.x = 0.0; e.y = 0.0;
        if (this.win != nullptr) { e.x = this.win.mx; e.y = this.win.my; }
        this.qCount = this.qCount + 1;
        // handlers: click/key/input/change/submit bubble; hover/focus/blur do not
        let bubbles: bool = kind == EV_CLICK || kind == EV_KEY || kind == EV_INPUT || kind == EV_CHANGE;
        let a: Node* = node;
        while (a != nullptr) {
            let h: Handler* = this.handlerOf(a, kind);
            if (h.fn != nullptr) this.invoke(h, a, e);
            if (bubbles) a = a.parent; else a = nullptr;
        }
        return e;
    }

    poll(out: Event*) => bool {
        if (this.qCount == 0) return false;
        let e: Event* = &this.q[this.qHead];
        out.kind = e.kind; out.node = e.node; out.key = e.key; out.ch = e.ch; out.x = e.x; out.y = e.y;
        this.qHead = (this.qHead + 1) % EV_QCAP; this.qCount = this.qCount - 1;
        return true;
    }

    clearQueue() => void { this.qHead = 0; this.qCount = 0; }

    // ---- focus ----
    setFocus(n: Node*) => void {
        if (n == this.focus) return;
        let old: Node* = this.focus;
        if (old != nullptr) { old.focus = false; old.markPaint(); this.push(EV_BLUR, old); }
        this.focus = n;
        if (n != nullptr) { n.focus = true; n.markPaint(); this.push(EV_FOCUS, n); }
    }

    // ---- per-frame: widgets -> nodes -> events ----
    begin(root: Node*) => void { this.root = root; resetFlags(root); }

    markClicked(n: Node*) => void {
        let a: Node* = n;
        while (a != nullptr) { a.clicked = true; a = a.parent; }
    }

    formOf(n: Node*) => Node* {
        let a: Node* = n.parent;
        while (a != nullptr && a.row.spec != SP_FORM) a = a.parent;
        return a;
    }

    // radio groups: checking one unchecks the others with the same name
    radioUncheck(n: Node*, keep: Node*, name: char*) => void {
        if (n != keep && n.kind == NK_CONTROL && n.ctl == CK_RADIO && n.checked) {
            let nm: char* = n.getAttr("name");
            if (nm != nullptr && strEq(nm, name)) { n.checked = false; n.valueDirty = true; n.markPaint(); }
        }
        let c: Node* = n.first;
        while (c != nullptr) { this.radioUncheck(c, keep, name); c = c.next; }
    }

    scan(n: Node*) => void {
        if (n.kind == NK_CONTROL) {
            ctlEnsure(n, this.pen);
            let bits: int = ctlCollect(n);
            if ((bits & CB_FOCUS) != 0) {
                if (this.focus != nullptr && this.focus != n && this.focus.wd == nullptr) { this.focus.focus = false; this.push(EV_BLUR, this.focus); }
                this.focus = n; this.push(EV_FOCUS, n);
            }
            if ((bits & CB_BLUR) != 0) { if (this.focus == n) this.focus = nullptr; this.push(EV_BLUR, n); }
            if ((bits & CB_CLICK) != 0) {
                this.markClicked(n); this.push(EV_CLICK, n);
                if (n.itype == IT_SUBMIT) { let f: Node* = this.formOf(n); if (f != nullptr) this.push(EV_SUBMIT, f); }
            }
            if ((bits & CB_CHANGE) != 0) {
                if (n.ctl == CK_RADIO && n.checked) { let nm: char* = n.getAttr("name"); if (nm != nullptr) this.radioUncheck(this.root, n, nm); }
                if (n.ctl == CK_TEXT || n.ctl == CK_TEXTAREA || n.ctl == CK_STUB) this.push(EV_INPUT, n);
                else this.push(EV_CHANGE, n);
            }
            if ((bits & CB_SUBMIT) != 0) {
                let f2: Node* = this.formOf(n);
                if (f2 != nullptr) this.push(EV_SUBMIT, f2); else this.push(EV_SUBMIT, n);
            }
        }
        let c: Node* = n.first;
        while (c != nullptr) { let nx: Node* = c.next; this.scan(c); c = nx; }
    }

    // a click on non-widget content: label forwarding, <summary> toggle, link/handler clicks
    clickElement(t: Node*) => void {
        this.markClicked(t); this.push(EV_CLICK, t);
        let a: Node* = t;
        while (a != nullptr) {
            if (a.row.spec == SP_SUMMARY && a.parent != nullptr && a.parent.row.spec == SP_DETAILS) {
                a.parent.open = !a.parent.open; a.parent.markLayout(); this.push(EV_CHANGE, a.parent);
            }
            if (a.row.spec == SP_LABEL) { this.labelActivate(a); }
            a = a.parent;
        }
    }

    labelActivate(l: Node*) => void {
        let ctl: Node* = nullptr;
        let fid: char* = l.getAttr("for");
        if (fid != nullptr) ctl = this.root.findId(fid);
        else { ctl = this.firstControl(l); }
        if (ctl != nullptr && ctl.kind == NK_CONTROL && !ctl.disabled) {
            if (ctl.ctl == CK_CHECK) { ctl.checked = !ctl.checked; ctl.valueDirty = true; ctl.changed = true; ctl.markPaint(); this.push(EV_CHANGE, ctl); }
            else if (ctl.ctl == CK_RADIO) {
                if (!ctl.checked) {
                    ctl.checked = true; ctl.valueDirty = true; ctl.changed = true; ctl.markPaint();
                    let nm: char* = ctl.getAttr("name");
                    if (nm != nullptr) this.radioUncheck(this.root, ctl, nm);
                    this.push(EV_CHANGE, ctl);
                }
            }
            else if (ctl.ctl == CK_BUTTON) { ctl.pressed = true; this.markClicked(ctl); this.push(EV_CLICK, ctl); }
        }
    }

    firstControl(n: Node*) => Node* {
        let r: Node* = nullptr;
        let c: Node* = n.first;
        while (c != nullptr && r == nullptr) {
            if (c.kind == NK_CONTROL) r = c; else r = this.firstControl(c);
            c = c.next;
        }
        return r;
    }

    // ---- mouse + keyboard ----
    update(root: Node*, usesPseudo: bool) => void {
        if (this.win == nullptr) return;
        let mx: float = this.win.mx; let my: float = this.win.my;
        let hit: Node* = hitNode(root, mx, my, 0.0, 0.0);
        let target: Node* = elementOf(hit);

        // hover
        if (target != this.hot) {
            let o: Node* = this.hot;
            while (o != nullptr) {
                if (target == nullptr || !o.contains(target)) { o.hover = false; if (usesPseudo) o.markStyle(); else o.markPaint(); }
                o = o.parent;
            }
            if (this.hot != nullptr) this.push(EV_LEAVE, this.hot);
            let nn: Node* = target;
            while (nn != nullptr) {
                if (!nn.hover) { nn.hover = true; if (usesPseudo) nn.markStyle(); else nn.markPaint(); }
                nn = nn.parent;
            }
            this.hot = target;
            if (target != nullptr) this.push(EV_HOVER, target);
        }

        // scrollbar: press on the thumb drags it, press on the track jumps there; the click does not reach what is underneath
        if (this.win.mousePressed()) {
            this.sbH = false;
            this.sbN = scrollbarAt(target, mx, my);
            if (this.sbN == nullptr) { this.sbN = hbarAt(target, mx, my); this.sbH = this.sbN != nullptr; }
            if (this.sbN != nullptr) this.grabBar(mx, my);
        }
        if (this.sbN != nullptr && this.win.mouseDown()) this.dragBar(mx, my);

        // press
        if (this.win.mousePressed() && this.sbN == nullptr) {
            this.down = target;
            let a: Node* = target;
            while (a != nullptr) { a.active = true; if (usesPseudo) a.markStyle(); else a.markPaint(); a = a.parent; }
            if (target != nullptr) {
                this.push(EV_MOUSEDOWN, target);
                let f: Node* = target;
                while (f != nullptr && !isFocusable(f)) f = f.parent;
                if (f != nullptr) { if (f.wd == nullptr) this.setFocus(f); }
                else if (this.focus != nullptr && this.focus.wd == nullptr) this.setFocus(nullptr);
            }
        }
        // release
        if (this.win.mouseReleased() && this.sbN != nullptr) { this.sbN = nullptr; }
        else if (this.win.mouseReleased()) {
            let d: Node* = this.down;
            let a2: Node* = d;
            while (a2 != nullptr) { a2.active = false; if (usesPseudo) a2.markStyle(); else a2.markPaint(); a2 = a2.parent; }
            if (target != nullptr) this.push(EV_MOUSEUP, target);
            // wrapped controls report their own clicks (scan); everything else is handled here
            if (d != nullptr && d == target && !(target.kind == NK_CONTROL && target.wd != nullptr)) this.clickElement(target);
            this.down = nullptr;
        }

        // keys: every key press/repeat goes to the focused element (or the root)
        let k: int = C.KEY_FIRST;
        while (k <= C.KEY_LAST) {
            if (this.win.keyRepeat(k)) {
                let tgt: Node* = this.focus;
                if (tgt == nullptr) tgt = root;
                let e: Event* = this.push(EV_KEY, tgt);
                e.key = k;
                if ((k == 257 || k == 32) && this.focus != nullptr && this.focus.wd == nullptr) this.clickElement(this.focus);   // Enter / Space
            }
            k = k + 1;
        }

        // cursor: arrow over scrollbars, otherwise by element
        if (this.sbN != nullptr || scrollbarAt(target, mx, my) != nullptr || hbarAt(target, mx, my) != nullptr) this.cursor = 0;
        else this.cursor = cursorKindOf(target);
    }

    // ---- scrollbar drag ----
    grabBar(mx: float, my: float) => void {
        let n: Node* = this.sbN;
        let b: Box* = &n.box;
        if (this.sbH) {
            let smx: float = n.scrollMaxX();
            let tr: float = hTrackLen(n);
            let tw: float = sbThumbLen(tr, smx, sbMinThumb());
            let tx: float = b.x + scrollOffX(n) + b.bl + (tr - tw) * (n.scrollX / smx);
            if (mx >= tx && mx < tx + tw) this.sbGrab = mx - tx; else this.sbGrab = tw * 0.5;
        } else {
            let sm: float = n.scrollMax();
            let th: float = sbThumbLen(b.ch, sm, sbMinThumb());
            let ty: float = b.y + scrollOffY(n) + b.bt + (b.ch - th) * (n.scrollY / sm);
            if (my >= ty && my < ty + th) this.sbGrab = my - ty; else this.sbGrab = th * 0.5;
        }
        this.dragBar(mx, my);
    }

    dragBar(mx: float, my: float) => void {
        let n: Node* = this.sbN;
        let b: Box* = &n.box;
        if (this.sbH) {
            let smx: float = n.scrollMaxX();
            if (smx <= 0.0) return;
            let tr: float = hTrackLen(n);
            let tw: float = sbThumbLen(tr, smx, sbMinThumb());
            let hspan: float = tr - tw;
            if (hspan <= 0.0) return;
            let left: float = b.x + scrollOffX(n) + b.bl;
            let u: float = clampF((mx - this.sbGrab - left) / hspan, 0.0, 1.0);
            n.scrollByX(u * smx - n.scrollX);
            return;
        }
        let sm: float = n.scrollMax();
        if (sm <= 0.0) return;
        let th: float = sbThumbLen(b.ch, sm, sbMinThumb());
        let span: float = b.ch - th;
        if (span <= 0.0) return;
        let top: float = b.y + scrollOffY(n) + b.bt;
        let t: float = clampF((my - this.sbGrab - top) / span, 0.0, 1.0);
        n.scrollBy(t * sm - n.scrollY);
    }

    // mouse wheel / touchpad scroll; the app feeds it pixels (dy > 0 = content moves up). Scrolls the nearest
    // scrollable ancestor of the element under the mouse that can still move that way, so a list that is already
    // at its end passes the wheel on to the page behind it.
    // horizontal wheel / Shift+wheel; dx > 0 = content moves left. Same walk as wheel().
    wheelX(dx: float) => bool {
        let a: Node* = this.hot;
        let done: bool = false;
        while (a != nullptr && !done) {
            if (hasHBar(a)) {
                let smx: float = a.scrollMaxX();
                if ((dx > 0.0 && a.scrollX < smx - 0.5) || (dx < 0.0 && a.scrollX > 0.5)) { a.scrollByX(dx); done = true; }
            }
            a = a.parent;
        }
        return done;
    }

    // Returns true when something scrolled, false when nothing under the mouse could (the app may then scroll the page itself).
    wheel(dy: float) => bool {
        let a: Node* = this.hot;
        let done: bool = false;
        while (a != nullptr && !done) {
            if (a.st.overflow == OV_AUTO || a.st.overflow == OV_SCROLL) {
                let sm: float = a.scrollMax();
                if (sm > 0.0 && ((dy > 0.0 && a.scrollY < sm - 0.5) || (dy < 0.0 && a.scrollY > 0.5))) { a.scrollBy(dy); done = true; }
            }
            a = a.parent;
        }
        return done;
    }
}
