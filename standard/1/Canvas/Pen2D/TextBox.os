    // ---- textbox editing ----
    tbRemove(wd: Widget*, a: int, b: int) => void {
        let n: int = b - a;
        if (n <= 0) return;
        let i: int = a;
        while (i <= wd.len - n) { wd.text[i] = wd.text[i + n]; i = i + 1; }    // includes the NUL
        wd.len = wd.len - n;
    }

    // Deletes the selection if there is one. Returns true if something was deleted.
    tbDeleteSel(wd: Widget*) => bool {
        if (wd.cur == wd.anc) return false;
        let a: int = imin(wd.cur, wd.anc); let b: int = imax(wd.cur, wd.anc);
        this.tbRemove(wd, a, b);
        wd.cur = a; wd.anc = a;
        return true;
    }

    tbInsertChar(wd: Widget*, c: char) => void {
        if (wd.len >= TEXTBOX_CAP - 1) return;
        let i: int = wd.len;
        while (i >= wd.cur) { wd.text[i + 1] = wd.text[i]; i = i - 1; }
        wd.text[wd.cur] = c;
        wd.len = wd.len + 1;
        wd.cur = wd.cur + 1; wd.anc = wd.cur;
    }

    tbWordL(wd: Widget*, from: int) => int {
        let j: int = from;
        while (j > 0 && !isWordCh(wd.text[j - 1] as int)) { j = j - 1; }
        while (j > 0 && isWordCh(wd.text[j - 1] as int)) { j = j - 1; }
        return j;
    }

    tbWordR(wd: Widget*, from: int) => int {
        let j: int = from;
        while (j < wd.len && isWordCh(wd.text[j] as int)) { j = j + 1; }
        while (j < wd.len && !isWordCh(wd.text[j] as int)) { j = j + 1; }
        return j;
    }

    tbSelectWordAt(wd: Widget*, idx: int) => void {
        let a: int = idx; let b: int = idx;
        while (a > 0 && isWordCh(wd.text[a - 1] as int)) { a = a - 1; }
        while (b < wd.len && isWordCh(wd.text[b] as int)) { b = b + 1; }
        if (a == b) { a = 0; b = wd.len; }          // clicked whitespace: select everything
        wd.anc = a; wd.cur = b;
    }

    // Character index under the mouse.
    tbIndexAt(wd: Widget*) => int {
        let gw: float = F.GLYPH_W as float;
        let rel: float = this.inp.mx - (wd.x + 6.0) + wd.scrollX;
        let idx: int = 0;
        if (rel > 0.0) idx = ((rel + gw * 0.5) / gw) as int;
        return clampi(idx, 0, wd.len);
    }

    // Keep the caret inside the visible part of the box.
    tbScroll(wd: Widget*) => void {
        let gw: float = F.GLYPH_W as float;
        let view: float = wd.w - 12.0;
        let total: float = (wd.len as float) * gw;
        let cp: float = (wd.cur as float) * gw;
        if (total + 2.0 <= view) { wd.scrollX = 0.0; }
        else {
            if (cp - wd.scrollX > view - 2.0) wd.scrollX = cp - view + 2.0;
            if (cp < wd.scrollX) wd.scrollX = cp;
            let mx: float = total - view + 2.0;
            if (wd.scrollX > mx) wd.scrollX = mx;
            if (wd.scrollX < 0.0) wd.scrollX = 0.0;
        }
        wd.scrollX = snap(wd.scrollX);
    }

    clipSet(wd: Widget*) => void {
        if (this.clip == nullptr) {
            this.clip = CMem.malloc(TEXTBOX_CAP as u64) as char*;
            CMem.memset(this.clip as void*, 0, TEXTBOX_CAP as u64);
        }
        let a: int = imin(wd.cur, wd.anc); let b: int = imax(wd.cur, wd.anc);
        let i: int = a;
        while (i < b) { this.clip[i - a] = wd.text[i]; i = i + 1; }
        this.clip[b - a] = 0 as char;
        this.clipLen = b - a;
    }

    tbPress(wd: Widget*) => void {
        let idx: int = this.tbIndexAt(wd);
        this.noDrag = false;
        if (this.inp.dbl) { this.tbSelectWordAt(wd, idx); this.noDrag = true; }
        else {
            wd.cur = idx;
            if (!this.inp.shift) wd.anc = idx;      // shift+click extends the selection
        }
        wd.blinkT = this.inp.time;
    }

    tbUpdate(wd: Widget*, stillDown: bool) => void {
        let edited: bool = false;
        let moved: bool = false;
        let sh: bool = this.inp.shift;
        let ct: bool = this.inp.ctrl;

        // drag to select
        if (this.activeW == wd && stillDown && !this.inp.pressed && !this.noDrag) {
            let di: int = this.tbIndexAt(wd);
            if (di != wd.cur) { wd.cur = di; moved = true; }
        }
        if (this.inp.selAll) { wd.anc = 0; wd.cur = wd.len; moved = true; }

        // clipboard
        if ((this.inp.copy || this.inp.cut) && wd.cur != wd.anc) {
            this.clipSet(wd);
            if (this.inp.cut) { this.tbDeleteSel(wd); edited = true; }
        }
        if (this.inp.paste && this.clip != nullptr && this.clipLen > 0) {
            this.tbDeleteSel(wd);
            let pi: int = 0;
            while (pi < this.clipLen) {
                let pc: int = this.clip[pi] as int;
                if (pc >= 32 && pc <= 126) this.tbInsertChar(wd, this.clip[pi]);
                pi = pi + 1;
            }
            edited = true;
        }

        // typing replaces the selection
        if (this.inp.typed != nullptr && this.inp.typed[0] as int != 0) {
            this.tbDeleteSel(wd);
            let k: int = 0;
            while (this.inp.typed[k] as int != 0) { this.tbInsertChar(wd, this.inp.typed[k]); k = k + 1; }
            edited = true;
        }

        // deletion (ctrl = whole word)
        if (this.inp.backspace) {
            if (this.tbDeleteSel(wd)) { edited = true; }
            else if (wd.cur > 0) {
                let a: int = wd.cur - 1;
                if (ct) a = this.tbWordL(wd, wd.cur);
                this.tbRemove(wd, a, wd.cur);
                wd.cur = a; wd.anc = a; edited = true;
            }
        }
        if (this.inp.kDel) {
            if (this.tbDeleteSel(wd)) { edited = true; }
            else if (wd.cur < wd.len) {
                let b: int = wd.cur + 1;
                if (ct) b = this.tbWordR(wd, wd.cur);
                this.tbRemove(wd, wd.cur, b);
                edited = true;
            }
        }

        // caret movement (shift extends the selection, ctrl jumps by word)
        if (this.inp.left) {
            if (!sh && wd.cur != wd.anc) { wd.cur = imin(wd.cur, wd.anc); wd.anc = wd.cur; }
            else {
                let n: int = wd.cur - 1;
                if (ct) n = this.tbWordL(wd, wd.cur);
                if (n < 0) n = 0;
                wd.cur = n;
                if (!sh) wd.anc = n;
            }
            moved = true;
        }
        if (this.inp.right) {
            if (!sh && wd.cur != wd.anc) { wd.cur = imax(wd.cur, wd.anc); wd.anc = wd.cur; }
            else {
                let n2: int = wd.cur + 1;
                if (ct) n2 = this.tbWordR(wd, wd.cur);
                if (n2 > wd.len) n2 = wd.len;
                wd.cur = n2;
                if (!sh) wd.anc = n2;
            }
            moved = true;
        }
        if (this.inp.kHome) { wd.cur = 0; if (!sh) wd.anc = 0; moved = true; }
        if (this.inp.kEnd) { wd.cur = wd.len; if (!sh) wd.anc = wd.len; moved = true; }

        if (edited) wd.changed = true;
        if (edited || moved) wd.blinkT = this.inp.time;
        if (this.inp.enter) wd.submitted = true;
        this.tbScroll(wd);
    }
