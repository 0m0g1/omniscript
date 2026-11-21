// standard/1/Ui/Measure.os
// One place that knows glyph sizes. GUI: backed by pen.textWidth (8x16 bitmap font scaled by size / cellH).
// TUI / headless without a pen: Metrics.cellW per glyph. Later: Font.glyph().advance for proportional fonts,
// which only changes advance() and textWidth(); layout asks for widths through here.

struct Measure {
    pen: C.Pen2D* = nullptr;
    m: Metrics* = nullptr;

    // width of one glyph at font size px (layout units)
    advance(px: float) => float {
        let scale: float = px / this.m.cellH;
        if (this.pen != nullptr) return this.pen.textWidth("M", scale);
        return this.m.cellW * scale;
    }

    // width of n glyphs of a monospace run
    runWidth(n: int, px: float) => float { return (n as float) * this.advance(px); }

    textWidth(s: char*, px: float) => float {
        let scale: float = px / this.m.cellH;
        if (this.pen != nullptr) return this.pen.textWidth(s, scale);
        return (strLen(s) as float) * this.m.cellW * scale;
    }

    // line box height for a font size with line-height: normal
    lineHeight(px: float) => float { return px * this.m.lineH / this.m.cellH; }
}
