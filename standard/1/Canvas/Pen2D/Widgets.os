    // ---- widgets ----
    addWidget(kind: int, x: float, y: float, w: float, hh: float) => Widget* {
        if (this.widgets == nullptr) {
            this.widgets = CMem.malloc((MAX_WIDGETS * 8) as u64) as Widget**;
            this.wcount = 0;
        }
        if (this.wcount >= MAX_WIDGETS) return nullptr;
        let wd: Widget* = CMem.malloc(sizeof(Widget)) as Widget*;
        CMem.memset(wd as void*, 0, sizeof(Widget));
        wd.kind = kind; wd.x = x; wd.y = y; wd.w = w; wd.h = hh;
        wd.visible = true; wd.enabled = true; wd.max = 1.0;
        this.widgets[this.wcount] = wd;
        this.wcount = this.wcount + 1;
        return wd;
    }

    createButton(text: char*, x: float, y: float, w: float, hh: float) => Widget* {
        let b: Widget* = this.addWidget(WK_BUTTON, x, y, w, hh);
        if (b != nullptr) b.text = text;
        return b;
    }

    createCheckbox(text: char*, x: float, y: float) => Widget* {
        let c: Widget* = this.addWidget(WK_CHECKBOX, x, y, 22.0, 22.0);
        if (c != nullptr) { c.text = text; c.w = 22.0 + 8.0 + this.textWidth(text, 1.0); }
        return c;
    }

    createSlider(x: float, y: float, w: float, lo: float, hi: float) => Widget* {
        let s: Widget* = this.addWidget(WK_SLIDER, x, y, w, 20.0);
        if (s != nullptr) { s.min = lo; s.max = hi; s.value = lo; }
        return s;
    }

    createLabel(text: char*, x: float, y: float) => Widget* {
        let l: Widget* = this.addWidget(WK_LABEL, x, y, this.textWidth(text, 1.0), this.textHeight(1.0));
        if (l != nullptr) l.text = text;
        return l;
    }

    createTextBox(x: float, y: float, w: float) => Widget* {
        let t: Widget* = this.addWidget(WK_TEXTBOX, x, y, w, 28.0);
        if (t != nullptr) {
            t.text = CMem.malloc(TEXTBOX_CAP as u64) as char*;
            CMem.memset(t.text as void*, 0, TEXTBOX_CAP as u64);
            t.owned = true;
        }
        return t;
    }

    destroyWidget(wd: Widget*) => void {
        let i: int = 0;
        let at: int = 0 - 1;
        while (i < this.wcount) { if (this.widgets[i] == wd) { at = i; } i = i + 1; }
        if (at >= 0) {
            if (this.activeW == wd) this.activeW = nullptr;
            if (this.focusW == wd) this.focusW = nullptr;
            if (wd.owned && wd.text != nullptr) CMem.free(wd.text as void*);
            CMem.free(wd as void*);
            i = at;
            while (i < this.wcount - 1) { this.widgets[i] = this.widgets[i + 1]; i = i + 1; }
            this.wcount = this.wcount - 1;
        }
    }

    freeWidgets() => void {
        let i: int = 0;
        while (i < this.wcount) {
            let wd: Widget* = this.widgets[i];
            if (wd.owned && wd.text != nullptr) CMem.free(wd.text as void*);
            CMem.free(wd as void*);
            i = i + 1;
        }
        this.wcount = 0;
        if (this.widgets != nullptr) { CMem.free(this.widgets as void*); this.widgets = nullptr; }
        if (this.clip != nullptr) { CMem.free(this.clip as void*); this.clip = nullptr; this.clipLen = 0; }
    }

    widgetHit(wd: Widget*) => bool {
        return this.inp.mx >= wd.x && this.inp.mx < wd.x + wd.w && this.inp.my >= wd.y && this.inp.my < wd.y + wd.h;
    }

    // ---- focus ----
    focusable(wd: Widget*) => bool { return wd.visible && wd.enabled && wd.kind != WK_LABEL; }

    setFocus(wd: Widget*) => void {
        this.focusW = wd;
        if (wd != nullptr && wd.kind == WK_TEXTBOX) {
            wd.anc = 0; wd.cur = wd.len;            // tabbing into a textbox selects its text
            wd.blinkT = this.inp.time;
        }
    }

    // dir = +1 (Tab) or -1 (Shift+Tab). Cycles through focusable widgets in creation order, wrapping.
    focusNext(dir: int) => void {
        let at: int = 0 - 1;
        let i: int = 0;
        while (i < this.wcount) { if (this.widgets[i] == this.focusW) at = i; i = i + 1; }
        let found: Widget* = nullptr;
        let step: int = 0;
        let idx: int = at;
        while (step < this.wcount && found == nullptr) {
            idx = idx + dir;
            if (idx >= this.wcount) idx = 0;
            if (idx < 0) idx = this.wcount - 1;
            if (this.focusable(this.widgets[idx])) found = this.widgets[idx];
            step = step + 1;
        }
        if (found != nullptr) { this.setFocus(found); this.kbFocus = true; }
    }
