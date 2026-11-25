// standard/1/Ui/Controls/Popup.os
// <select> dropdown list: state, geometry and row logic. One popup can be open at a time; Events owns the instance (events.pop).
// This file draws nothing. Painter paints it after the whole tree (see "Painter contract" at the bottom).
// Coordinates are screen coordinates (scroll offsets of clipping ancestors already applied), like the mouse position.

let POP_MAXROWS: int = 10;

struct Popup {
    owner: Node* = nullptr;            // the open <select> (nullptr = closed)
    hover: int = 0;                    // highlighted row (mouse or keyboard); -1 = none
    top: int = 0;                      // first visible row
    count: int = 0;                    // number of options
    rows: int = 0;                     // visible rows (<= count)
    x: float = 0.0; y: float = 0.0; w: float = 0.0; h: float = 0.0;
    rowH: float = 0.0;
    ax: float = 0.0; ay: float = 0.0;  // where the select was when the popup opened (moves => close)
    lastX: float = 0.0; lastY: float = 0.0;
    grab: bool = false;                // the mouse went down inside the popup

    reset() => void {
        this.owner = nullptr; this.hover = 0 - 1; this.top = 0; this.count = 0; this.rows = 0;
        this.x = 0.0; this.y = 0.0; this.w = 0.0; this.h = 0.0; this.rowH = 0.0;
        this.ax = 0.0; this.ay = 0.0; this.lastX = 0.0; this.lastY = 0.0; this.grab = false;
    }

    isOpen() => bool { return this.owner != nullptr; }

    // viewW/viewH: window size in pixels (0 = unknown: no flipping or clamping). mx/my: current mouse position.
    // Opens below the select, or above when there is more room there.
    open(sel: Node*, viewW: float, viewH: float, mx: float, my: float) => void {
        this.owner = sel;
        this.count = selectCount(sel);
        this.rowH = sel.st.fontPx + 8.0;
        let bx: float = sel.box.x + scrollOffX(sel);
        let by: float = sel.box.y + scrollOffY(sel);
        this.ax = bx; this.ay = by;
        let vh: float = viewH; if (vh <= 0.0) vh = 100000.0;
        let below: float = vh - (by + sel.box.h) - 4.0;
        let above: float = by - 4.0;
        let down: bool = true; let space: float = below;
        if ((this.count as float) * this.rowH + 2.0 > below && above > below) { down = false; space = above; }
        let fit: int = ((space - 2.0) / this.rowH) as int;
        if (fit < 1) fit = 1;
        let r: int = this.count;
        if (r > POP_MAXROWS) r = POP_MAXROWS;
        if (r > fit) r = fit;
        if (r < 1) r = 1;
        this.rows = r;
        this.w = sel.box.w;
        this.h = (r as float) * this.rowH + 2.0;
        this.x = bx;
        if (viewW > 0.0 && this.x + this.w > viewW) this.x = maxF(0.0, viewW - this.w);
        if (down) this.y = by + sel.box.h; else this.y = by - this.h;
        this.top = 0;
        this.hover = sel.selIndex;
        if (this.hover < 0 || this.hover >= this.count) this.hover = 0 - 1;
        this.reveal(this.hover);
        this.lastX = mx; this.lastY = my; this.grab = false;
    }

    close() => void { this.reset(); }

    contains(mx: float, my: float) => bool {
        if (this.owner == nullptr) return false;
        return mx >= this.x && mx < this.x + this.w && my >= this.y && my < this.y + this.h;
    }

    // option index under (mx, my), or -1
    rowAt(mx: float, my: float) => int {
        if (!this.contains(mx, my)) return 0 - 1;
        let i: int = this.top + ((my - this.y - 1.0) / this.rowH) as int;
        if (my - this.y - 1.0 < 0.0) return 0 - 1;
        if (i >= this.top + this.rows || i >= this.count) return 0 - 1;
        return i;
    }

    // top-left y of the row for option i (valid only for visible rows)
    rowY(i: int) => float { return this.y + 1.0 + ((i - this.top) as float) * this.rowH; }
    isVisible(i: int) => bool { return i >= this.top && i < this.top + this.rows; }

    disabledAt(i: int) => bool {
        if (this.owner == nullptr) return true;
        return selOptionDisabled(this.owner, i);
    }

    reveal(i: int) => void {
        if (i < 0) return;
        if (i < this.top) this.top = i;
        else if (i >= this.top + this.rows) this.top = i - this.rows + 1;
        this.clampTop();
        this.touch();
    }

    clampTop() => void {
        let m: int = this.count - this.rows;
        if (m < 0) m = 0;
        if (this.top > m) this.top = m;
        if (this.top < 0) this.top = 0;
    }

    // wheel: dy > 0 moves the content up (shows later rows), in pixels
    scroll(dy: float) => void {
        let step: int = (dy / this.rowH) as int;
        if (step == 0) { if (dy > 0.0) step = 1; else if (dy < 0.0) step = 0 - 1; }
        this.top = this.top + step;
        this.clampTop();
        this.touch();
    }

    // Painter only repaints when the tree is dirty, so every visible change marks the select
    touch() => void { if (this.owner != nullptr) this.owner.markPaint(); }
    setHover(i: int) => void { if (i != this.hover) { this.hover = i; this.touch(); } }

    // scrollbar thumb for the painter (only when count > rows): top offset and length inside the popup, in pixels
    hasThumb() => bool { return this.count > this.rows; }
    thumbLen() => float {
        let t: float = (this.h - 2.0) * (this.rows as float) / (this.count as float);
        return maxF(t, 12.0);
    }
    thumbPos() => float {
        let span: float = (this.h - 2.0) - this.thumbLen();
        let m: int = this.count - this.rows;
        if (m <= 0) return 0.0;
        return 1.0 + span * (this.top as float) / (m as float);
    }
}

// Painter contract (Painter.os, after the normal tree pass and after every CM_POPCLIP, so nothing clips or covers it):
//   if (ev.pop.isOpen()) {
//       fill/border the rect (pop.x, pop.y, pop.w, pop.h);
//       let i = pop.top;
//       while (i < pop.top + pop.rows && i < pop.count) {
//           let ry = pop.rowY(i); row rect = (pop.x + 1, ry, pop.w - 2 (minus thumb width if pop.hasThumb()), pop.rowH);
//           highlight when i == pop.hover and !pop.disabledAt(i); dim text when pop.disabledAt(i); mark i == pop.owner.selIndex;
//           label = slice selLabelBase(owner, i) [selSpan(owner, i, 0), +selSpan(owner, i, 1)] (no allocation);
//           i = i + 1;
//       }
//       if (pop.hasThumb()) draw a thumb at (pop.x + pop.w - sbBarW() - 1, pop.y + pop.thumbPos(), sbBarW(), pop.thumbLen());
//   }
// Pen2D widgets draw on top of the display list: after ctlGeom, call ctlHideUnder(root, pop.owner, pop.x, pop.y, pop.w, pop.h)
// so no widget shows through the popup.
