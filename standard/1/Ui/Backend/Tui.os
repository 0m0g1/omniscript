// standard/1/Ui/Backend/Tui.os
// Display list -> character grid (char + fg + bg + attributes per cell) -> one ANSI string.
// Layout units are cells (Metrics: cellW = cellH = lineH = 1, unit 0.125), so the same layout code runs unchanged.
//   tui.init(cols, rows);  ui frame -> tui.run(dl, metrics);  let s: char* = tui.frameText();  then print s.
// frameText() returns the whole screen (cursor-home + 24-bit colour escapes). Printing it is left to the app because
// Console.os was not part of the uploaded files; the backend itself has no dependency on it.
// Borders become + - |, fills become background colour, text goes cell by cell, bold/underline become ANSI attributes,
// list markers use markerText(). Images are skipped. Widgets are drawn by Paint as plain text (see Painter.paintControl).
// NEXT: keyboard input -> Ui events (Tab order, Enter, arrows), mouse reporting, cursor placement for focused inputs.

struct TuiBackend {
    cols: int = 0; rows: int = 0;
    ch: char* = nullptr; fg: u32* = nullptr; bg: u32* = nullptr; at: char* = nullptr;
    out: char* = nullptr; outCap: int = 0; outLen: int = 0;
    clipStack: int* = nullptr; depth: int = 0;
    cx0: int = 0; cy0: int = 0; cx1: int = 0; cy1: int = 0;
    mk: char* = nullptr;

    init(cols: int, rows: int) => void {
        this.cols = cols; this.rows = rows;
        let n: int = cols * rows;
        this.ch = CMem.malloc(n as u64) as char*;
        this.at = CMem.malloc(n as u64) as char*;
        this.fg = CMem.malloc((n as u64) * 4) as u32*;
        this.bg = CMem.malloc((n as u64) * 4) as u32*;
        this.outCap = n * 48 + 64;
        this.out = CMem.malloc(this.outCap as u64) as char*;
        this.clipStack = CMem.malloc(16 * 4 * 4) as int*;
        this.mk = CMem.malloc(32) as char*;
        this.clear(DEF_BG);
    }

    clear(bgColor: u32) => void {
        let n: int = this.cols * this.rows; let i: int = 0;
        while (i < n) { this.ch[i] = 32 as char; this.at[i] = 0 as char; this.fg[i] = DEF_TEXT; this.bg[i] = bgColor; i = i + 1; }
        this.depth = 0; this.cx0 = 0; this.cy0 = 0; this.cx1 = this.cols; this.cy1 = this.rows;
    }

    cell(col: int, row: int, c: int, fgc: u32, attr: int) => void {
        if (col >= this.cx0 && col < this.cx1 && row >= this.cy0 && row < this.cy1) {
            let i: int = row * this.cols + col;
            this.ch[i] = c as char; this.fg[i] = fgc; this.at[i] = attr as char;
        }
    }

    fillCells(x0: int, y0: int, x1: int, y1: int, color: u32) => void {
        let r: int = maxI(y0, this.cy0);
        let rEnd: int = minI(y1, this.cy1);
        while (r < rEnd) {
            let c: int = maxI(x0, this.cx0);
            let cEnd: int = minI(x1, this.cx1);
            while (c < cEnd) {
                let i: int = r * this.cols + c;
                this.bg[i] = color; this.ch[i] = 32 as char; this.at[i] = 0 as char;
                c = c + 1;
            }
            r = r + 1;
        }
    }

    run(dl: DisplayList*, m: Metrics*) => void {
        let i: int = 0;
        while (i < dl.n) {
            let c: Cmd* = &dl.cmds[i];
            let x0: int = (c.x + 0.5) as int; let y0: int = (c.y + 0.5) as int;
            let x1: int = ((c.x + c.w) + 0.5) as int; let y1: int = ((c.y + c.h) + 0.5) as int;
            if (c.op == CM_FILL) {
                if (x1 <= x0) x1 = x0 + 1;
                if (y1 <= y0) y1 = y0 + 1;
                this.fillCells(x0, y0, x1, y1, c.color);
            }
            else if (c.op == CM_BORDER) {
                if (x1 <= x0) x1 = x0 + 1;
                if (y1 <= y0) y1 = y0 + 1;
                let k: int = x0;
                while (k < x1) {
                    if (c.bt > 0.0) this.cell(k, y0, 45, c.color, 0);
                    if (c.bb > 0.0) this.cell(k, y1 - 1, 45, c.color, 0);
                    k = k + 1;
                }
                k = y0;
                while (k < y1) {
                    if (c.bl > 0.0) this.cell(x0, k, 124, c.color, 0);
                    if (c.br > 0.0) this.cell(x1 - 1, k, 124, c.color, 0);
                    k = k + 1;
                }
                if (c.bt > 0.0 && c.bl > 0.0) this.cell(x0, y0, 43, c.color, 0);
                if (c.bt > 0.0 && c.br > 0.0) this.cell(x1 - 1, y0, 43, c.color, 0);
                if (c.bb > 0.0 && c.bl > 0.0) this.cell(x0, y1 - 1, 43, c.color, 0);
                if (c.bb > 0.0 && c.br > 0.0) this.cell(x1 - 1, y1 - 1, 43, c.color, 0);
            }
            else if (c.op == CM_TEXT) {
                let attr: int = 0;
                if ((c.flags & TF_BOLD) != 0) attr = attr | 1;
                if ((c.flags & TF_UNDERLINE) != 0) attr = attr | 2;
                if ((c.flags & TF_STRIKE) != 0) attr = attr | 4;
                let k2: int = 0;
                while (k2 < c.tlen) { this.cell(x0 + k2, y0, c.text[c.tfrom + k2] as int, c.color, attr); k2 = k2 + 1; }
            }
            else if (c.op == CM_MARK) {
                let len: int = markerText(this.mk, c.tfrom, c.tlen);
                let k3: int = 0;
                while (k3 < len) { this.cell(x0 - len - 1 + k3, y0, this.mk[k3] as int, c.color, 0); k3 = k3 + 1; }
            }
            else if (c.op == CM_PUSHCLIP) {
                if (this.depth < 16) {
                    let b: int = this.depth * 4;
                    this.clipStack[b] = this.cx0; this.clipStack[b + 1] = this.cy0;
                    this.clipStack[b + 2] = this.cx1; this.clipStack[b + 3] = this.cy1;
                    this.depth = this.depth + 1;
                    this.cx0 = maxI(this.cx0, x0); this.cy0 = maxI(this.cy0, y0);
                    this.cx1 = minI(this.cx1, x1); this.cy1 = minI(this.cy1, y1);
                }
            }
            else if (c.op == CM_POPCLIP) {
                if (this.depth > 0) {
                    this.depth = this.depth - 1;
                    let b2: int = this.depth * 4;
                    this.cx0 = this.clipStack[b2]; this.cy0 = this.clipStack[b2 + 1];
                    this.cx1 = this.clipStack[b2 + 2]; this.cy1 = this.clipStack[b2 + 3];
                }
            }
            i = i + 1;
        }
    }

    // ---- ANSI output ----
    putc(c: int) => void {
        if (this.outLen < this.outCap - 1) { this.out[this.outLen] = c as char; this.outLen = this.outLen + 1; }
    }
    puts(s: char*) => void {
        let i: int = 0;
        while (s[i] as int != 0) { this.putc(s[i] as int); i = i + 1; }
    }
    putInt(v: int) => void {
        if (v >= 100) this.putc(48 + (v / 100) % 10);
        if (v >= 10) this.putc(48 + (v / 10) % 10);
        this.putc(48 + v % 10);
    }
    sgr(fgc: u32, bgc: u32, attr: int) => void {
        this.putc(27); this.puts("[0");
        if ((attr & 1) != 0) this.puts(";1");
        if ((attr & 2) != 0) this.puts(";4");
        if ((attr & 4) != 0) this.puts(";9");
        this.puts(";38;2;"); this.putInt(((fgc >> 24) & 255) as int); this.putc(59);
        this.putInt(((fgc >> 16) & 255) as int); this.putc(59); this.putInt(((fgc >> 8) & 255) as int);
        this.puts(";48;2;"); this.putInt(((bgc >> 24) & 255) as int); this.putc(59);
        this.putInt(((bgc >> 16) & 255) as int); this.putc(59); this.putInt(((bgc >> 8) & 255) as int);
        this.putc(109);
    }

    // the whole screen as one string (valid until the next call)
    frameText() => char* {
        this.outLen = 0;
        this.putc(27); this.puts("[H");
        let lf: u32 = 0; let lb: u32 = 0; let la: int = 0 - 1;
        let r: int = 0;
        while (r < this.rows) {
            let c: int = 0;
            while (c < this.cols) {
                let i: int = r * this.cols + c;
                let a: int = this.at[i] as int;
                if (this.fg[i] != lf || this.bg[i] != lb || a != la) {
                    this.sgr(this.fg[i], this.bg[i], a);
                    lf = this.fg[i]; lb = this.bg[i]; la = a;
                }
                let ch: int = this.ch[i] as int;
                if (ch < 32 || ch > 126) ch = 32;
                this.putc(ch);
                c = c + 1;
            }
            this.putc(27); this.puts("[0m");
            if (r + 1 < this.rows) this.putc(10);
            la = 0 - 1;
            r = r + 1;
        }
        this.out[this.outLen] = 0 as char;
        return this.out;
    }

    // plain text dump of the grid (no colours), handy for tests
    plainText() => char* {
        this.outLen = 0;
        let r: int = 0;
        while (r < this.rows) {
            let c: int = 0;
            while (c < this.cols) {
                let ch: int = this.ch[r * this.cols + c] as int;
                if (ch < 32 || ch > 126) ch = 32;
                this.putc(ch);
                c = c + 1;
            }
            this.putc(10);
            r = r + 1;
        }
        this.out[this.outLen] = 0 as char;
        return this.out;
    }

    destroy() => void {
        CMem.free(this.ch as void*); CMem.free(this.at as void*); CMem.free(this.fg as void*); CMem.free(this.bg as void*);
        CMem.free(this.out as void*); CMem.free(this.clipStack as void*); CMem.free(this.mk as void*);
    }
}
