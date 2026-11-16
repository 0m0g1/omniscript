// canvas_test.os
// Canvas feature test. Opens a GL window; spawn more at runtime.
//
//   Left / Right   previous / next page
//   F2             open a SOFTWARE window   (CPU rasteriser, shown via GL blit)
//   F3             open a VULKAN window     (needs shaders/canvas2d.{vert,frag}.spv)
//   F4             open another GL window   (tests shared GL context / textures)
//   Esc            close the focused window (program ends when all are closed)
//
// Pages (every window has its own current page):
//   1 Shapes   fillRect, rect, line, circle, fillTriangle, alpha blending
//   2 Text     textScaled, textWidth, multi-line, alpha, charset
//   3 Clip     pushClip / popClip, nested clips
//   4 Images   createImage, image, imageTinted, imageFlipped
//   5 Shader   createShaderBoth + shaderRect (GL: GLSL; Vulkan: shaders/plasma.frag.spv; software: skipped)
//   6 3D       Pen3D: camera, light, lit cubes, textured cube (GL windows only)
//   7 Input    mouse, buttons, edges, modifiers, held key, typed text, backspace repeat
//   8 Stress   600 animated rects, FPS readout (one batched draw call)
module C = import "standard/1/Canvas.os";
module OGL = import "standard/1/OpenGL.os";
module CMem = import "standard/1/c/mem.os";

let PAGES: int = 8;

struct TestWin {
    win: C.Win* = nullptr;
    used: bool = false;
    ready: bool = false;
    api: int = 0;
    page: int = 0;
    img: u32 = 0;
    shader: u32 = 0;
    cubeOk: bool = false;
    cube: OGL.Mesh;
    m1: float* = nullptr; m2: float* = nullptr; m3: float* = nullptr;
    num: char* = nullptr;
    buf: char* = nullptr; blen: int = 0;
    clicks: int = 0; rclicks: int = 0;
    lastT: float = 0.0; fps: float = 0.0;
}

// ---------------------------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------------------------
function palette(i: int) => u32 {
    let k: int = i % 6;
    if (k == 0) return 0xFF5555FF;
    if (k == 1) return 0xFFAA33FF;
    if (k == 2) return 0xFFEE55FF;
    if (k == 3) return 0x55DD77FF;
    if (k == 4) return 0x55AAFFFF;
    return 0xBB77FFFF;
}

function itoa(buf: char*, v: int) => void {
    let n: int = v;
    if (n < 0) n = 0;
    if (n == 0) { buf[0] = 48 as char; buf[1] = 0 as char; return; }
    let len: int = 0;
    let t: int = n;
    while (t > 0) { len = len + 1; t = t / 10; }
    buf[len] = 0 as char;
    let i: int = len - 1;
    while (n > 0) {
        buf[i] = (48 + n % 10) as char;
        n = n / 10;
        i = i - 1;
    }
}

function labelInt(pen: C.Pen2D*, num: char*, x: float, y: float, label: char*, v: int, c: u32) => void {
    itoa(num, v);
    pen.text(x, y, label, c);
    pen.text(x + pen.textWidth(label, 1.0), y, num, c);
}

function onoff(b: bool, a: char*, c: char*) => char* {
    if (b) return a;
    return c;
}

function backendName(api: int) => char* {
    if (api == C.API_GL) return "OpenGL";
    if (api == C.API_VULKAN) return "Vulkan";
    return "Software";
}

function pageName(p: int) => char* {
    if (p == 0) return "1/8 Shapes";
    if (p == 1) return "2/8 Text";
    if (p == 2) return "3/8 Clip";
    if (p == 3) return "4/8 Images";
    if (p == 4) return "5/8 Shader";
    if (p == 5) return "6/8 3D";
    if (p == 6) return "7/8 Input";
    return "8/8 Stress";
}

// ---------------------------------------------------------------------------------------------
// per-window setup (runs on the window's first frame)
// ---------------------------------------------------------------------------------------------
function setupSlot(t: TestWin*) => void {
    let win: C.Win* = t.win;
    let pen: C.Pen2D* = win.context2d();

    t.num = CMem.malloc(16) as char*;
    t.buf = CMem.malloc(128) as char*;
    t.buf[0] = 0 as char; t.blen = 0;
    t.m1 = CMem.malloc(64) as float*;
    t.m2 = CMem.malloc(64) as float*;
    t.m3 = CMem.malloc(64) as float*;

    // 64x64 test image: red/green gradient with blue checker
    let px = CMem.malloc(64 * 64 * 4) as u8*;
    let y: int = 0;
    while (y < 64) {
        let x: int = 0;
        while (x < 64) {
            let o: int = (y * 64 + x) * 4;
            let chk: int = ((x / 8) + (y / 8)) % 2;
            px[o] = (x * 4) as u8;
            px[o + 1] = (y * 4) as u8;
            if (chk == 0) { px[o + 2] = 255 as u8; } else { px[o + 2] = 90 as u8; }
            px[o + 3] = 255 as u8;
            x = x + 1;
        }
        y = y + 1;
    }
    t.img = pen.createImage(px, 64, 64);
    CMem.free(px as void*);

    // plasma shader (GL: GLSL body; Vulkan: SPIR-V file; software: returns 0)
    // t.shader = pen.createShaderBoth(
    //     "void main() { vec2 p = vUV * 6.0; float v = sin(p.x + uTime) + sin(p.y * 1.3 + uTime * 1.2) + sin((p.x + p.y) * 0.7 + uTime); o = vec4(0.5 + 0.5 * sin(v), 0.5 + 0.5 * sin(v + 2.1), 0.5 + 0.5 * sin(v + 4.2), 1.0) * vCol; }",
    //     "shaders/plasma.frag.spv");

    // 3D (GL windows only)
    let sc: C.Pen3D* = win.context3d();
    if (sc.ready) {
        sc.createCube(&t.cube);
        t.cubeOk = true;
    }
    t.ready = true;
}

// ---------------------------------------------------------------------------------------------
// pages
// ---------------------------------------------------------------------------------------------
function pageShapes(pen: C.Pen2D*, t: TestWin*, w: float, h: float, tm: float) => void {
    // filled rects
    let i: int = 0;
    while (i < 6) {
        pen.fillRect(20.0 + (i as float) * 50.0, 44.0, 40.0, 40.0, palette(i));
        i = i + 1;
    }
    // outlines, thickness 1 / 2 / 4
    pen.rect(20.0, 100.0, 80.0, 50.0, 1.0, 0xFFFFFFFF);
    pen.rect(120.0, 100.0, 80.0, 50.0, 2.0, 0xFFFFFFFF);
    pen.rect(220.0, 100.0, 80.0, 50.0, 4.0, 0xFFFFFFFF);
    pen.text(20.0, 156.0, "rect t=1 / 2 / 4", 0xAAAAAAFF);

    // spokes (line) + overlapping alpha circles
    let cx: float = w * 0.72;
    let cy: float = 140.0;
    i = 0;
    while (i < 12) {
        let a: float = (i as float) * C.PI / 6.0 + tm;
        pen.line(cx, cy, cx + C.mcos(a) * 80.0, cy + C.msin(a) * 80.0, 2.0, palette(i));
        i = i + 1;
    }
    pen.circle(cx - 18.0, cy, 36.0, 0xFF000080);
    pen.circle(cx + 18.0, cy, 36.0, 0x0000FF80);
    pen.text(cx - 60.0, cy + 90.0, "line + alpha circles", 0xAAAAAAFF);

    // rotating triangles
    let tx: float = 110.0; let ty: float = 300.0;
    let k: int = 0;
    while (k < 3) {
        let a0: float = tm + (k as float) * 2.094;
        let a1: float = a0 + 2.094;
        let a2: float = a1 + 2.094;
        let r: float = 70.0 - (k as float) * 20.0;
        pen.fillTriangle(tx + C.mcos(a0) * r, ty + C.msin(a0) * r,
                         tx + C.mcos(a1) * r, ty + C.msin(a1) * r,
                         tx + C.mcos(a2) * r, ty + C.msin(a2) * r, palette(k + 3));
        k = k + 1;
    }
    pen.text(40.0, 380.0, "fillTriangle", 0xAAAAAAFF);

    // circle ring
    i = 0;
    while (i < 8) {
        pen.circle(cx - 100.0 + (i as float) * 28.0, 320.0, 6.0 + (i as float) * 2.0, palette(i));
        i = i + 1;
    }
    pen.text(cx - 100.0, 360.0, "circle radii", 0xAAAAAAFF);
}

function pageText(pen: C.Pen2D*, t: TestWin*, w: float, h: float, tm: float) => void {
    pen.textScaled(20.0, 40.0, "Scale 1.0", 0xFFFFFFFF, 1.0);
    pen.textScaled(20.0, 58.0, "Scale 2.0", 0xFFEE55FF, 2.0);
    pen.textScaled(20.0, 92.0, "Scale 3.0", 0x55DD77FF, 3.0);
    pen.textScaled(20.0, 140.0, "Scale 4.0", 0x55AAFFFF, 4.0);

    pen.text(300.0, 44.0, "multi-line\nsecond line\nthird line", 0xFFFFFFFF);

    let s: char* = "Centered with textWidth";
    let sw: float = pen.textWidth(s, 2.0);
    let sx: float = (w - sw) * 0.5;
    pen.textScaled(sx, 220.0, s, 0xFF5555FF, 2.0);
    pen.line(sx, 254.0, sx + sw, 254.0, 1.0, 0xFF5555FF);

    pen.text(20.0, 280.0, "Half transparent text", 0xFFFFFF80);

    pen.text(20.0, 320.0, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0xCCCCCCFF);
    pen.text(20.0, 340.0, "abcdefghijklmnopqrstuvwxyz", 0xCCCCCCFF);
    pen.text(20.0, 360.0, "0123456789 !#$%&'()*+,-./:;<=>?@[]^_`{|}~", 0xCCCCCCFF);
}

function pageClip(pen: C.Pen2D*, t: TestWin*, w: float, h: float, tm: float) => void {
    // panel 1: nested clips
    pen.pushClip(30, 50, 260, 160);
    pen.circle(160.0 + C.mcos(tm) * 200.0, 130.0, 90.0, 0x55AAFFFF);
    pen.textScaled(160.0 + C.msin(tm) * 200.0 - 100.0, 60.0, "This text is clipped by the panel", 0xFFFFFFFF, 2.0);
    pen.pushClip(80, 90, 120, 80);
    pen.fillRect(0.0, 0.0, w, h, 0xFF555590);
    pen.text(90.0, 120.0, "nested clip", 0xFFFFFFFF);
    pen.popClip();
    pen.popClip();
    pen.rect(30.0, 50.0, 260.0, 160.0, 1.0, 0x8892BBFF);
    pen.rect(80.0, 90.0, 120.0, 80.0, 1.0, 0xFF5555FF);

    // panel 2: rotating lines inside a clip
    let px: int = 320;
    pen.pushClip(px, 50, (w as int) - px - 30, 160);
    let i: int = 0;
    while (i < 10) {
        let a: float = (i as float) * 0.6 + tm;
        pen.line(450.0, 130.0, 450.0 + C.mcos(a) * 300.0, 130.0 + C.msin(a) * 300.0, 3.0, palette(i));
        i = i + 1;
    }
    pen.popClip();
    pen.rect((px as float), 50.0, w - (px as float) - 30.0, 160.0, 1.0, 0x8892BBFF);

    pen.text(30.0, 230.0, "After popClip: drawing is unclipped again", 0x55DD77FF);
    pen.circle(w - 60.0, 260.0, 30.0, 0x55DD77FF);
}

function pageImages(pen: C.Pen2D*, t: TestWin*, w: float, h: float, tm: float) => void {
    if (t.img == 0) { pen.text(20.0, 50.0, "createImage failed", 0xFF5555FF); return; }
    pen.text(20.0, 36.0, "1x / 2x / tinted red / green / half alpha", 0xAAAAAAFF);
    pen.image(t.img, 20.0, 56.0, 64.0, 64.0);
    pen.image(t.img, 100.0, 56.0, 128.0, 128.0);
    pen.imageTinted(t.img, 250.0, 56.0, 96.0, 96.0, 0xFF6666FF);
    pen.imageTinted(t.img, 360.0, 56.0, 96.0, 96.0, 0x66FF66FF);
    pen.imageTinted(t.img, 470.0, 56.0, 96.0, 96.0, 0xFFFFFF80);

    pen.text(20.0, 200.0, "imageFlipped (vertical flip) / moving, overlapping", 0xAAAAAAFF);
    pen.imageFlipped(t.img, 20.0, 222.0, 96.0, 96.0);
    pen.imageFlipped(t.img, 130.0, 222.0, 160.0, 96.0);
    let mx: float = 380.0 + C.mcos(tm) * 80.0;
    pen.image(t.img, mx, 230.0, 96.0, 96.0);
    pen.imageTinted(t.img, mx + 40.0, 250.0, 96.0, 96.0, 0xFFFFFFA0);
}

function pageShader(pen: C.Pen2D*, t: TestWin*, w: float, h: float, tm: float) => void {
    if (t.shader == 0) {
        pen.text(20.0, 50.0, "No custom shader on this backend.", 0xFFAA33FF);
        pen.text(20.0, 70.0, "GL: GLSL compile failed?  Vulkan: shaders/plasma.frag.spv missing?", 0xAAAAAAFF);
        pen.text(20.0, 90.0, "Software: shaderRect is skipped by design.", 0xAAAAAAFF);
        return;
    }
    pen.shaderRect(t.shader, 20.0, 44.0, w - 40.0, 180.0);
    pen.fillRect(60.0, 60.0, 180.0, 40.0, 0x000000A0);
    pen.text(70.0, 72.0, "drawn after shaderRect", 0xFFFFFFFF);
    pen.shaderRect(t.shader, w - 160.0, 250.0, 120.0, 120.0);
    pen.fillRect(w - 140.0, 290.0, 80.0, 30.0, 0x000000A0);
    pen.text(w - 134.0, 298.0, "on top", 0xFFFFFFFF);
    pen.circle(100.0, 300.0, 40.0, 0x55AAFFFF);
    pen.text(40.0, 360.0, "batch order: 2D / shader / 2D / shader / 2D", 0xAAAAAAFF);
}

function page3D(pen: C.Pen2D*, t: TestWin*, w: float, h: float, tm: float) => void {
    if (!t.cubeOk) {
        pen.text(20.0, 50.0, "Pen3D is available on OpenGL windows only.", 0xFFAA33FF);
        pen.text(20.0, 70.0, "(Vulkan: use win.vk directly for 3D.)", 0xAAAAAAFF);
        return;
    }
    let sc: C.Pen3D* = t.win.context3d();
    sc.camera(0.0, 1.5, 6.0, 0.0, 0.0, 0.0, 60.0);
    sc.light(0.4, 0.8, 0.5);

    // centre: lit, rotating on two axes
    C.matRotateY(t.m1, tm);
    C.matRotateX(t.m2, tm * 0.7);
    C.matMul(t.m3, t.m1, t.m2);
    sc.draw(&t.cube, t.m3, 0x55AAFFFF);

    // right: textured, spinning on Y
    C.matRotateY(t.m2, 0.0 - tm);
    C.matTranslate(t.m1, 2.6, 0.0, 0.0);
    C.matMul(t.m3, t.m1, t.m2);
    sc.drawTextured(&t.cube, t.m3, 0xFFFFFFFF, t.img);

    // left: scaled, orange, bobbing (tests depth + translate + scale)
    C.matScale(t.m2, 0.7, 0.7, 0.7);
    C.matTranslate(t.m1, 0.0 - 2.6, C.msin(tm * 2.0) * 0.5, 0.0);
    C.matMul(t.m3, t.m1, t.m2);
    sc.draw(&t.cube, t.m3, 0xFFAA33FF);

    pen.text(20.0, 40.0, "Pen3D: lit cube / textured cube / scaled cube. 2D overlay drawn on top.", 0xFFFFFFFF);
}

function pageInput(pen: C.Pen2D*, t: TestWin*, w: float, h: float, tm: float) => void {
    let win: C.Win* = t.win;
    if (win.mousePressed()) t.clicks = t.clicks + 1;
    if (win.rightPressed()) t.rclicks = t.rclicks + 1;

    // typed text + backspace repeat
    if (win.backspace() && t.blen > 0) t.blen = t.blen - 1;
    let s: char* = win.typedText();
    let i: int = 0;
    let more: bool = true;
    while (more) {
        let ch: int = s[i] as int;
        if (ch == 0) { more = false; }
        else {
            if (t.blen < 100) { t.buf[t.blen] = ch as char; t.blen = t.blen + 1; }
            i = i + 1;
        }
    }
    t.buf[t.blen] = 0 as char;

    // held key scan
    let held: int = 0;
    let k: int = 32;
    while (k <= 348) {
        if (win.keyDown(k)) held = k;
        k = k + 1;
    }

    labelInt(pen, t.num, 20.0, 44.0, "Mouse X: ", win.mx as int, 0xFFFFFFFF);
    labelInt(pen, t.num, 20.0, 62.0, "Mouse Y: ", win.my as int, 0xFFFFFFFF);
    pen.text(20.0, 86.0, onoff(win.mouseDown(), "Left:  DOWN", "Left:  up"), 0x55DD77FF);
    pen.text(20.0, 104.0, onoff(win.rightDown(), "Right: DOWN", "Right: up"), 0xFFAA33FF);
    labelInt(pen, t.num, 20.0, 128.0, "Left presses:  ", t.clicks, 0xCCCCCCFF);
    labelInt(pen, t.num, 20.0, 146.0, "Right presses: ", t.rclicks, 0xCCCCCCFF);
    pen.text(20.0, 170.0, onoff(win.shiftDown(), "Shift: DOWN", "Shift: up"), 0xCCCCCCFF);
    pen.text(20.0, 188.0, onoff(win.ctrlDown(), "Ctrl:  DOWN", "Ctrl:  up"), 0xCCCCCCFF);
    labelInt(pen, t.num, 20.0, 212.0, "Held key code: ", held, 0xCCCCCCFF);

    pen.text(20.0, 250.0, "Type here (backspace repeats):", 0xAAAAAAFF);
    pen.fillRect(20.0, 270.0, w - 40.0, 28.0, 0x1E2230FF);
    pen.rect(20.0, 270.0, w - 40.0, 28.0, 1.0, 0x8892BBFF);
    pen.pushClip(22, 272, (w as int) - 44, 24);
    pen.text(28.0, 278.0, t.buf, 0xFFFFFFFF);
    if (((tm * 2.0) as int) % 2 == 0) {
        pen.fillRect(28.0 + (t.blen * 8) as float, 276.0, 2.0, 16.0, 0xFFFFFFFF);
    }
    pen.popClip();

    let col: u32 = 0xFFFFFF99;
    if (win.mouseDown()) col = 0x55DD77FF;
    if (win.rightDown()) col = 0xFFAA33FF;
    pen.circle(win.mx, win.my, 12.0, col);
}

function pageStress(pen: C.Pen2D*, t: TestWin*, w: float, h: float, tm: float) => void {
    let cx: float = w * 0.5;
    let cy: float = h * 0.5;
    let i: int = 0;
    while (i < 600) {
        let a: float = (i as float) * 0.1 + tm * (0.4 + ((i % 7) as float) * 0.08);
        let r: float = ((i % 100) as float) * 2.4;
        pen.fillRect(cx + C.mcos(a) * r, cy + C.msin(a) * r, 6.0, 6.0, palette(i));
        i = i + 1;
    }
    pen.text(20.0, 40.0, "600 rects, one texture atlas, one draw call", 0xFFFFFFFF);
}

// ---------------------------------------------------------------------------------------------
// one window, one frame. Returns a spawn request key (291/292/293) or 0.
// ---------------------------------------------------------------------------------------------
function runSlot(t: TestWin*, now: float, status: char*) => int {
    if (!t.used) return 0;
    if (!t.win.isOpen) return 0;
    let win: C.Win* = t.win;
    if (!win.active) return 0;                  // minimised
    if (!t.ready) setupSlot(t);

    let pen: C.Pen2D* = win.context2d();
    let w: float = win.width() as float;
    let h: float = win.height() as float;

    let dt: float = now - t.lastT;
    t.lastT = now;
    if (dt > 0.0) t.fps = t.fps * 0.9 + (1.0 / dt) * 0.1;

    if (win.keyPressed(256)) win.close();
    if (win.keyPressed(262)) t.page = (t.page + 1) % PAGES;
    if (win.keyPressed(263)) t.page = (t.page + PAGES - 1) % PAGES;

    // clear: the 3D page clears colour + depth through Pen3D
    if (t.page == 5 && t.cubeOk) {
        let sc: C.Pen3D* = win.context3d();
        sc.clear(0.06, 0.08, 0.12, 1.0);
    } else {
        pen.clear(0.08, 0.09, 0.12, 1.0);
    }

    if (t.page == 0) pageShapes(pen, t, w, h, now);
    if (t.page == 1) pageText(pen, t, w, h, now);
    if (t.page == 2) pageClip(pen, t, w, h, now);
    if (t.page == 3) pageImages(pen, t, w, h, now);
    if (t.page == 4) pageShader(pen, t, w, h, now);
    if (t.page == 5) page3D(pen, t, w, h, now);
    if (t.page == 6) pageInput(pen, t, w, h, now);
    if (t.page == 7) pageStress(pen, t, w, h, now);

    // header
    pen.fillRect(0.0, 0.0, w, 28.0, 0x1E2230FF);
    pen.text(10.0, 7.0, backendName(t.api), 0x55AAFFFF);
    pen.text(110.0, 7.0, pageName(t.page), 0xFFEE55FF);
    labelInt(pen, t.num, w - 90.0, 7.0, "FPS ", t.fps as int, 0x55DD77FF);

    // footer
    pen.fillRect(0.0, h - 40.0, w, 40.0, 0x1E2230FF);
    pen.text(10.0, h - 36.0, "Left/Right: page | F2 software | F3 vulkan | F4 GL | Esc: close", 0xAAAAAAFF);
    pen.text(10.0, h - 18.0, status, 0xFFEE55FF);

    let req: int = 0;
    if (win.keyPressed(291)) req = 291;
    if (win.keyPressed(292)) req = 292;
    if (win.keyPressed(293)) req = 293;
    return req;
}

// ---------------------------------------------------------------------------------------------
// window slots
// ---------------------------------------------------------------------------------------------
function slotFree(t: TestWin*) => bool {
    if (!t.used) return true;
    if (t.win.isOpen) return false;
    return true;
}

function spawnInto(app: C.Canvas*, t: TestWin*, api: int, title: char*) => bool {
    let win: C.Win* = app.createWindowEx(title, 640, 480, api);
    if (win == nullptr) return false;
    CMem.memset(t as void*, 0, sizeof(TestWin));
    t.win = win;
    t.api = win.api;
    t.used = true;
    return true;
}

function spawnAny(app: C.Canvas*, a: TestWin*, b: TestWin*, c: TestWin*, d: TestWin*, api: int, title: char*) => bool {
    if (slotFree(a)) return spawnInto(app, a, api, title);
    if (slotFree(b)) return spawnInto(app, b, api, title);
    if (slotFree(c)) return spawnInto(app, c, api, title);
    if (slotFree(d)) return spawnInto(app, d, api, title);
    return false;
}

// Slots live on the heap, allocated exactly like Canvas allocates a Win (malloc(sizeof) + memset).
function newSlot() => TestWin* {
    let t = CMem.malloc(sizeof(TestWin)) as TestWin*;
    CMem.memset(t as void*, 0, sizeof(TestWin));
    return t;
}

function main() => int {
    let app: C.Canvas;
    if (!app.init()) return 1;

    let t0: TestWin* = newSlot();
    let t1: TestWin* = newSlot();
    let t2: TestWin* = newSlot();
    let t3: TestWin* = newSlot();

    if (!spawnInto(&app, t0, C.API_GL, "Canvas test - OpenGL")) { app.quit(); return 2; }

    let status: char* = "Ready.";

    while (app.frame()) {
        let now: float = app.time() as float;
        let r0: int = runSlot(t0, now, status);
        let r1: int = runSlot(t1, now, status);
        let r2: int = runSlot(t2, now, status);
        let r3: int = runSlot(t3, now, status);
        app.present();

        let req: int = r0;
        if (r1 != 0) req = r1;
        if (r2 != 0) req = r2;
        if (r3 != 0) req = r3;

        if (req == 291) {
            if (spawnAny(&app, t0, t1, t2, t3, C.API_SOFTWARE, "Canvas test - Software")) status = "Opened software window.";
            else status = "Software window failed (window limit or no free slot).";
        }
        if (req == 292) {
            if (spawnAny(&app, t0, t1, t2, t3, C.API_VULKAN, "Canvas test - Vulkan")) status = "Opened Vulkan window.";
            else status = "Vulkan window failed (driver, shaders/*.spv, or window limit).";
        }
        if (req == 293) {
            if (spawnAny(&app, t0, t1, t2, t3, C.API_GL, "Canvas test - OpenGL 2")) status = "Opened second GL window.";
            else status = "GL window failed (window limit or no free slot).";
        }
    }

    app.quit();
    return 0;
}
