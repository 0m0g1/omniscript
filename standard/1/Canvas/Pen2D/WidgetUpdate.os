    // Called by Win.beginFrame() after the input snapshot is filled. Runs hit-testing and updates widget state.
    updateWidgets() => void {
        this.wantsMouse = false;
        // topmost widget under the mouse wins (last created = drawn on top)
        let hit: Widget* = nullptr;
        let i: int = this.wcount - 1;
        while (i >= 0) {
            let c: Widget* = this.widgets[i];
            if (hit == nullptr && this.focusable(c) && this.widgetHit(c)) hit = c;
            i = i - 1;
        }
        if (hit != nullptr) this.wantsMouse = true;
        if (this.focusW != nullptr && !this.focusable(this.focusW)) this.focusW = nullptr;   // hidden/disabled loses focus

        if (this.inp.pressed) {
            this.activeW = hit;
            this.focusW = hit;                      // clicking empty space clears focus
            this.kbFocus = false;
            if (hit != nullptr && hit.kind == WK_TEXTBOX) this.tbPress(hit);
        }
        if (this.inp.tab && this.wcount > 0) {
            if (this.inp.shift) this.focusNext(0 - 1); else this.focusNext(1);
        }
        if (this.inp.esc) this.focusW = nullptr;
        let stillDown: bool = this.inp.down;

        i = 0;
        while (i < this.wcount) {
            let wd: Widget* = this.widgets[i];
            wd.pressed = false; wd.released = false; wd.changed = false; wd.submitted = false;
            wd.hovered = (wd == hit) && (this.activeW == nullptr || this.activeW == wd);
            wd.down = (this.activeW == wd) && stillDown;
            wd.focused = (this.focusW == wd);
            if (wd.visible && wd.enabled) {
                if (wd.kind == WK_BUTTON || wd.kind == WK_CHECKBOX) {
                    if (this.activeW == wd && this.inp.released) {
                        wd.released = true;
                        if (hit == wd) {
                            wd.pressed = true;
                            if (wd.kind == WK_CHECKBOX) { wd.checked = !wd.checked; wd.changed = true; }
                        }
                    }
                    // Space or Enter activates the focused button / checkbox
                    if (wd.focused && (this.inp.space || this.inp.enter)) {
                        wd.pressed = true;
                        if (wd.kind == WK_CHECKBOX) { wd.checked = !wd.checked; wd.changed = true; }
                    }
                } else if (wd.kind == WK_SLIDER) {
                    if (this.activeW == wd && stillDown) {
                        let t: float = (this.inp.mx - wd.x) / wd.w;
                        if (t < 0.0) t = 0.0;
                        if (t > 1.0) t = 1.0;
                        let nv: float = wd.min + t * (wd.max - wd.min);
                        if (nv != wd.value) { wd.value = nv; wd.changed = true; }
                    }
                    if (wd.focused) {
                        let stp: float = (wd.max - wd.min) * 0.05;
                        let nv2: float = wd.value;
                        if (this.inp.left || this.inp.kDown) nv2 = nv2 - stp;
                        if (this.inp.right || this.inp.up) nv2 = nv2 + stp;
                        if (this.inp.kHome) nv2 = wd.min;
                        if (this.inp.kEnd) nv2 = wd.max;
                        if (nv2 < wd.min) nv2 = wd.min;
                        if (nv2 > wd.max) nv2 = wd.max;
                        if (nv2 != wd.value) { wd.value = nv2; wd.changed = true; }
                    }
                } else if (wd.kind == WK_TEXTBOX) {
                    if (wd.focused) this.tbUpdate(wd, stillDown);
                }
            }
            i = i + 1;
        }
        if (this.inp.released) { this.activeW = nullptr; this.noDrag = false; }
    }

    // Called by Win.endFrame() just before flush(): widgets land on top of everything the user drew.
    // All geometry is snapped to whole pixels so borders and text stay crisp.
    drawWidgets() => void {
        let i: int = 0;
        while (i < this.wcount) {
            let wd: Widget* = this.widgets[i];
            if (wd.visible) {
                let x: float = snap(wd.x); let y: float = snap(wd.y);
                let w: float = snap(wd.w); let h: float = snap(wd.h);
                let th: float = this.textHeight(1.0);
                let col: u32 = UI_IDLE;
                if (!wd.enabled) col = UI_PANEL;
                else if (wd.down) col = UI_ACTIVE;
                else if (wd.hovered) col = UI_HOT;
                let tc: u32 = UI_TEXT;
                if (!wd.enabled) tc = UI_DIM;

                if (wd.kind == WK_BUTTON) {
                    this.fillRect(x, y, w, h, col);
                    this.rect(x, y, w, h, 1.0, UI_BORDER);
                    let tw: float = this.textWidth(wd.text, 1.0);
                    this.text(x + (w - tw) * 0.5, y + (h - th) * 0.5, wd.text, tc);
                } else if (wd.kind == WK_CHECKBOX) {
                    this.fillRect(x, y, 22.0, 22.0, col);
                    this.rect(x, y, 22.0, 22.0, 1.0, UI_BORDER);
                    if (wd.checked) this.fillRect(x + 5.0, y + 5.0, 12.0, 12.0, UI_ACTIVE);
                    this.text(x + 30.0, y + (22.0 - th) * 0.5, wd.text, tc);
                } else if (wd.kind == WK_SLIDER) {
                    let ty: float = y + snap(h * 0.5) - 2.0;
                    this.fillRect(x, ty, w, 4.0, UI_PANEL);
                    this.rect(x, ty, w, 4.0, 1.0, UI_BORDER);
                    let t: float = 0.0;
                    if (wd.max > wd.min) t = (wd.value - wd.min) / (wd.max - wd.min);
                    let hx: float = x + snap(t * w) - 5.0;
                    this.fillRect(hx, y, 10.0, h, col);
                    this.rect(hx, y, 10.0, h, 1.0, UI_BORDER);
                } else if (wd.kind == WK_LABEL) {
                    this.text(x, y, wd.text, tc);
                } else if (wd.kind == WK_TEXTBOX) {
                    this.fillRect(x, y, w, h, UI_PANEL);
                    let bc: u32 = UI_BORDER;
                    if (wd.focused) bc = UI_ACTIVE;
                    this.rect(x, y, w, h, 1.0, bc);
                    let gw: float = F.GLYPH_W as float;
                    let ty: float = y + snap((h - th) * 0.5);
                    let tx: float = x + 6.0 - wd.scrollX;
                    this.pushClip((x + 3.0) as int, (y + 2.0) as int, (w - 6.0) as int, (h - 4.0) as int);
                    if (wd.focused && wd.cur != wd.anc) {
                        let sa: int = imin(wd.cur, wd.anc); let sb: int = imax(wd.cur, wd.anc);
                        this.fillRect(tx + (sa as float) * gw, ty, (sb - sa) as float * gw, th, UI_SEL);
                    }
                    if (wd.len == 0 && wd.hint != nullptr) this.text(tx, ty, wd.hint, UI_DIM);
                    else this.text(tx, ty, wd.text, tc);
                    if (wd.focused) {
                        let ph: float = (this.inp.time - wd.blinkT) as float * 2.0;
                        let on: bool = ((ph as int) % 2) == 0;
                        if (on) this.fillRect(snap(tx + (wd.cur as float) * gw), ty, 1.0, th, UI_TEXT);
                    }
                    this.popClip();
                }

                // keyboard focus ring (Tab navigation)
                if (wd.focused && this.kbFocus && wd.enabled) this.rect(x - 2.0, y - 2.0, w + 4.0, h + 4.0, 1.0, UI_FOCUS);
            }
            i = i + 1;
        }
    }
