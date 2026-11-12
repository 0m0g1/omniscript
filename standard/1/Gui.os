// standard/1/Gui.os
// Tier 7: immediate-mode GUI on top of a Canvas Pen2D + Win. It only draws through the pen and reads
// input from the Win, so it never touches GL/Vulkan.
//   let ui: Ui.Gui; ui.init(pen, win);
//   loop: ui.begin(); ui.label("Hi"); if (ui.button("Go")) { ... } ui.end();
// Widgets stack vertically inside a panel (setPanel). Not compiled/run yet. No `return` in loop bodies.
module C = import "standard/1/Canvas.os";
module W = import "standard/1/Window.os";
module CMem = import "standard/1/c/mem.os";

let COL_BG: u32 = 0x1E1E24FF;
let COL_PANEL: u32 = 0x2A2A33FF;
let COL_IDLE: u32 = 0x3A3A46FF;
let COL_HOT: u32 = 0x4A4A5CFF;
let COL_ACTIVE: u32 = 0x5B7CFAFF;
let COL_TEXT: u32 = 0xE8E8F0FF;
let COL_DIM: u32 = 0x9A9AAAFF;
let COL_ACCENT: u32 = 0x5B7CFAFF;
let COL_BORDER: u32 = 0x55556AFF;

function hashStr(s: char*) => u32 {
    let h: u32 = 5381;
    let i: int = 0;
    let more: bool = true;
    while (more) {
        let c: int = s[i] as int;
        if (c == 0) { more = false; }
        else { h = h * 33 + (c as u32); i = i + 1; }
    }
    if (h == 0) h = 1;
    return h;
}

function strLen(s: char*) => int {
    let n: int = 0;
    while (s[n] as int != 0) { n = n + 1; }
    return n;
}

// Writes `v` with 2 decimals into out (>= 48 bytes; digits are staged at out[24..]).
function fmtFloat(v: float, out: char*) => void {
    let neg: bool = v < 0.0;
    let a: float = v;
    if (neg) a = 0.0 - v;
    let scaled: i64 = (a * 100.0 + 0.5) as i64;
    let w: i64 = scaled / 100;
    let frac: int = (scaled % 100) as int;
    let digits: int = 0;
    if (w == 0) { out[24] = 48 as char; digits = 1; }
    while (w > 0 && digits < 20) { out[24 + digits] = (48 + ((w % 10) as int)) as char; digits = digits + 1; w = w / 10; }
    let p: int = 0;
    if (neg) { out[p] = 45 as char; p = p + 1; }
    let k: int = digits - 1;
    while (k >= 0) { out[p] = out[24 + k]; p = p + 1; k = k - 1; }
    out[p] = 46 as char; p = p + 1;
    out[p] = (48 + frac / 10) as char; p = p + 1;
    out[p] = (48 + frac % 10) as char; p = p + 1;
    out[p] = 0 as char;
}

struct Gui {
    pen: C.Pen2D* = nullptr;
    win: C.Win* = nullptr;
    // panel + layout cursor
    px: float = 16.0; py: float = 16.0; pw: float = 280.0;
    cy: float = 0.0;
    pad: float = 8.0; gap: float = 6.0; rowH: float = 28.0;
    scale: float = 1.0;                 // text scale
    // input snapshot
    mx: float = 0.0; my: float = 0.0;
    down: bool = false; pressed: bool = false; released: bool = false;
    // widget state
    hot: u32 = 0; active: u32 = 0; focus: u32 = 0;
    wantsMouse: bool = false;           // true while the mouse is over/dragging any widget
    scratch: char* = nullptr;

    init(pen: C.Pen2D*, win: C.Win*) => void {
        this.pen = pen; this.win = win;
        this.scratch = CMem.malloc(48) as char*;
    }

    free() => void { if (this.scratch != nullptr) { CMem.free(this.scratch as void*); this.scratch = nullptr; } }

    setPanel(x: float, y: float, w: float) => void { this.px = x; this.py = y; this.pw = w; this.cy = y; }
    setScale(s: float) => void { this.scale = s; this.rowH = 14.0 + 14.0 * s; }

    begin() => void {
        this.mx = this.win.mx; this.my = this.win.my;
        this.down = this.win.mouseDown();
        this.pressed = this.win.mousePressed();
        this.released = this.win.mouseReleased();
        this.hot = 0;
        this.wantsMouse = false;
        this.cy = this.py;
        if (this.pressed) this.focus = 0;           // click anywhere clears text focus; textInput re-takes it
    }

    end() => void {
        if (!this.down && !this.released) this.active = 0;
        if (this.released) this.active = 0;
    }

    // Reserves a row; returns its y.
    nextRow(h: float) => float {
        let y: float = this.cy;
        this.cy = this.cy + h + this.gap;
        return y;
    }

    over(x: float, y: float, w: float, h: float) => bool {
        return this.mx >= x && this.mx < x + w && this.my >= y && this.my < y + h;
    }

    textY(y: float, h: float) => float { return y + (h - this.pen.textHeight(this.scale)) * 0.5; }

    // ---- widgets ----
    spacer(h: float) => void { this.cy = this.cy + h; }

    separator() => void {
        let y: float = this.nextRow(2.0);
        this.pen.fillRect(this.px, y, this.pw, 1.0, COL_BORDER);
    }

    header(text: char*) => void {
        let y: float = this.nextRow(this.rowH);
        this.pen.fillRect(this.px, y, this.pw, this.rowH, COL_PANEL);
        this.pen.fillRect(this.px, y, 3.0, this.rowH, COL_ACCENT);
        this.pen.textScaled(this.px + this.pad + 2.0, this.textY(y, this.rowH), text, COL_TEXT, this.scale);
    }

    label(text: char*) => void {
        let y: float = this.nextRow(this.rowH);
        this.pen.textScaled(this.px + 2.0, this.textY(y, this.rowH), text, COL_TEXT, this.scale);
    }

    dim(text: char*) => void {
        let y: float = this.nextRow(this.rowH);
        this.pen.textScaled(this.px + 2.0, this.textY(y, this.rowH), text, COL_DIM, this.scale);
    }

    button(text: char*) => bool {
        let y: float = this.nextRow(this.rowH);
        let id: u32 = hashStr(text);
        let isOver: bool = this.over(this.px, y, this.pw, this.rowH);
        if (isOver) { this.hot = id; this.wantsMouse = true; }
        if (isOver && this.pressed && this.active == 0) this.active = id;
        if (this.active == id) this.wantsMouse = true;
        let clicked: bool = isOver && this.released && this.active == id;
        let col: u32 = COL_IDLE;
        if (isOver) col = COL_HOT;
        if (this.active == id && isOver) col = COL_ACTIVE;
        this.pen.fillRect(this.px, y, this.pw, this.rowH, col);
        this.pen.rect(this.px, y, this.pw, this.rowH, 1.0, COL_BORDER);
        let tw: float = this.pen.textWidth(text, this.scale);
        this.pen.textScaled(this.px + (this.pw - tw) * 0.5, this.textY(y, this.rowH), text, COL_TEXT, this.scale);
        return clicked;
    }

    checkbox(text: char*, v: bool*) => bool {
        let y: float = this.nextRow(this.rowH);
        let id: u32 = hashStr(text);
        let box: float = this.rowH - 8.0;
        let isOver: bool = this.over(this.px, y, this.pw, this.rowH);
        if (isOver) { this.hot = id; this.wantsMouse = true; }
        let changed: bool = false;
        if (isOver && this.pressed) { *v = !(*v); changed = true; }
        let col: u32 = COL_IDLE;
        if (isOver) col = COL_HOT;
        this.pen.fillRect(this.px, y + 4.0, box, box, col);
        this.pen.rect(this.px, y + 4.0, box, box, 1.0, COL_BORDER);
        if (*v) this.pen.fillRect(this.px + 4.0, y + 8.0, box - 8.0, box - 8.0, COL_ACCENT);
        this.pen.textScaled(this.px + box + this.pad, this.textY(y, this.rowH), text, COL_TEXT, this.scale);
        return changed;
    }

    // Label on the left, value on the right, draggable bar. Returns true while the value changes.
    slider(text: char*, v: float*, lo: float, hi: float) => bool {
        let y: float = this.nextRow(this.rowH);
        let id: u32 = hashStr(text);
        let isOver: bool = this.over(this.px, y, this.pw, this.rowH);
        if (isOver) { this.hot = id; this.wantsMouse = true; }
        if (isOver && this.pressed && this.active == 0) this.active = id;
        let changed: bool = false;
        if (this.active == id) {
            this.wantsMouse = true;
            let t: float = (this.mx - this.px) / this.pw;
            if (t < 0.0) t = 0.0;
            if (t > 1.0) t = 1.0;
            let nv: float = lo + (hi - lo) * t;
            if (nv != *v) { *v = nv; changed = true; }
        }
        let frac: float = 0.0;
        if (hi > lo) frac = (*v - lo) / (hi - lo);
        if (frac < 0.0) frac = 0.0;
        if (frac > 1.0) frac = 1.0;
        let col: u32 = COL_IDLE;
        if (isOver || this.active == id) col = COL_HOT;
        this.pen.fillRect(this.px, y, this.pw, this.rowH, col);
        this.pen.fillRect(this.px, y, this.pw * frac, this.rowH, 0x5B7CFA99);
        this.pen.rect(this.px, y, this.pw, this.rowH, 1.0, COL_BORDER);
        let ty: float = this.textY(y, this.rowH);
        this.pen.textScaled(this.px + this.pad, ty, text, COL_TEXT, this.scale);
        fmtFloat(*v, this.scratch);
        let vw: float = this.pen.textWidth(this.scratch, this.scale);
        this.pen.textScaled(this.px + this.pw - vw - this.pad, ty, this.scratch, COL_TEXT, this.scale);
        return changed;
    }

    // Single-line text field editing buf (NUL-terminated, capacity cap incl. the NUL).
    // Returns true when the text changed this frame.
    textInput(text: char*, buf: char*, cap: u64) => bool {
        let y: float = this.nextRow(this.rowH);
        let id: u32 = hashStr(text) + 1;
        let isOver: bool = this.over(this.px, y, this.pw, this.rowH);
        if (isOver) { this.hot = id; this.wantsMouse = true; }
        if (isOver && this.pressed) this.focus = id;
        let changed: bool = false;
        if (this.focus == id) {
            let n: int = strLen(buf);
            let typed: char* = this.win.typedText();
            let i: int = 0;
            let more: bool = true;
            while (more) {
                let c: int = typed[i] as int;
                if (c == 0) { more = false; }
                else {
                    if ((n + 1) < (cap as int)) { buf[n] = c as char; n = n + 1; buf[n] = 0 as char; changed = true; }
                    i = i + 1;
                }
            }
            if (this.win.backspace() && n > 0) { n = n - 1; buf[n] = 0 as char; changed = true; }
        }
        let col: u32 = COL_PANEL;
        if (this.focus == id) col = COL_BG;
        this.pen.fillRect(this.px, y, this.pw, this.rowH, col);
        let bc: u32 = COL_BORDER;
        if (this.focus == id) bc = COL_ACCENT;
        this.pen.rect(this.px, y, this.pw, this.rowH, 1.0, bc);
        this.pen.pushClip((this.px + 2.0) as int, y as int, (this.pw - 4.0) as int, this.rowH as int);
        let tw: float = this.pen.textWidth(buf, this.scale);
        let tx: float = this.px + this.pad;
        let room: float = this.pw - 2.0 * this.pad;
        if (tw > room) tx = tx - (tw - room);
        this.pen.textScaled(tx, this.textY(y, this.rowH), buf, COL_TEXT, this.scale);
        if (this.focus == id) {
            this.pen.fillRect(tx + tw + 1.0, y + 5.0, 2.0, this.rowH - 10.0, COL_TEXT);
        }
        this.pen.popClip();
        return changed;
    }
}
