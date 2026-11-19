// standard/1/Ui/Tree.os
// The retained tree. One Node struct for every element and text run. Nodes are heap objects (malloc + memset(0)),
// so initTag()/initText() set every non-zero default. Children are a doubly linked list (no arrays to grow).
// Dirty flags: setting attributes/text/style marks the node and its ancestors; Ui.frame() restyles/relayouts/repaints.
// Api.os (sugar: panel(), row(), button(...)) is pasted into the body at the end of this struct.

struct Node {
    kind: int = 0;
    tag: char* = nullptr;              // lowercase tag name ("#text" for text runs)
    row: TagRow;                       // tag defaults (kind/display/special/control kind...)
    parent: Node* = nullptr; first: Node* = nullptr; last: Node* = nullptr;
    next: Node* = nullptr; prev: Node* = nullptr;

    // content
    text: char* = nullptr;             // text run content; button caption; text input value (mirrors the widget buffer)
    ownsText: bool = false;
    ctext: char* = nullptr;            // layout copy with white-space applied; frags slice into it
    id: char* = nullptr; cls: char* = nullptr; attrs: Attr* = nullptr; inlineCss: char* = nullptr;

    // computed
    st: Style; box: Box;
    frags: Frag* = nullptr;
    forceW: float = 0.0; forceH: float = 0.0;                    // forced border-box size from flex/table/abs (UNSET = none)
    fbase: float = 0.0; fmain: float = 0.0; fcross: float = 0.0; flexMeasured: bool = false;   // flex scratch
    scrollX: float = 0.0; scrollY: float = 0.0;

    // state (written by Events)
    hover: bool = false; focus: bool = false; active: bool = false;
    checked: bool = false; disabled: bool = false; open: bool = false;
    dirtyStyle: bool = false; subStyle: bool = false; dirtyLayout: bool = false; dirtyPaint: bool = false;

    // control (same flag names as Pen2D widgets, so app code is unchanged when native controls replace the wrappers)
    ctl: int = 0; itype: int = 0;
    wd: C.Widget* = nullptr; wpen: C.Pen2D* = nullptr;
    value: float = 0.0; min: float = 0.0; max: float = 1.0; step: float = 0.0;
    pressed: bool = false; changed: bool = false; submitted: bool = false; clicked: bool = false;
    valueDirty: bool = false;          // app changed value/checked/text: push into the widget next frame
    selIndex: int = 0;                 // select: chosen option
    imgId: int = 0; imgW: float = 0.0; imgH: float = 0.0;        // img/canvas: Pen2D image id (0 = none) and its size

    // API sugar read after the cascade (UNSET = not used). Assign right after creating a node, or call dirty() later.
    gap: float = 0.0; padding: float = 0.0; grow: float = 0.0; width: float = 0.0; height: float = 0.0;
    tabIndex: int = 0; hasTab: bool = false;

    // handlers (see Events.os: poll queue works today, handler invocation is one hook)
    onClick: Handler; onHover: Handler; onKey: Handler; onInput: Handler;
    onChange: Handler; onFocus: Handler; onBlur: Handler;

    // ---- init ----
    initCommon() => void {
        this.st.reset();
        this.forceW = UNSET; this.forceH = UNSET;
        this.gap = UNSET; this.padding = UNSET; this.grow = UNSET; this.width = UNSET; this.height = UNSET;
        this.max = 1.0;
        this.dirtyStyle = true; this.dirtyLayout = true; this.dirtyPaint = true;
    }

    initTag(name: char*) => void {
        this.initCommon();
        this.tag = strLowerDup(name);
        tagLookup(this.tag, &this.row);
        this.kind = this.row.kind;
        this.ctl = this.row.ctl;
        this.st.display = this.row.display;
        if (this.row.spec == SP_INPUT) this.itype = IT_TEXT;
        if (this.row.spec == SP_BUTTON) this.itype = IT_BUTTON;
        if (this.row.spec == SP_DETAILS || this.row.spec == SP_DIALOG) this.open = false;
    }

    initText(s: char*) => void {
        this.initCommon();
        this.tag = strDup("#text");
        this.kind = NK_TEXT;
        this.st.display = DP_INLINE;
        this.text = strDup(s); this.ownsText = true;
    }

    isElement() => bool { return this.kind != NK_TEXT; }

    // ---- dirty marking ----
    markPaint() => void {
        let n: Node* = this;
        while (n != nullptr) { n.dirtyPaint = true; n = n.parent; }
    }
    markLayout() => void {
        let n: Node* = this;
        while (n != nullptr) { n.dirtyLayout = true; n.dirtyPaint = true; n = n.parent; }
    }
    markStyle() => void {
        this.dirtyStyle = true;
        let n: Node* = this.parent;
        while (n != nullptr) { n.subStyle = true; n = n.parent; }
        this.markLayout();
    }
    dirty() => void { this.markStyle(); }          // call after assigning gap/padding/grow/width/height later

    // ---- tree ----
    detach() => void {
        if (this.parent != nullptr) {
            let p: Node* = this.parent;
            if (this.prev != nullptr) this.prev.next = this.next; else p.first = this.next;
            if (this.next != nullptr) this.next.prev = this.prev; else p.last = this.prev;
            this.parent = nullptr; this.prev = nullptr; this.next = nullptr;
            p.markLayout();
        }
    }

    append(c: Node*) => void {
        c.detach();
        c.parent = this; c.prev = this.last; c.next = nullptr;
        if (this.last != nullptr) this.last.next = c; else this.first = c;
        this.last = c;
        c.markStyle();
    }

    insertBefore(c: Node*, ref: Node*) => void {
        if (ref == nullptr) { this.append(c); }
        else {
            c.detach();
            c.parent = this; c.next = ref; c.prev = ref.prev;
            if (ref.prev != nullptr) ref.prev.next = c; else this.first = c;
            ref.prev = c;
            c.markStyle();
        }
    }

    remove() => void { this.detach(); }

    // detach and free this node and everything below it
    destroy() => void {
        this.detach();
        this.release();
    }

    release() => void {
        let c: Node* = this.first;
        while (c != nullptr) { let nx: Node* = c.next; c.release(); c = nx; }
        this.first = nullptr; this.last = nullptr;
        if (this.wd != nullptr && this.wpen != nullptr) { this.wpen.destroyWidget(this.wd); }
        this.wd = nullptr;
        this.clearFrags();
        let a: Attr* = this.attrs;
        while (a != nullptr) { let an: Attr* = a.next; strFree(a.name); strFree(a.value); CMem.free(a as void*); a = an; }
        strFree(this.tag); strFree(this.id); strFree(this.cls); strFree(this.inlineCss); strFree(this.ctext);
        if (this.ownsText) strFree(this.text);
        CMem.free(this as void*);
    }

    clearFrags() => void {
        let f: Frag* = this.frags;
        while (f != nullptr) { let fn2: Frag* = f.next; CMem.free(f as void*); f = fn2; }
        this.frags = nullptr;
    }

    // new element appended as the last child
    newChild(name: char*) => Node* {
        let n: Node* = CMem.malloc(sizeof(Node)) as Node*;
        CMem.memset(n as void*, 0, sizeof(Node));
        n.initTag(name);
        this.append(n);
        return n;
    }

    // new text run appended as the last child
    addText(s: char*) => Node* {
        let n: Node* = CMem.malloc(sizeof(Node)) as Node*;
        CMem.memset(n as void*, 0, sizeof(Node));
        n.initText(s);
        this.append(n);
        return n;
    }

    firstEl() => Node* {
        let c: Node* = this.first;
        while (c != nullptr && c.kind == NK_TEXT) c = c.next;
        return c;
    }
    nextEl() => Node* {
        let c: Node* = this.next;
        while (c != nullptr && c.kind == NK_TEXT) c = c.next;
        return c;
    }
    prevEl() => Node* {
        let c: Node* = this.prev;
        while (c != nullptr && c.kind == NK_TEXT) c = c.prev;
        return c;
    }
    contains(o: Node*) => bool {
        let found: bool = false;
        let n: Node* = o;
        while (n != nullptr) { if (n == this) found = true; n = n.parent; }
        return found;
    }

    // ---- find ----
    findId(want: char*) => Node* {
        let r: Node* = nullptr;
        if (this.id != nullptr && strEq(this.id, want)) r = this;
        let c: Node* = this.first;
        while (c != nullptr && r == nullptr) { r = c.findId(want); c = c.next; }
        return r;
    }
    findTag(want: char*) => Node* {
        let r: Node* = nullptr;
        if (this.tag != nullptr && strEq(this.tag, want)) r = this;
        let c: Node* = this.first;
        while (c != nullptr && r == nullptr) { r = c.findTag(want); c = c.next; }
        return r;
    }
    hasClass(c: char*) => bool { return hasToken(this.cls, c, 0, strLen(c)); }

    // ---- attributes ----
    getAttr(name: char*) => char* {
        let r: char* = nullptr;
        let a: Attr* = this.attrs;
        while (a != nullptr) { if (r == nullptr && strEqI(a.name, name)) r = a.value; a = a.next; }
        return r;
    }
    hasAttr(name: char*) => bool { return this.getAttr(name) != nullptr; }

    setAttr(name: char*, value: char*) => void {
        let a: Attr* = this.attrs; let found: bool = false;
        while (a != nullptr) {
            if (!found && strEqI(a.name, name)) { found = true; strFree(a.value); a.value = strDup(value); }
            a = a.next;
        }
        if (!found) {
            let na: Attr* = CMem.malloc(sizeof(Attr)) as Attr*;
            na.name = strLowerDup(name); na.value = strDup(value); na.next = this.attrs;
            this.attrs = na;
        }
        this.attrChanged(name, value);
    }

    removeAttr(name: char*) => void {
        let a: Attr* = this.attrs; let prev: Attr* = nullptr; let done: bool = false;
        while (a != nullptr && !done) {
            if (strEqI(a.name, name)) {
                if (prev != nullptr) prev.next = a.next; else this.attrs = a.next;
                strFree(a.name); strFree(a.value); CMem.free(a as void*);
                done = true;
            } else { prev = a; a = a.next; }
        }
        this.attrChanged(name, nullptr);
    }

    // keeps the cached fields (id, class, state flags, control settings) in step with the attribute list
    attrChanged(name: char*, value: char*) => void {
        let present: bool = value != nullptr;
        if (strEqI(name, "id")) { strFree(this.id); this.id = nullptr; if (present) this.id = strDup(value); }
        else if (strEqI(name, "class")) { strFree(this.cls); this.cls = nullptr; if (present) this.cls = strDup(value); }
        else if (strEqI(name, "style")) { strFree(this.inlineCss); this.inlineCss = nullptr; if (present) this.inlineCss = strDup(value); }
        else if (strEqI(name, "type")) { if (this.row.spec == SP_INPUT) this.setInputType(value); }
        else if (strEqI(name, "disabled")) { this.disabled = present; }
        else if (strEqI(name, "checked")) { this.checked = present; this.valueDirty = true; }
        else if (strEqI(name, "open")) { this.open = present; }
        else if (strEqI(name, "tabindex")) { this.hasTab = present; if (present) this.tabIndex = parseFloatStr(value, 0.0) as int; }
        else if (strEqI(name, "min")) { if (present) this.min = parseFloatStr(value, 0.0); }
        else if (strEqI(name, "max")) { if (present) this.max = parseFloatStr(value, 1.0); }
        else if (strEqI(name, "step")) { if (present) this.step = parseFloatStr(value, 0.0); }
        else if (strEqI(name, "value")) {
            if (present) {
                if (this.ctl == CK_RANGE || this.ctl == CK_PROGRESS || this.ctl == CK_METER) this.value = parseFloatStr(value, 0.0);
                else if (this.ctl == CK_TEXT || this.ctl == CK_TEXTAREA) this.setValue(value);
            }
        }
        this.markStyle();
    }

    setInputType(t: char*) => void {
        this.itype = inputTypeOf(t);
        this.ctl = ctlOfInput(this.itype);
        this.valueDirty = true;
        this.markStyle();
    }

    // ---- text / values ----
    setText(s: char*) => void {
        if (this.kind == NK_TEXT || this.ctl == CK_BUTTON || this.ctl == CK_SELECT) {
            if (this.ownsText) strFree(this.text);
            this.text = strDup(s); this.ownsText = true;
            this.markLayout();
        } else {
            let c: Node* = this.first;
            while (c != nullptr && c.kind != NK_TEXT) c = c.next;
            if (c != nullptr) c.setText(s); else this.addText(s);
        }
    }

    // text input value (copied into the widget on the next frame)
    setValue(s: char*) => void {
        if (this.ownsText) strFree(this.text);
        this.text = strDup(s); this.ownsText = true;
        this.valueDirty = true;
        this.markPaint();
    }
    getValue() => char* { return this.text; }
    setChecked(b: bool) => void { this.checked = b; this.valueDirty = true; this.markPaint(); }
    setRange(lo: float, hi: float, v: float) => void { this.min = lo; this.max = hi; this.value = v; this.valueDirty = true; this.markPaint(); }
    setDisabled(b: bool) => void { this.disabled = b; this.markStyle(); }

    // inline CSS: style() appends declarations, setStyle() replaces them
    style(css: char*) => Node* {
        let merged: char* = strCat(this.inlineCss, css);
        strFree(this.inlineCss);
        this.inlineCss = merged;
        this.markStyle();
        return this;
    }
    setStyle(css: char*) => void {
        strFree(this.inlineCss);
        this.inlineCss = strDup(css);
        this.markStyle();
    }

    // ---- scrolling (overflow: auto / scroll) ----
    scrollMax() => float {
        let m: float = this.box.contentH - this.box.ch;
        if (m < 0.0) m = 0.0;
        return m;
    }
    scrollTo(x: float, y: float) => void {
        this.scrollX = x; this.scrollY = clampF(y, 0.0, this.scrollMax());
        this.markPaint();
    }
    scrollBy(dy: float) => void { this.scrollTo(this.scrollX, this.scrollY + dy); }

    // ---- handlers: fn is a function pointer; see Events.os (invoke) ----
    on(kind: int, fn: void*, user: void*) => void {
        let h: Handler* = &this.onClick;
        if (kind == EV_HOVER) h = &this.onHover;
        else if (kind == EV_KEY) h = &this.onKey;
        else if (kind == EV_INPUT) h = &this.onInput;
        else if (kind == EV_CHANGE) h = &this.onChange;
        else if (kind == EV_FOCUS) h = &this.onFocus;
        else if (kind == EV_BLUR) h = &this.onBlur;
        h.fn = fn; h.user = user;
    }

    include "standard/1/Ui/Api.os";
}

// One event, as returned by Ui.poll(). `node` is the deepest element the event happened on.
struct Event {
    kind: int = 0; node: Node* = nullptr;
    key: int = 0; ch: int = 0; x: float = 0.0; y: float = 0.0;
}

// Free-function constructors (the HTML parser calls createByTag instead of D.newElement).
function createByTag(name: char*) => Node* {
    let n: Node* = CMem.malloc(sizeof(Node)) as Node*;
    CMem.memset(n as void*, 0, sizeof(Node));
    n.initTag(name);
    return n;
}

function createText(s: char*) => Node* {
    let n: Node* = CMem.malloc(sizeof(Node)) as Node*;
    CMem.memset(n as void*, 0, sizeof(Node));
    n.initText(s);
    return n;
}
