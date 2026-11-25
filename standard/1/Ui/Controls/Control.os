// standard/1/Ui/Controls/Control.os
// Phase 1 controls: a control node owns a Pen2D Widget*. Each frame:
//   ctlEnsure   create/destroy the widget to match the node, push app-side changes (value/checked/disabled/caption)
//   ctlCollect  copy widget results back to the node (pressed, changed, submitted, value, checked, text, focus)
//   ctlGeom     after layout: write wd.x/y/w/h/visible (hidden when display:none, closed <details>, or scrolled out)
// Mapping: button/select/input[button|submit|reset] -> button; input text-likes/textarea -> textbox;
//          checkbox/radio -> checkbox (radio group logic is in Events); range -> slider.
// progress/meter have no widget; Paint draws them. Known limits until native controls replace these:
// Pen2D-only, 256 widget cap, widgets always draw on top (no clipping in scroll areas), no TUI.
// Field names (pressed, changed, submitted, value, checked, text) are the same as Pen2D widgets, so app code survives the swap.

let CB_CLICK: int = 1;   let CB_CHANGE: int = 2;   let CB_SUBMIT: int = 4;   let CB_FOCUS: int = 8;   let CB_BLUR: int = 16;

function ctlCaption(n: Node*) => char* {
    let r: char* = n.text;
    if (r == nullptr) {
        let c: Node* = n.first;
        while (c != nullptr && r == nullptr) { if (c.kind == NK_TEXT) r = c.text; c = c.next; }
    }
    if (r == nullptr) r = "";
    return r;
}

function ctlWidgetKind(n: Node*) => int {
    let k: int = 0 - 1;
    if (n.kind == NK_CONTROL) {
        if (n.ctl == CK_BUTTON || n.ctl == CK_SELECT) k = C.WK_BUTTON;
        else if (n.ctl == CK_TEXT || n.ctl == CK_TEXTAREA || n.ctl == CK_STUB) k = C.WK_TEXTBOX;
        else if (n.ctl == CK_CHECK || n.ctl == CK_RADIO) k = C.WK_CHECKBOX;
        else if (n.ctl == CK_RANGE) k = C.WK_SLIDER;
    }
    return k;
}

// ---- <select> options ----
// Options are the <option> children of the select (also inside an <optgroup>); an `options="A|B|C"` attribute
// is still honoured when there are none. Index order = document order, optgroup rows are not counted.
function selIsTag(c: Node*, tag: char*) => bool {
    if (c.kind == NK_TEXT) return false;
    if (c.tag == nullptr) return false;
    return strEq(c.tag, tag);
}

function selOptionCount(n: Node*) => int {
    let k: int = 0;
    let c: Node* = n.first;
    while (c != nullptr) {
        if (selIsTag(c, "option")) k = k + 1;
        else if (selIsTag(c, "optgroup")) {
            let g: Node* = c.first;
            while (g != nullptr) { if (selIsTag(g, "option")) k = k + 1; g = g.next; }
        }
        c = c.next;
    }
    return k;
}

// the idx-th <option> node, or nullptr
function selOptionAt(n: Node*, idx: int) => Node* {
    let k: int = 0; let res: Node* = nullptr;
    let c: Node* = n.first;
    while (c != nullptr && res == nullptr) {
        if (selIsTag(c, "option")) { if (k == idx) res = c; k = k + 1; }
        else if (selIsTag(c, "optgroup")) {
            let g: Node* = c.first;
            while (g != nullptr && res == nullptr) {
                if (selIsTag(g, "option")) { if (k == idx) res = g; k = k + 1; }
                g = g.next;
            }
        }
        c = c.next;
    }
    return res;
}

// trimmed copy of option text (new string)
function selOptionText(o: Node*) => char* {
    let t: char* = ctlCaption(o);
    let a: int = 0; let b: int = strLen(t);
    while (a < b && isSpaceC(t[a] as int)) a = a + 1;
    while (b > a && isSpaceC(t[b - 1] as int)) b = b - 1;
    return strSub(t, a, b - a);
}

// <select options="A|B|C"> (attribute form): number of options and the label of option `idx` (new string)
function selectAttrCount(n: Node*) => int {
    let o: char* = n.getAttr("options");
    let cnt: int = 0;
    if (o != nullptr) {
        cnt = 1;
        let i: int = 0;
        while (o[i] as int != 0) { if (o[i] as int == 124) cnt = cnt + 1; i = i + 1; }
    }
    return cnt;
}

function selectAttrLabel(n: Node*, idx: int) => char* {
    let o: char* = n.getAttr("options");
    if (o == nullptr) return strDup("");
    let cur: int = 0; let start: int = 0; let i: int = 0; let res: char* = nullptr; let more: bool = true;
    while (more) {
        let c: int = o[i] as int;
        if (c == 124 || c == 0) {
            if (cur == idx) res = strSub(o, start, i - start);
            cur = cur + 1; start = i + 1;
        }
        if (c == 0) more = false; else i = i + 1;
    }
    if (res == nullptr) res = strDup("");
    return res;
}

function selectCount(n: Node*) => int {
    let cnt: int = selOptionCount(n);
    if (cnt == 0) cnt = selectAttrCount(n);
    return cnt;
}

function selectLabel(n: Node*, idx: int) => char* {
    let o: Node* = selOptionAt(n, idx);
    if (o != nullptr) return selOptionText(o);
    return selectAttrLabel(n, idx);
}

// first <option selected> becomes the initial choice (called once, when the widget is created)
function selectApplySelected(n: Node*) => void {
    let cnt: int = selOptionCount(n);
    let i: int = 0; let found: bool = false;
    while (i < cnt && !found) {
        let o: Node* = selOptionAt(n, i);
        if (o != nullptr && o.hasAttr("selected")) { n.selIndex = i; found = true; }
        i = i + 1;
    }
}

function selectSync(n: Node*) => void {
    let cnt: int = selectCount(n);
    if (cnt > 0) {
        if (n.selIndex >= cnt || n.selIndex < 0) n.selIndex = 0;
        let lbl: char* = selectLabel(n, n.selIndex);
        if (n.text == nullptr || !strEq(n.text, lbl)) { n.setText(lbl); }
        strFree(lbl);
    }
}

// value for form submission: the option's value attribute, else its text. Borrowed pointer, valid until the next
// selectSync / DOM change. "" when there are no options.
function selectSelectedValue(n: Node*) => char* {
    let o: Node* = selOptionAt(n, n.selIndex);
    if (o != nullptr) {
        let v: char* = o.getAttr("value");
        if (v != nullptr) return v;
    }
    selectSync(n);
    if (n.text == nullptr) return "";
    return n.text;
}

// scripts/apps: choose option i (ignored when out of range). Does not raise EV_CHANGE; sets no `changed` flag.
function selectSetSelected(n: Node*, i: int) => void {
    let cnt: int = selectCount(n);
    if (i >= 0 && i < cnt) { n.selIndex = i; selectSync(n); n.valueDirty = true; }
}

// Label of option idx as a slice of an existing string (no allocation; the display list keeps the pointer for the frame).
// selLabelBase = the string, selSpan(n, idx, 0) = first char, selSpan(n, idx, 1) = length. Option text is trimmed.
function selLabelBase(n: Node*, idx: int) => char* {
    let o: Node* = selOptionAt(n, idx);
    if (o != nullptr) return ctlCaption(o);
    let a: char* = n.getAttr("options");
    if (a == nullptr) return "";
    return a;
}

function selSpan(n: Node*, idx: int, which: int) => int {
    let o: Node* = selOptionAt(n, idx);
    let from: int = 0; let len: int = 0;
    if (o != nullptr) {
        let t: char* = ctlCaption(o);
        let a: int = 0; let b: int = strLen(t);
        while (a < b && isSpaceC(t[a] as int)) a = a + 1;
        while (b > a && isSpaceC(t[b - 1] as int)) b = b - 1;
        from = a; len = b - a;
    } else {
        let s: char* = n.getAttr("options");
        if (s != nullptr) {
            let cur: int = 0; let i: int = 0; let start: int = 0; let more: bool = true;
            while (more) {
                let c: int = s[i] as int;
                if (c == 124 || c == 0) {
                    if (cur == idx) { from = start; len = i - start; more = false; }
                    cur = cur + 1; start = i + 1;
                    if (c == 0) more = false;
                }
                i = i + 1;
            }
        }
    }
    if (which == 0) return from;
    return len;
}

// true when option idx (or its <optgroup>) has the `disabled` attribute. Attribute-form options are never disabled.
function selOptionDisabled(n: Node*, idx: int) => bool {
    let o: Node* = selOptionAt(n, idx);
    if (o == nullptr) return false;
    if (o.hasAttr("disabled")) return true;
    if (o.parent != nullptr && o.parent != n && o.parent.hasAttr("disabled")) return true;
    return false;
}

// next enabled option from `from` in direction dir (1 or -1); `from` itself when there is none. from = -1 starts before the
// first option, from = count starts after the last.
function selStep(n: Node*, from: int, dir: int) => int {
    let cnt: int = selectCount(n);
    let i: int = from + dir; let res: int = from; let done: bool = false;
    while (i >= 0 && i < cnt && !done) {
        if (!selOptionDisabled(n, i)) { res = i; done = true; }
        i = i + dir;
    }
    return res;
}

// first enabled option whose label starts with letter/digit `ch` (case-insensitive), searching after `from` and wrapping; -1 if none
function selFindByChar(n: Node*, from: int, ch: int) => int {
    let cnt: int = selectCount(n);
    let want: int = ch; if (want >= 65 && want <= 90) want = want + 32;
    let res: int = 0 - 1; let k: int = 1;
    while (k <= cnt && res < 0) {
        let i: int = (from + k + cnt) % cnt;
        if (!selOptionDisabled(n, i)) {
            let lbl: char* = selectLabel(n, i);
            let c: int = lbl[0] as int;
            if (c >= 65 && c <= 90) c = c + 32;
            if (c == want) res = i;
            strFree(lbl);
        }
        k = k + 1;
    }
    return res;
}

// choose option i from the UI (popup or keyboard). Returns true when the choice changed; the caller pushes EV_CHANGE.
function selectCommit(n: Node*, i: int) => bool {
    let cnt: int = selectCount(n);
    if (i < 0 || i >= cnt || selOptionDisabled(n, i)) return false;
    let diff: bool = n.selIndex != i;
    n.selIndex = i; selectSync(n); n.valueDirty = true; n.markPaint();
    if (diff) n.changed = true;
    return diff;
}

function ctlEnsure(n: Node*, pen: C.Pen2D*) => void {
    if (n.kind != NK_CONTROL) return;
    let k: int = ctlWidgetKind(n);
    if (k < 0) {
        if (n.wd != nullptr) { n.wpen.destroyWidget(n.wd); n.wd = nullptr; }
        return;
    }
    if (n.wd != nullptr && n.wd.kind != k) { n.wpen.destroyWidget(n.wd); n.wd = nullptr; n.valueDirty = true; }
    if (n.wd == nullptr) {
        if (k == C.WK_BUTTON) n.wd = pen.createButton("", 0.0, 0.0, 10.0, 10.0);
        else if (k == C.WK_TEXTBOX) n.wd = pen.createTextBox(0.0, 0.0, 10.0);
        else if (k == C.WK_CHECKBOX) n.wd = pen.createCheckbox("", 0.0, 0.0);
        else n.wd = pen.createSlider(0.0, 0.0, 10.0, n.min, n.max);
        n.wpen = pen;
        n.valueDirty = true;
        if (n.ctl == CK_SELECT) selectApplySelected(n);
    }
    let wd: C.Widget* = n.wd;
    if (wd == nullptr) return;                     // widget cap reached
    wd.enabled = !n.disabled;
    if (n.ctl == CK_SELECT) selectSync(n);
    if (k == C.WK_BUTTON) {
        wd.text = ctlCaption(n);
    }
    else if (k == C.WK_TEXTBOX) {
        wd.hint = n.getAttr("placeholder");
        if (n.valueDirty) {
            let src: char* = n.text;
            if (src == nullptr) src = "";
            wd.setText(src);
            if (n.ownsText) strFree(n.text);
            n.ownsText = false;
            n.text = wd.text;
        }
    }
    else if (k == C.WK_CHECKBOX) {
        if (n.valueDirty) wd.checked = n.checked;
    }
    else {
        wd.min = n.min; wd.max = n.max;
        if (n.valueDirty) wd.value = n.value;
    }
    n.valueDirty = false;
}

// widget -> node. Returns CB_* bits for Events to turn into queue events.
function ctlCollect(n: Node*) => int {
    let bits: int = 0;
    let wd: C.Widget* = n.wd;
    if (wd == nullptr) return 0;
    n.pressed = wd.pressed; n.changed = wd.changed; n.submitted = wd.submitted;
    if (wd.focused && !n.focus) bits = bits | CB_FOCUS;
    if (!wd.focused && n.focus) bits = bits | CB_BLUR;
    n.focus = wd.focused;
    n.active = wd.down;
    if (wd.kind == C.WK_CHECKBOX) {
        if (n.ctl == CK_RADIO && wd.changed && !wd.checked) { wd.checked = true; wd.changed = false; n.changed = false; }
        n.checked = wd.checked;
    }
    else if (wd.kind == C.WK_SLIDER) {
        if (wd.changed && n.step > 0.0) {
            let qi: int = ((wd.value - wd.min) / n.step + 0.5) as int;
            let q: float = qi as float;
            wd.value = clampF(wd.min + q * n.step, wd.min, wd.max);
        }
        n.value = wd.value;
    }
    else if (wd.kind == C.WK_TEXTBOX) { n.text = wd.text; n.ownsText = false; }
    // select: a press only reports CB_CLICK; Events opens the popup (events.pop) and commits the choice
    if (n.pressed) bits = bits | CB_CLICK;
    if (n.changed) bits = bits | CB_CHANGE;
    if (n.submitted) bits = bits | CB_SUBMIT;
    return bits;
}

// After layout: widget rectangles. (ox, oy) = accumulated scroll offset; x0..y1 = current clip rect.
function ctlGeom(n: Node*, ox: float, oy: float, x0: float, y0: float, x1: float, y1: float, hide: bool) => void {
    if (n.kind == NK_TEXT) return;
    let hid: bool = hide || n.st.display == DP_NONE;
    if (n.wd != nullptr) {
        let wd: C.Widget* = n.wd;
        wd.x = n.box.x + ox; wd.y = n.box.y + oy; wd.w = n.box.w; wd.h = n.box.h;
        let inside: bool = wd.x < x1 && wd.x + wd.w > x0 && wd.y < y1 && wd.y + wd.h > y0;
        wd.visible = !hid && !n.st.hidden && inside;
    }
    let cx0: float = x0; let cy0: float = y0; let cx1: float = x1; let cy1: float = y1;
    let cox: float = ox; let coy: float = oy;
    if (n.st.overflow != OV_VISIBLE) {
        cx0 = maxF(x0, n.box.x + n.box.bl + ox);
        cy0 = maxF(y0, n.box.y + n.box.bt + oy);
        cx1 = minF(x1, n.box.x + n.box.w - n.box.br + ox);
        cy1 = minF(y1, n.box.y + n.box.h - n.box.bb + oy);
        cox = ox - n.scrollX; coy = oy - n.scrollY;
    }
    let c: Node* = n.first;
    while (c != nullptr) {
        let ch: bool = hid;
        if (n.row.spec == SP_DETAILS && !n.open && c.row.spec != SP_SUMMARY) ch = true;
        ctlGeom(c, cox, coy, cx0, cy0, cx1, cy1, ch);
        c = c.next;
    }
}

// While a popup is open: hide every widget (except `keep`, the select itself) whose rectangle overlaps the popup rectangle,
// because Pen2D widgets draw above the display list. Call after ctlGeom each frame the popup is open.
function ctlHideUnder(n: Node*, keep: Node*, px: float, py: float, pw: float, ph: float) => void {
    if (n.kind == NK_TEXT) return;
    if (n.wd != nullptr && n != keep) {
        let wd: C.Widget* = n.wd;
        if (wd.visible && wd.x < px + pw && wd.x + wd.w > px && wd.y < py + ph && wd.y + wd.h > py) wd.visible = false;
    }
    let c: Node* = n.first;
    while (c != nullptr) { ctlHideUnder(c, keep, px, py, pw, ph); c = c.next; }
}
