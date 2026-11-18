// docs/examples/showcase.os
// Canvas showcase: matrix rain, 3D-ish orbiting rings, a decoding title with a right-to-left
// scramble wave, a glowing mouse trail and click ripples. Pure Pen2D, so it runs on GL, Vulkan or software.
// Esc = quit, R = replay the intro, click = ripple.

module C = import "standard/1/Canvas.os";
module T = import "standard/1/Time.os";
module CMem = import "standard/1/c/mem.os";

let TARGET_N: int = 10;
let KEEP: int = 5;
let POOL_N: int = 28;

let SETTLE: float = 2.7;
let PERIOD: float = 3.5;
let SWEEP: float = 1.4;
let SIZE: float = 4.0;

let TRAIL_N: int = 20;
let RIPPLES: int = 8;
let RAIN_TRAIL: int = 14;

let PI: float = 3.14159265;
let TWO_PI: float = 6.2831853;

let window = {
    width: 900,
    height: 640
}

// ---------------------------------------------------------------- helpers
// String constants are functions, not module-level lets (module-level char* globals read back as garbage).
function targetStr() => char* { return "0m0g1's OS"; }
function poolStr() => char* { return "01ABCDEF#$%&*+<>?/|[]{}=~^:;"; }

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

// sin via range reduction + Taylor series (good to ~1%, plenty for visuals).
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

function clampc(v: float) => int {
    let r: int = v as int;
    if (r < 0) { r = 0; }
    if (r > 255) { r = 255; }
    return r;
}

// Smooth rainbow: h in turns (0..1 repeating).
function palette(h: float, a: int) => u32 {
    let r: int = clampc(128.0 + 127.0 * sinf(TWO_PI * h));
    let g: int = clampc(128.0 + 127.0 * sinf(TWO_PI * (h + 0.333)));
    let b: int = clampc(128.0 + 127.0 * sinf(TWO_PI * (h + 0.667)));
    return rgba(r, g, b, a);
}

// ---------------------------------------------------------------- drawing pieces
function drawRain(pen: C.Pen2D*, ch: char*, t: float, w: int, h: int) => void {
    let pool: char* = poolStr();
    let cols: int = w / 8;
    let rows: int = h / 16;
    let total: int = rows + RAIN_TRAIL;
    let step: int = (t * 4.0) as int;
    let c: int = 0;
    while (c < cols) {
        let speed: int = 6 + hash(c * 7) % 15;
        let off: int = hash(c * 13) % total;
        let head: int = (((t * (speed as float)) as int) + off) % total;
        let k: int = 0;
        while (k < RAIN_TRAIL) {
            let row: int = head - k;
            if (row >= 0 && row < rows) {
                ch[0] = pool[hash(c * 131 + row * 17 + step) % POOL_N];
                let fade: float = 1.0 - (k as float) / (RAIN_TRAIL as float);
                let col: u32 = rgba(0, 40 + clampc(150.0 * fade), 30 + clampc(40.0 * fade), 255);
                if (k == 0) { col = rgba(190, 255, 210, 255); }
                pen.text((c * 8) as float, (row * 16) as float, ch, col);
            }
            k = k + 1;
        }
        c = c + 1;
    }
}

// One pass over three tilted rings. pass 0 = back half (before the title), pass 1 = front half (after it).
function drawRings(pen: C.Pen2D*, t: float, cx: float, cy: float, u: float, pass: int) => void {
    let ring: int = 0;
    while (ring < 3) {
        let n: int = 8 + ring * 4;
        let rad: float = (210.0 + (ring as float) * 55.0) * u;
        let dir: float = 1.0;
        if (ring == 1) { dir = 0.0 - 1.0; }
        let spin: float = t * (0.6 + (ring as float) * 0.25) * dir;
        let i: int = 0;
        while (i < n) {
            let a: float = spin + TWO_PI * (i as float) / (n as float);
            let a2: float = spin + TWO_PI * ((i + 1) as float) / (n as float);
            let s: float = sinf(a);
            let x: float = cx + rad * cosf(a);
            let y: float = cy + rad * 0.42 * s;
            let front: bool = s >= 0.0;
            if ((pass == 1 && front) || (pass == 0 && !front)) {
                let depth: float = (s + 1.0) / 2.0;
                let hue: float = (i as float) / (n as float) + t * 0.08 + (ring as float) * 0.2;
                pen.circle(x, y, (3.0 + 5.0 * depth) * u, palette(hue, 120 + clampc(135.0 * depth)));
            }
            if (pass == 0) {
                let x2: float = cx + rad * cosf(a2);
                let y2: float = cy + rad * 0.42 * sinf(a2);
                pen.line(x, y, x2, y2, 1.0, palette((i as float) / (n as float) + t * 0.08 + (ring as float) * 0.2, 70));
            }
            i = i + 1;
        }
        ring = ring + 1;
    }
}

function drawRipple(pen: C.Pen2D*, x: float, y: float, age: float) => void {
    let r: float = age * 320.0;
    let a: int = clampc(255.0 * (1.0 - age / 1.2));
    let seg: int = 0;
    while (seg < 36) {
        let a0: float = TWO_PI * (seg as float) / 36.0;
        let a1: float = TWO_PI * ((seg + 1) as float) / 36.0;
        pen.line(x + r * cosf(a0), y + r * sinf(a0), x + r * cosf(a1), y + r * sinf(a1), 2.0,
                 palette(age * 0.6, a));
        seg = seg + 1;
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

    let sw: T.Stopwatch;
    sw.start();

    let target: char* = targetStr();
    let pool: char* = poolStr();
    let ch = CMem.malloc(2) as char*;
    ch[1] = 0 as char;

    // mouse trail + ripples
    let trx = CMem.malloc((TRAIL_N * 4) as u64) as float*;
    let try_ = CMem.malloc((TRAIL_N * 4) as u64) as float*;
    let rx = CMem.malloc((RIPPLES * 4) as u64) as float*;
    let ry = CMem.malloc((RIPPLES * 4) as u64) as float*;
    let rt = CMem.malloc((RIPPLES * 4) as u64) as float*;
    let j: int = 0;
    while (j < TRAIL_N) { trx[j] = 0.0; try_[j] = 0.0; j = j + 1; }
    j = 0;
    while (j < RIPPLES) { rx[j] = 0.0; ry[j] = 0.0; rt[j] = 0.0 - 10.0; j = j + 1; }
    let frameNo: int = 0;
    let rippleNo: int = 0;

    while (app.frame()) {
        if (win.keyPressed(256)) app.quit();

        pen.clear(0.02, 0.03, 0.05, 1.0);

        if (win.keyPressed(82)) sw.start();                // R: replay intro

        let t: float = sw.elapsedSeconds() as float;
        let w: int = pen.width();
        let h: int = pen.height();
        let cx: float = (w as float) / 2.0;
        let cy: float = (h as float) / 2.0;
        let u: float = (w as float) / 900.0;
        if ((h as float) / 640.0 < u) { u = (h as float) / 640.0; }

        // input
        if (win.mousePressed()) {
            rx[rippleNo % RIPPLES] = win.mx;
            ry[rippleNo % RIPPLES] = win.my;
            rt[rippleNo % RIPPLES] = t;
            rippleNo = rippleNo + 1;
        }
        trx[frameNo % TRAIL_N] = win.mx;
        try_[frameNo % TRAIL_N] = win.my;

        // ---- background layers
        drawRain(pen, ch, t, w, h);
        drawRings(pen, t, cx, cy, u, 0);

        // ---- title panel
        let s: float = SIZE * u;
        let gw: float = pen.textWidth("M", s);
        let total: float = gw * (TARGET_N as float);
        let th: float = pen.textHeight(s);
        let sx: float = cx - total / 2.0;
        let sy: float = cy - th / 2.0;
        pen.fillRect(sx - 28.0 * u, sy - 20.0 * u, total + 56.0 * u, th + 40.0 * u, rgba(3, 5, 9, 215));
        pen.rect(sx - 28.0 * u, sy - 20.0 * u, total + 56.0 * u, th + 40.0 * u, 1.0, palette(t * 0.1, 200));

        // wave position (letter units), right -> left
        let waveOn: bool = false;
        let pos: float = 0.0;
        if (t > SETTLE) {
            let cyc: int = ((t - SETTLE) / PERIOD) as int;
            let ph: float = (t - SETTLE) - (cyc as float) * PERIOD;
            if (ph < SWEEP) {
                waveOn = true;
                pos = ((TARGET_N as float) + 1.5) - (ph / SWEEP) * ((TARGET_N as float) + 3.0);
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
                            lift = 10.0 * u * k;
                            col = rgba(60 + clampc(130.0 * (1.0 - k)), 255, 140 + clampc(75.0 * (1.0 - k)), 255);
                            if (d < 1.0) {
                                ch[0] = pool[hash(i * 31 + frame) % POOL_N];
                                col = rgba(0, 255, 120, 255);
                            }
                        }
                    }
                }
                let x: float = sx + (i as float) * gw;
                pen.textScaled(x + 2.0, sy + 2.0 - lift, ch, rgba(0, 40, 20, 255), s);
                pen.textScaled(x, sy - lift, ch, col, s);
            }
            i = i + 1;
        }

        // ---- foreground: front half of the rings, ripples, mouse trail
        drawRings(pen, t, cx, cy, u, 1);

        j = 0;
        while (j < RIPPLES) {
            let age: float = t - rt[j];
            if (age >= 0.0 && age < 1.2) { drawRipple(pen, rx[j], ry[j], age); }
            j = j + 1;
        }

        j = 0;
        while (j < TRAIL_N) {
            let idx: int = ((frameNo - j) % TRAIL_N + TRAIL_N) % TRAIL_N;
            let f: float = 1.0 - (j as float) / (TRAIL_N as float);
            pen.circle(trx[idx], try_[idx], 3.0 + 10.0 * f, palette(t * 0.3 + (j as float) * 0.03, clampc(150.0 * f)));
            j = j + 1;
        }

        pen.text(10.0, (h as float) - 22.0, "Esc: quit | R: replay intro | click: ripple", rgba(110, 120, 140, 255));

        frameNo = frameNo + 1;
        app.present();
    }

    CMem.free(ch as void*);
    CMem.free(trx as void*); CMem.free(try_ as void*);
    CMem.free(rx as void*); CMem.free(ry as void*); CMem.free(rt as void*);
    return 0;
}
