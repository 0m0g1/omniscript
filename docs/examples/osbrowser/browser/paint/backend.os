// browser/paint/backend.os  -- TIER 5: executes a display list on your Canvas Pen2D.
// The page is drawn inside the rectangle (0, top, viewW, viewH): offset by `top`, scrolled by scrollY, clipped.
// NEXT: image cmds, damage rects, glyph-accurate text via Font.
module C = import "standard/1/Canvas.os";
module P = import "browser/paint/paint.os";

let BASE_PX: float = 16.0;       // ADAPT: pixel height of pen.text() at scale 1.0

function render(pen: C.Pen2D*, list: P.Cmd*, scrollY: float, top: float, viewW: float, viewH: float) => void {
    pen.pushClip(0, top as int, viewW as int, viewH as int);     // ADAPT if pushClip takes different args
    let c: P.Cmd* = list;
    while (c != nullptr) {
        let y: float = c.y - scrollY + top;
        if (c.kind == 0) pen.fillRect(c.x, y, c.w, c.h, c.color);
        else pen.textScaled(c.x, y, c.text, c.color, c.size / BASE_PX);
        c = c.next;
    }
    pen.popClip();
}
