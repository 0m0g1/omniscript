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

// <select options="A|B|C">: number of options and the label of option `idx` (new string)
function selectCount(n: Node*) => int {
    let o: char* = n.getAttr("options");
    let cnt: int = 0;
    if (o != nullptr) {
        cnt = 1;
        let i: int = 0;
        while (o[i] as int != 0) { if (o[i] as int == 124) cnt = cnt + 1; i = i + 1; }
    }
    return cnt;
}

function selectLabel(n: Node*, idx: int) => char* {
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

function selectSync(n: Node*) => void {
    let cnt: int = selectCount(n);
    if (cnt > 0) {
        if (n.selIndex >= cnt) n.selIndex = 0;
        let lbl: char* = selectLabel(n, n.selIndex);
        if (n.text == nullptr || !strEq(n.text, lbl)) { n.setText(lbl); }
        strFree(lbl);
    }
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
    else if (n.ctl == CK_SELECT && wd.pressed) {
        let cnt: int = selectCount(n);
        if (cnt > 0) { n.selIndex = (n.selIndex + 1) % cnt; selectSync(n); n.changed = true; }
    }
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
