// docs/examples/city.os
// 2D + 3D on one canvas: a rippling neon city of lit cubes (Pen3D) with a floating core and orbiting
// satellites, plus a 2D HUD on top (Pen2D): decoding title with a left-to-right scramble wave, radar, brackets.
// Needs an OpenGL window (Pen3D is GL only). Esc = quit, R = replay intro,
// click = shockwave, move the mouse = orbit the camera.

module C = import "standard/1/Canvas.os";
module OGL = import "standard/1/OpenGL.os";
module T = import "standard/1/Time.os";
module CMem = import "standard/1/c/mem.os";

let TARGET_N: int = 10;
let KEEP: int = 5;
let POOL_N: int = 28;
let GRID: int = 21;              // GRID x GRID cubes
let SPACING: float = 1.25;

let SETTLE: float = 2.7;
let PERIOD: float = 3.5;
let SWEEP: float = 1.6;

let PI: float = 3.14159265;
let TWO_PI: float = 6.2831853;

let window = {
    width: 1000,
    height: 640
}

// String constants are functions (module-level char* globals read back as garbage in this engine).
function targetStr() => char* { return "0m0g1's OS"; }
function poolStr() => char* { return "01ABCDEF#$%&*+<>?/|[]{}=~^:;"; }
function subtitleStr() => char* { return "Pen2D + Pen3D on one canvas"; }
function hintStr() => char* { return "Esc: quit | R: replay | click: shockwave | mouse: orbit"; }

// ---------------------------------------------------------------- helpers
function rgba(r: int, g: int, b: int, a: int) => u32 {
    return ((r as u32) << 24) | ((g as u32) << 16) | ((b as u32) << 8) | (a as u32);
}

function hash(n: int) => int {
    let x: u32 = (n as u32) * (1103515245 as u32) + (12345 as u32);
    x = x ^ (x >> 15);
    x = x * (1103515245 as u32) + (12345 as u32);
    x = x ^ (x >> 13);
    return (x & 0x7FFFFFFF) as int;
}

function absf(v: float) => float {
    if (v < 0.0) { return 0.0 - v; }
    return v;
}

function sinf(x: float) => float {
    let n: int = (x / TWO_PI) as int;
    let r: float = x - (n as float) * TWO_PI;
    if (r > PI) { r = r - TWO_PI; }
    if (r < 0.0 - PI) { r = r + TWO_PI; }
    let r2: float = r * r;
    let r3: float = r2 * r;
    let r5: float = r3 * r2;
    let r7: float = r5 * r2;
    let r9: float = r7 * r2;
    return r - r3 / 6.0 + r5 / 120.0 - r7 / 5040.0 + r9 / 362880.0;
}

function cosf(x: float) => float { return sinf(x + PI / 2.0); }

function sqrtf(v: float) => float {
    if (v <= 0.0) { return 0.0; }
    let x: float = v;
    if (x < 1.0) { x = 1.0; }
    let i: int = 0;
    while (i < 10) { x = 0.5 * (x + v / x); i = i + 1; }
    return x;
}

function clampc(v: float) => int {
    let r: int = v as int;
    if (r < 0) { r = 0; }
    if (r > 255) { r = 255; }
    return r;
}

function palette(h: float, a: int) => u32 {
    let r: int = clampc(128.0 + 127.0 * sinf(TWO_PI * h));
    let g: int = clampc(128.0 + 127.0 * sinf(TWO_PI * (h + 0.333)));
    let b: int = clampc(128.0 + 127.0 * sinf(TWO_PI * (h + 0.667)));
    return rgba(r, g, b, a);
}

// Scratch matrices for drawBox.
struct Mats {
    tm: float*; rm: float*; rx: float*; sm: float*; ra: float*; rb: float*; fin: float*;
}

function initMats(m: Mats*) => void {
    m.tm = CMem.malloc(64) as float*; m.rm = CMem.malloc(64) as float*; m.rx = CMem.malloc(64) as float*;
    m.sm = CMem.malloc(64) as float*; m.ra = CMem.malloc(64) as float*; m.rb = CMem.malloc(64) as float*;
    m.fin = CMem.malloc(64) as float*;
}

function freeMats(m: Mats*) => void {
    CMem.free(m.tm as void*); CMem.free(m.rm as void*); CMem.free(m.rx as void*);
    CMem.free(m.sm as void*); CMem.free(m.ra as void*); CMem.free(m.rb as void*); CMem.free(m.fin as void*);
}

// One lit box: model = Translate * RotY * RotX * Scale.
function drawBox(scene: C.Pen3D*, cube: OGL.Mesh*, m: Mats*,
                 x: float, y: float, z: float, sx: float, sy: float, sz: float,
                 ry: float, rxa: float, col: u32) => void {
    C.matTranslate(m.tm, x, y, z);
    C.matScale(m.sm, sx, sy, sz);
    C.matRotateY(m.rm, ry);
    C.matRotateX(m.rx, rxa);
    C.matMul(m.ra, m.rm, m.rx);        // RotY * RotX
    C.matMul(m.rb, m.ra, m.sm);        // * Scale
    C.matMul(m.fin, m.tm, m.rb);       // Translate *
    scene.draw(cube, m.fin, col);
}

// Height of the city block at (x, z) at time t; pulseAge = seconds since the last click.
function cityHeight(x: float, z: float, t: float, pulseAge: float) => float {
    let d: float = sqrtf(x * x + z * z);
    let h: float = 0.9 + 0.55 * sinf(d * 0.85 - t * 2.4) + 0.35 * sinf(x * 0.5 + t * 1.3) * cosf(z * 0.5 - t * 0.9);
    if (pulseAge < 2.2) {
        let r: float = pulseAge * 9.0;
        let k: float = 1.0 - absf(d - r) / 1.8;
        if (k > 0.0) { h = h + 2.6 * k * (1.0 - pulseAge / 2.2); }
    }
    if (h < 0.15) { h = 0.15; }
    return h;
}

// ---------------------------------------------------------------- 2D HUD pieces
function drawBrackets(pen: C.Pen2D*, w: float, h: float, col: u32) => void {
    let m: float = 18.0; let l: float = 34.0;
    pen.fillRect(m, m, l, 2.0, col);                  pen.fillRect(m, m, 2.0, l, col);
    pen.fillRect(w - m - l, m, l, 2.0, col);          pen.fillRect(w - m - 2.0, m, 2.0, l, col);
    pen.fillRect(m, h - m - 2.0, l, 2.0, col);        pen.fillRect(m, h - m - l, 2.0, l, col);
    pen.fillRect(w - m - l, h - m - 2.0, l, 2.0, col); pen.fillRect(w - m - 2.0, h - m - l, 2.0, l, col);
}

function drawRing(pen: C.Pen2D*, cx: float, cy: float, r: float, t: float, col: u32) => void {
    let seg: int = 0;
    while (seg < 40) {
        let a0: float = TWO_PI * (seg as float) / 40.0;
        let a1: float = TWO_PI * ((seg + 1) as float) / 40.0;
        pen.line(cx + r * cosf(a0), cy + r * sinf(a0), cx + r * cosf(a1), cy + r * sinf(a1), t, col);
        seg = seg + 1;
    }
}

function drawRadar(pen: C.Pen2D*, cx: float, cy: float, r: float, t: float) => void {
    pen.circle(cx, cy, r, rgba(0, 20, 12, 170));
    drawRing(pen, cx, cy, r, 1.5, rgba(0, 255, 120, 200));
    drawRing(pen, cx, cy, r * 0.66, 1.0, rgba(0, 255, 120, 90));
    drawRing(pen, cx, cy, r * 0.33, 1.0, rgba(0, 255, 120, 90));
    pen.line(cx - r, cy, cx + r, cy, 1.0, rgba(0, 255, 120, 60));
    pen.line(cx, cy - r, cx, cy + r, 1.0, rgba(0, 255, 120, 60));

    let sweep: float = t * 2.0;
    // fading wedge behind the sweep line
    let k: int = 0;
    while (k < 12) {
        let a: float = sweep - (k as float) * 0.06;
        pen.line(cx, cy, cx + r * cosf(a), cy + r * sinf(a), 2.0, rgba(0, 255, 120, 120 - k * 9));
        k = k + 1;
    }

    // blips light up when the sweep passes them and fade
    let i: int = 0;
    while (i < 7) {
        let ba: float = ((hash(i * 5) % 628) as float) / 100.0;
        let bd: float = (((20 + hash(i * 9) % 75) as float) / 100.0) * r;
        let diff: float = sweep - ba;
        let n: int = (diff / TWO_PI) as int;
        diff = diff - (n as float) * TWO_PI;
        if (diff < 0.0) { diff = diff + TWO_PI; }
        let alpha: int = clampc(255.0 * (1.0 - diff / TWO_PI));
        pen.circle(cx + bd * cosf(ba), cy + bd * sinf(ba), 3.0, rgba(255, 200, 80, alpha));
        i = i + 1;
    }
}

// ---------------------------------------------------------------- main
function main() => int {
    let app: C.Canvas;

    if (!app.init()) return 1;

    app.iconPath = "docs/examples/icon.png";

    let win = app.createWindow("0m0g1's OS", window.width, window.height);

    if (win == nullptr) { app.quit(); return 2; }

    let pen = win.context2d();
    let scene = win.context3d();

    let cube = CMem.malloc(sizeof(OGL.Mesh)) as OGL.Mesh*;
    CMem.memset(cube as void*, 0, sizeof(OGL.Mesh));
    scene.createCube(cube);

    let mats = CMem.malloc(sizeof(Mats)) as Mats*;
    initMats(mats);

    let target: char* = targetStr();
    let pool: char* = poolStr();
    let ch = CMem.malloc(2) as char*;
    ch[1] = 0 as char;

    let sw: T.Stopwatch;
    sw.start();
    let pulseT: float = 0.0 - 10.0;

    while (app.frame()) {
        if (win.keyPressed(256)) app.quit();
        if (win.keyPressed(82)) sw.start();            // R: replay intro

        let t: float = sw.elapsedSeconds() as float;
        let w: int = pen.width();
        let h: int = pen.height();
        let wf: float = w as float;
        let hf: float = h as float;
        let u: float = wf / 1000.0;
        if (hf / 640.0 < u) { u = hf / 640.0; }

        if (win.mousePressed()) { pulseT = t; }
        let pulseAge: float = t - pulseT;

        // ================= 3D: neon city =================
        scene.clear(0.015, 0.02, 0.05, 1.0);

        let mxn: float = (win.mx / wf) - 0.5;          // -0.5 .. 0.5
        let myn: float = (win.my / hf) - 0.5;
        let ang: float = t * 0.15 + mxn * 2.2;
        let camR: float = 27.0;
        let camY: float = 13.0 - myn * 14.0;
        scene.camera(camR * sinf(ang), camY, camR * cosf(ang), 0.0, 1.2, 0.0, 55.0);
        scene.light(cosf(t * 0.4), 0.9, sinf(t * 0.4));

        let half: float = ((GRID - 1) as float) / 2.0;
        let gz: int = 0;
        while (gz < GRID) {
            let gx: int = 0;
            while (gx < GRID) {
                let x: float = ((gx as float) - half) * SPACING;
                let z: float = ((gz as float) - half) * SPACING;
                let ht: float = cityHeight(x, z, t, pulseAge);
                let d: float = sqrtf(x * x + z * z);
                let hue: float = d * 0.035 + t * 0.05 + ht * 0.12;
                drawBox(scene, cube, mats, x, ht / 2.0, z, SPACING * 0.78, ht, SPACING * 0.78, 0.0, 0.0, palette(hue, 255));
                gx = gx + 1;
            }
            gz = gz + 1;
        }

        // floating core + orbiting satellites
        drawBox(scene, cube, mats, 0.0, 6.5 + 0.6 * sinf(t * 1.5), 0.0, 2.6, 2.6, 2.6, t * 0.9, t * 0.6, palette(t * 0.12, 255));
        let s: int = 0;
        while (s < 4) {
            let a: float = t * 1.4 + (s as float) * (TWO_PI / 4.0);
            drawBox(scene, cube, mats, 5.0 * cosf(a), 6.5 + 1.8 * sinf(a * 2.0), 5.0 * sinf(a), 0.9, 0.9, 0.9, t * 2.0, t * 1.5,
                    palette(t * 0.12 + (s as float) * 0.25, 255));
            s = s + 1;
        }

        // ================= 2D: HUD on top =================
        // scanline-ish top/bottom bands
        pen.fillRect(0.0, 0.0, wf, 8.0 * u, rgba(0, 0, 0, 140));
        pen.fillRect(0.0, hf - 8.0 * u, wf, 8.0 * u, rgba(0, 0, 0, 140));
        drawBrackets(pen, wf, hf, palette(t * 0.1, 220));

        // ---- title with decode + left-to-right wave
        let ts: float = 3.4 * u;
        let gw: float = pen.textWidth("M", ts);
        let total: float = gw * (TARGET_N as float);
        let th: float = pen.textHeight(ts);
        let sx: float = (wf - total) / 2.0;
        let sy: float = 34.0 * u;
        pen.fillRect(sx - 24.0 * u, sy - 14.0 * u, total + 48.0 * u, th + 28.0 * u, rgba(3, 5, 9, 190));
        pen.rect(sx - 24.0 * u, sy - 14.0 * u, total + 48.0 * u, th + 28.0 * u, 1.0, palette(t * 0.1, 210));

        let waveOn: bool = false;
        let pos: float = 0.0;
        if (t > SETTLE) {
            let cyc: int = ((t - SETTLE) / PERIOD) as int;
            let ph: float = (t - SETTLE) - (cyc as float) * PERIOD;
            if (ph < SWEEP) {
                waveOn = true;
                pos = 0.0 - 1.5 + (ph / SWEEP) * ((TARGET_N as float) + 3.0);    // left -> right
            }
        }
        let frame: int = (t * 20.0) as int;

        let i: int = 0;
        while (i < TARGET_N) {
            let startT: float = 0.0;
            let lockT: float = 0.25 + (i as float) * 0.10;
            if (i >= KEEP) {
                startT = 1.4;
                lockT = 1.9 + ((i - KEEP) as float) * 0.15;
            }
            if (t >= startT) {
                let locked: bool = t >= lockT;
                ch[0] = pool[hash(i * 977 + frame) % POOL_N];
                let col: u32 = rgba(0, 140, 60, 255);
                let lift: float = 0.0;
                if (locked) {
                    ch[0] = target[i];
                    col = rgba(190, 255, 215, 255);
                    if (t - lockT < 0.12) { col = rgba(255, 255, 255, 255); }
                    if (waveOn) {
                        let d: float = absf((i as float) - pos);
                        if (d < 2.2) {
                            let k: float = 1.0 - d / 2.2;
                            lift = 9.0 * u * k;
                            col = rgba(60 + clampc(130.0 * (1.0 - k)), 255, 140 + clampc(75.0 * (1.0 - k)), 255);
                            if (d < 1.0) {
                                ch[0] = pool[hash(i * 31 + frame) % POOL_N];
                                col = rgba(0, 255, 120, 255);
                            }
                        }
                    }
                }
                let x: float = sx + (i as float) * gw;
                pen.textScaled(x + 2.0, sy + 2.0 - lift, ch, rgba(0, 40, 20, 255), ts);
                pen.textScaled(x, sy - lift, ch, col, ts);
            }
            i = i + 1;
        }

        // subtitle fades in after the intro
        let sub: char* = subtitleStr();
        let subW: float = pen.textWidth(sub, 1.5 * u);
        let subA: int = clampc(255.0 * (t - SETTLE) * 2.0);
        if (t > SETTLE) { pen.textScaled((wf - subW) / 2.0, sy + th + 24.0 * u, sub, rgba(150, 220, 255, subA), 1.5 * u); }

        // radar bottom-right, hint bottom-left
        drawRadar(pen, wf - 90.0 * u - 18.0, hf - 90.0 * u - 18.0, 70.0 * u, t);
        pen.text(22.0, hf - 38.0, hintStr(), rgba(120, 130, 150, 255));

        app.present();
    }

    freeMats(mats);
    CMem.free(mats as void*);
    CMem.free(ch as void*);
    return 0;
}
