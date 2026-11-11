// standard/1/Canvas.os  -- STUB (not implemented yet)
// Tier 7: 2D drawing API (rects, lines, circles, text, images) batching to OpenGL; software fallback on Image.
// All functions return false / do nothing until implemented.
struct Canvas {
    init(w: int, h: int) => bool { return false; }                // TODO: GL batch renderer (see OpenGL.Mesh) + font atlas
    clear(r: float, g: float, b: float, a: float) => void { }
    fillRect(x: float, y: float, w: float, h: float, rgba: u32) => void { }
    line(x1: float, y1: float, x2: float, y2: float, rgba: u32) => void { }
    circle(cx: float, cy: float, r: float, rgba: u32) => void { }
    text(x: float, y: float, s: char*, rgba: u32) => void { }
    flush() => void { }
}
