// standard/1/Ui/Backend/Gui.os
// Display list -> Pen2D. Uses only fillRect, fillRoundRect, strokeRoundRect, textScaled, pushClip/popClip and image. Called every frame
// (Pen2D clears each frame); the display list itself is rebuilt only when something is dirty.
// Text is drawn with the 8x16 bitmap font scaled by fontPx / cellH. Bold = drawn twice 1px apart, italic is ignored
// (NEXT: Font variants), underline / strike are drawn as lines. Widgets (buttons, inputs...) are drawn by Pen2D
// after this, on top.

struct GuiBackend {
    pen: C.Pen2D* = nullptr;
    buf: char* = nullptr; cap: int = 0;
    clipDepth: int = 0;

    init(pen: C.Pen2D*) => void {
        this.pen = pen;
        this.cap = 512;
        this.buf = CMem.malloc(512) as char*;
        this.clipDepth = 0;
    }

    ensure(n: int) => void {
        if (n + 1 > this.cap) {
            CMem.free(this.buf as void*);
            this.cap = n + 64;
            this.buf = CMem.malloc(this.cap as u64) as char*;
        }
    }

    rect(x: float, y: float, w: float, h: float, color: u32) => void {
        if (w > 0.0 && h > 0.0 && (color & 0xFF) != 0) this.pen.fillRect(x, y, w, h, color);
    }

    rrect(x: float, y: float, w: float, h: float, r: float, color: u32) => void {
        if (w > 0.0 && h > 0.0 && (color & 0xFF) != 0) this.pen.fillRoundRect(x, y, w, h, r, color);
    }

    run(dl: DisplayList*, m: Metrics*) => void {
        let i: int = 0;
        while (i < dl.n) {
            let c: Cmd* = &dl.cmds[i];
            if (c.op == CM_FILL) {
                if (c.radius > 0.0) this.rrect(c.x, c.y, c.w, c.h, c.radius, c.color);
                else this.rect(c.x, c.y, c.w, c.h, c.color);
            }
            else if (c.op == CM_BORDER && c.radius > 0.0 && c.bt == c.br && c.bt == c.bb && c.bt == c.bl) {
                // uniform border on a rounded box: a ring (per-side widths with a radius fall back to the square border below)
                if (c.bt > 0.0 && (c.color & 0xFF) != 0) this.pen.strokeRoundRect(c.x, c.y, c.w, c.h, c.radius, c.bt, c.color);
            }
            else if (c.op == CM_BORDER) {
                this.rect(c.x, c.y, c.w, c.bt, c.color);
                this.rect(c.x, c.y + c.h - c.bb, c.w, c.bb, c.color);
                this.rect(c.x, c.y + c.bt, c.bl, c.h - c.bt - c.bb, c.color);
                this.rect(c.x + c.w - c.br, c.y + c.bt, c.br, c.h - c.bt - c.bb, c.color);
            }
            else if (c.op == CM_TEXT) {
                this.ensure(c.tlen);
                let k: int = 0;
                while (k < c.tlen) { this.buf[k] = c.text[c.tfrom + k]; k = k + 1; }
                this.buf[c.tlen] = 0 as char;
                let scale: float = c.size / m.cellH;
                this.pen.textScaled(c.x, c.y, this.buf, c.color, scale);
                if ((c.flags & TF_BOLD) != 0) this.pen.textScaled(c.x + 1.0, c.y, this.buf, c.color, scale);
                let w: float = this.pen.textWidth(this.buf, scale);
                let th: float = 1.0;
                if (c.size >= 24.0) th = 2.0;
                if ((c.flags & TF_UNDERLINE) != 0) this.rect(c.x, c.y + c.size * 0.9, w, th, c.color);
                if ((c.flags & TF_STRIKE) != 0) this.rect(c.x, c.y + c.size * 0.5, w, th, c.color);
            }
            else if (c.op == CM_MARK) {
                this.ensure(16);
                let len: int = markerText(this.buf, c.tfrom, c.tlen);
                let scale2: float = c.size / m.cellH;
                let w2: float = this.pen.textWidth(this.buf, scale2);
                this.pen.textScaled(c.x - w2 - m.cellW * 0.5, c.y, this.buf, c.color, scale2);
            }
            else if (c.op == CM_PUSHCLIP) {
                this.pen.pushClip(c.x as int, c.y as int, c.w as int, c.h as int);
            }
            else if (c.op == CM_POPCLIP) {
                this.pen.popClip();
            }
            else if (c.op == CM_IMAGE) {
                this.pen.image(c.tfrom, c.x, c.y, c.w, c.h);
            }
            i = i + 1;
        }
    }
}
