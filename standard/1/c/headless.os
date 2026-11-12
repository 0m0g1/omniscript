// examples/app/headless.os -- Pen2D with no Canvas and no window: the software backend renders into memory.
// Returns 0 when the centre pixel of the rect is the expected blue. Read pen.pixels() (RGBA8) to save or upload it.
module C = import "standard/1/Canvas.os";

function main() => int {
    let pen: C.Pen2D;
    pen.initSoftware(320, 200);
    pen.begin(320, 200);
    pen.clear(0.0, 0.0, 0.0, 1.0);
    pen.fillRect(40.0, 40.0, 100.0, 60.0, 0x3366FFFF);
    pen.text(48.0, 48.0, "headless", 0xFFFFFFFF);
    pen.flush();
    let px: u8* = pen.pixels();
    let i: int = (70 * 320 + 120) * 4;          // inside the rect, right of the text
    let blue: bool = (px[i] as int) == 51 && (px[i + 1] as int) == 102 && (px[i + 2] as int) == 255;
    pen.free();
    if (blue) return 0;
    return 1;
}
