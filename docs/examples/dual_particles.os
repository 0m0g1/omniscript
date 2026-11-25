// docs/examples/dual_particles.os
// ONE particle world shared by TWO OpenGL windows. Particles live in screen coordinates, so a particle
// that flies out of window A through the gap shows up in window B (and is invisible while in the gap).
//
//  - every particle is pulled toward the OTHER window: it flips its target when it reaches a window,
//    so the swarm keeps ping-ponging between the two windows; move a window and the whole flow bends
//  - particles collide elastically (spatial hash grid), flash white on impact and kick their hue
//  - colour = which window it is heading to (blue -> pink, blended in flight), brightness = speed
//  - dragging a window shoves the particles inside it
//  - a render thread does simulation + drawing, so dragging a window does not freeze anything
//
//  Left mouse   : attract + swirl (vortex)       Right mouse : repel (shockwave)
//  Space        : burst of particles at cursor   R          : reset      Esc : close focused window
//
// Uses std modules: Math (f32 min/max/clamp/damp), Vec (Vec2 maths), Color (hsv), Spatial (collision grid),
// Mem (arena), Time (FrameTimer), Thread, Random (RNG), String (number -> text for the HUD).
// Random works in double and the sim in float, hence the cast in rnd().
// Drag the windows apart if they open on top of each other.
module C = import "standard/1/Canvas.os";
module G = import "standard/1/c/glfw.os";
module Mem = import "standard/1/Mem.os";
module T = import "standard/1/Thread.os";
module Tm = import "standard/1/Time.os";
module M = import "standard/1/Math.os";
module V = import "standard/1/Vec.os";
module Col = import "standard/1/Color.os";
module Sp = import "standard/1/Spatial.os";
module R = import "standard/1/Random.os";
module S = import "standard/1/String.os";

let MAXP: int = 1500;
let START_N: int = 700;
let RAD: float = 5.0;            // particle radius in screen units
let CELL: float = 12.0;          // collision grid cell size (> 2*RAD)
let TABLE: int = 8192;           // hash buckets
let PULL: float = 320.0;         // acceleration toward the target window
let MAXV: float = 1100.0;
let REST: float = 0.92;          // restitution of collisions / walls
let HUE_A: float = 0.58;         // heading to window A (blue)
let HUE_B: float = 0.93;         // heading to window B (pink)

struct Slot {
    win: C.Win* = nullptr;
    ready: bool = false;
}

struct World {
    n: int = 0;
    inited: bool = false;
    hadPrev: bool = false;
    reset: bool = false;
    burst: bool = false;
    bx: float = 0.0; by: float = 0.0;
    rng: R.Random;
    x: float* = nullptr; y: float* = nullptr;
    vx: float* = nullptr; vy: float* = nullptr;
    hue: float* = nullptr; flash: float* = nullptr;
    tgt: int* = nullptr;
    mem: Mem.Arena;                  // backs x, y, vx, vy, hue, flash, tgt
    grid: Sp.SpatialHash;            // collision grid (owns its own buckets)
    label: S.String;                 // scratch buffer for HUD numbers
    fps: float = 0.0;
    collisions: int = 0;
    // arena = bounding box of the open windows (screen units)
    ax0: float = 0.0; ay0: float = 0.0; ax1: float = 0.0; ay1: float = 0.0;
    // window A / B client rects (screen units), published by the draw code each frame
    aOpen: bool = false; bOpen: bool = false;
    aX: float = 0.0; aY: float = 0.0; aW: float = 0.0; aH: float = 0.0;
    bX: float = 0.0; bY: float = 0.0; bW: float = 0.0; bH: float = 0.0;
    paX: float = 0.0; paY: float = 0.0; pbX: float = 0.0; pbY: float = 0.0;
    wvAx: float = 0.0; wvAy: float = 0.0; wvBx: float = 0.0; wvBy: float = 0.0;
    // mouse in global screen coordinates, per window
    aMx: float = 0.0; aMy: float = 0.0; aAtt: bool = false; aRep: bool = false;
    bMx: float = 0.0; bMy: float = 0.0; bAtt: bool = false; bRep: bool = false;
}

struct Shared {
    a: Slot* = nullptr;
    b: Slot* = nullptr;
    wd: World* = nullptr;
    stop: T.Atomic;
}

// ------------------------------------------------------------------ small helpers
// GLFW key codes. Move to Canvas.os once it exports them.
let KEY_SPACE: int = 32;
let KEY_R: int = 82;
let KEY_ESC: int = 256;

function rnd(wd: World*) => float {
    return wd.rng.nextFloat() as float;
}

function labelInt(pen: C.Pen2D*, buf: S.String*, x: float, y: float, label: char*, v: int, c: u32) => void {
    buf.clear();
    buf.appendInt(v as i64);
    pen.text(x, y, label, c);
    pen.text(x + pen.textWidth(label, 1.0), y, buf.cstr(), c);
}

function closed(t: Slot*) => bool {
    if (t.win == nullptr) return true;
    return G.glfwWindowShouldClose(t.win.win.h) != 0;
}

function inRect(px: float, py: float, rx: float, ry: float, rw: float, rh: float) => bool {
    return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}

// ------------------------------------------------------------------ world setup
function place(wd: World*, i: int) => void {
    let useA: bool = wd.aOpen;
    if (wd.aOpen && wd.bOpen) useA = rnd(wd) < 0.5;
    if (useA) {
        wd.x[i] = wd.aX + rnd(wd) * wd.aW;
        wd.y[i] = wd.aY + rnd(wd) * wd.aH;
        wd.tgt[i] = 1;
        wd.hue[i] = HUE_B;
    } else {
        wd.x[i] = wd.bX + rnd(wd) * wd.bW;
        wd.y[i] = wd.bY + rnd(wd) * wd.bH;
        wd.tgt[i] = 0;
        wd.hue[i] = HUE_A;
    }
    wd.vx[i] = (rnd(wd) - 0.5) * 200.0;
    wd.vy[i] = (rnd(wd) - 0.5) * 200.0;
    wd.flash[i] = 0.0;
}

function allocWorld(wd: World*) => bool {
    let fb: u64 = (MAXP * 4) as u64;
    if (!wd.mem.init(fb * 7)) return false;                  // six float arrays + the target array
    wd.x = wd.mem.alloc(fb) as float*;
    wd.y = wd.mem.alloc(fb) as float*;
    wd.vx = wd.mem.alloc(fb) as float*;
    wd.vy = wd.mem.alloc(fb) as float*;
    wd.hue = wd.mem.alloc(fb) as float*;
    wd.flash = wd.mem.alloc(fb) as float*;
    wd.tgt = wd.mem.alloc(fb) as int*;
    if (!wd.grid.init(CELL, TABLE, MAXP)) return false;
    wd.rng.seed(2463534242 as u64);
    return true;
}

// ------------------------------------------------------------------ physics
function applyWell(wd: World*, i: int, mx: float, my: float, att: bool, rep: bool, dt: float) => void {
    if (!att && !rep) return;
    let off = V.vec2(mx - wd.x[i], my - wd.y[i]);
    let d: float = off.length() + 1.0;
    let n = off.scale(1.0 / d);
    if (att && d < 420.0) {
        let a: float = 2600.0 * (1.0 - d / 420.0);
        let acc = n.scale(a).add(n.perp().scale(a * 0.35));      // pull + sideways swirl -> vortex
        wd.vx[i] = wd.vx[i] + acc.x * dt;
        wd.vy[i] = wd.vy[i] + acc.y * dt;
    }
    if (rep && d < 320.0) {
        let a: float = 7000.0 * (1.0 - d / 320.0);
        let acc = n.scale(a);
        wd.vx[i] = wd.vx[i] - acc.x * dt;
        wd.vy[i] = wd.vy[i] - acc.y * dt;
    }
}

function collide(wd: World*) => void {
    wd.grid.setOrigin(wd.ax0, wd.ay0);
    wd.grid.clear();
    let p: int = 0;
    while (p < wd.n) {
        wd.grid.insert(p, wd.x[p], wd.y[p]);
        p = p + 1;
    }

    let minD: float = RAD * 2.0;
    let minD2: float = minD * minD;
    p = 0;
    while (p < wd.n) {
        let cx: int = wd.grid.cellX(wd.x[p]);
        let cy: int = wd.grid.cellY(wd.y[p]);
        let oy: int = -1;
        while (oy <= 1) {
            let ox: int = -1;
            while (ox <= 1) {
                let q: int = wd.grid.first(cx + ox, cy + oy);
                while (q >= 0) {
                    if (q > p) {
                        let off = V.vec2(wd.x[q] - wd.x[p], wd.y[q] - wd.y[p]);
                        let d2: float = off.lengthSq();
                        if (d2 < minD2 && d2 > 0.0001) {
                            let d: float = off.length();
                            let nrm = off.scale(1.0 / d);
                            let ov: float = (minD - d) * 0.5;
                            wd.x[p] = wd.x[p] - nrm.x * ov;  wd.y[p] = wd.y[p] - nrm.y * ov;
                            wd.x[q] = wd.x[q] + nrm.x * ov;  wd.y[q] = wd.y[q] + nrm.y * ov;
                            let rel = V.vec2(wd.vx[p] - wd.vx[q], wd.vy[p] - wd.vy[q]);
                            let vn: float = rel.dot(nrm);
                            if (vn > 0.0) {                         // approaching: equal-mass elastic impulse
                                let j: float = vn * 0.5 * (1.0 + REST);
                                wd.vx[p] = wd.vx[p] - j * nrm.x;  wd.vy[p] = wd.vy[p] - j * nrm.y;
                                wd.vx[q] = wd.vx[q] + j * nrm.x;  wd.vy[q] = wd.vy[q] + j * nrm.y;
                                if (vn > 60.0) {                    // real hit: flash + colour kick
                                    wd.flash[p] = 1.0;  wd.flash[q] = 1.0;
                                    wd.hue[p] = wd.hue[p] + 0.07;
                                    wd.hue[q] = wd.hue[q] - 0.07;
                                    wd.collisions = wd.collisions + 1;
                                }
                            }
                        }
                    }
                    q = wd.grid.following(q);
                }
                ox = ox + 1;
            }
            oy = oy + 1;
        }
        p = p + 1;
    }
}

function stepSim(wd: World*, dt: float) => void {
    let cAx: float = wd.aX + wd.aW * 0.5;
    let cAy: float = wd.aY + wd.aH * 0.5;
    let cBx: float = wd.bX + wd.bW * 0.5;
    let cBy: float = wd.bY + wd.bH * 0.5;
    let wsA: float = V.vec2(wd.wvAx, wd.wvAy).length();
    let wsB: float = V.vec2(wd.wvBx, wd.wvBy).length();

    let i: int = 0;
    while (i < wd.n) {
        // which window are we heading for (fall back to the only open one)
        let tg: int = wd.tgt[i];
        if (tg == 1 && !wd.bOpen) tg = 0;
        if (tg == 0 && !wd.aOpen) tg = 1;
        let tx: float = cAx;  let ty: float = cAy;
        if (tg == 1) { tx = cBx; ty = cBy; }

        let toT = V.vec2(tx - wd.x[i], ty - wd.y[i]);
        let d: float = toT.length() + 1.0;
        let dir = toT.scale(1.0 / d);
        let pull = dir.scale(PULL).add(dir.perp().scale(40.0));
        wd.vx[i] = wd.vx[i] + pull.x * dt;
        wd.vy[i] = wd.vy[i] + pull.y * dt;

        // arrived: inner 60% of the target window -> turn around and head for the other one
        if (tg == 0 && wd.aOpen && inRect(wd.x[i], wd.y[i], wd.aX + wd.aW * 0.2, wd.aY + wd.aH * 0.2, wd.aW * 0.6, wd.aH * 0.6)) wd.tgt[i] = 1;
        if (tg == 1 && wd.bOpen && inRect(wd.x[i], wd.y[i], wd.bX + wd.bW * 0.2, wd.bY + wd.bH * 0.2, wd.bW * 0.6, wd.bH * 0.6)) wd.tgt[i] = 0;

        // dragging a window shoves what is inside it
        if (wd.aOpen && wsA > 15.0 && inRect(wd.x[i], wd.y[i], wd.aX, wd.aY, wd.aW, wd.aH)) {
            wd.vx[i] = wd.vx[i] + wd.wvAx * 2.0 * dt;
            wd.vy[i] = wd.vy[i] + wd.wvAy * 2.0 * dt;
        }
        if (wd.bOpen && wsB > 15.0 && inRect(wd.x[i], wd.y[i], wd.bX, wd.bY, wd.bW, wd.bH)) {
            wd.vx[i] = wd.vx[i] + wd.wvBx * 2.0 * dt;
            wd.vy[i] = wd.vy[i] + wd.wvBy * 2.0 * dt;
        }

        applyWell(wd, i, wd.aMx, wd.aMy, wd.aAtt, wd.aRep, dt);
        applyWell(wd, i, wd.bMx, wd.bMy, wd.bAtt, wd.bRep, dt);

        // mild drag + speed limit, then integrate
        let drag: float = 1.0 - 0.12 * dt;
        let vel = V.vec2(wd.vx[i] * drag, wd.vy[i] * drag).limit(MAXV);
        let pos = V.vec2(wd.x[i], wd.y[i]).mulAdd(vel, dt);
        wd.vx[i] = vel.x;  wd.vy[i] = vel.y;
        wd.x[i] = pos.x;   wd.y[i] = pos.y;

        // arena walls (bounding box of the windows)
        if (wd.x[i] < wd.ax0) { wd.x[i] = wd.ax0; if (wd.vx[i] < 0.0) wd.vx[i] = -wd.vx[i] * REST; }
        if (wd.x[i] > wd.ax1) { wd.x[i] = wd.ax1; if (wd.vx[i] > 0.0) wd.vx[i] = -wd.vx[i] * REST; }
        if (wd.y[i] < wd.ay0) { wd.y[i] = wd.ay0; if (wd.vy[i] < 0.0) wd.vy[i] = -wd.vy[i] * REST; }
        if (wd.y[i] > wd.ay1) { wd.y[i] = wd.ay1; if (wd.vy[i] > 0.0) wd.vy[i] = -wd.vy[i] * REST; }

        // colour: ease toward the heading colour, wrap, fade the impact flash
        let th: float = HUE_A;
        if (wd.tgt[i] == 1) th = HUE_B;
        wd.hue[i] = Col.wrapHue(M.dampF32(wd.hue[i], th, 1.6, dt));
        wd.flash[i] = M.maxF32(0.0, wd.flash[i] - 3.0 * dt);
        i = i + 1;
    }
    collide(wd);
}

function tick(wd: World*, dt: float) => void {
    if (!wd.aOpen && !wd.bOpen) return;

    // arena = bounding box of the open windows, inset by the particle radius
    let x0: float = 1000000.0; let y0: float = 1000000.0;
    let x1: float = -1000000.0; let y1: float = -1000000.0;
    if (wd.aOpen) {
        x0 = M.minF32(x0, wd.aX); y0 = M.minF32(y0, wd.aY);
        x1 = M.maxF32(x1, wd.aX + wd.aW); y1 = M.maxF32(y1, wd.aY + wd.aH);
    }
    if (wd.bOpen) {
        x0 = M.minF32(x0, wd.bX); y0 = M.minF32(y0, wd.bY);
        x1 = M.maxF32(x1, wd.bX + wd.bW); y1 = M.maxF32(y1, wd.bY + wd.bH);
    }
    wd.ax0 = x0 + RAD; wd.ay0 = y0 + RAD; wd.ax1 = x1 - RAD; wd.ay1 = y1 - RAD;

    if (!wd.inited || wd.reset) {
        wd.n = START_N;
        let i: int = 0;
        while (i < wd.n) { place(wd, i); i = i + 1; }
        wd.inited = true;
        wd.reset = false;
        wd.hadPrev = false;
    }

    // window velocities (screen units / s), smoothed
    if (wd.hadPrev && dt > 0.0001) {
        wd.wvAx = wd.wvAx * 0.7 + M.clampF32((wd.aX - wd.paX) / dt, -1500.0, 1500.0) * 0.3;
        wd.wvAy = wd.wvAy * 0.7 + M.clampF32((wd.aY - wd.paY) / dt, -1500.0, 1500.0) * 0.3;
        wd.wvBx = wd.wvBx * 0.7 + M.clampF32((wd.bX - wd.pbX) / dt, -1500.0, 1500.0) * 0.3;
        wd.wvBy = wd.wvBy * 0.7 + M.clampF32((wd.bY - wd.pbY) / dt, -1500.0, 1500.0) * 0.3;
    }
    wd.paX = wd.aX; wd.paY = wd.aY; wd.pbX = wd.bX; wd.pbY = wd.bY;
    wd.hadPrev = true;

    if (wd.burst) {
        wd.burst = false;
        let k: int = 0;
        while (k < 40 && wd.n < MAXP) {
            let i: int = wd.n;
            wd.n = wd.n + 1;
            let rx: float = rnd(wd) * 2.0 - 1.0;
            let ry: float = rnd(wd) * 2.0 - 1.0;
            wd.x[i] = M.clampF32(wd.bx, wd.ax0, wd.ax1);
            wd.y[i] = M.clampF32(wd.by, wd.ay0, wd.ay1);
            wd.vx[i] = rx * 450.0;
            wd.vy[i] = ry * 450.0;
            wd.tgt[i] = 0;
            if (rnd(wd) < 0.5) wd.tgt[i] = 1;
            wd.hue[i] = rnd(wd);
            wd.flash[i] = 1.0;
            k = k + 1;
        }
    }

    let sub: float = dt * 0.5;                 // two substeps keep collisions stable
    stepSim(wd, sub);
    stepSim(wd, sub);
}

// ------------------------------------------------------------------ one window: publish rect/input, draw
function runSlot(t: Slot*, wd: World*, idx: int) => void {
    if (closed(t)) {
        if (idx == 0) wd.aOpen = false; else wd.bOpen = false;
        return;
    }
    let win: C.Win* = t.win;
    if (!t.ready) {
        let p0: C.Pen2D* = win.context2d();
        p0.clearR = 0.012; p0.clearG = 0.014; p0.clearB = 0.035; p0.clearA = 1.0;
        t.ready = true;
    }

    win.beginFrame();                          // makes this window's GL context current, polls input, clears
    let pen: C.Pen2D* = win.context2d();
    let w: float = win.width() as float;
    let h: float = win.height() as float;

    if (!win.active) {                         // minimised
        if (idx == 0) wd.aOpen = false; else wd.bOpen = false;
        win.endFrame();
        return;
    }

    let wx: float = win.posX() as float;
    let wy: float = win.posY() as float;
    let sw: float = win.screenW() as float;
    let sh: float = win.screenH() as float;
    let sx: float = 1.0;
    let sy: float = 1.0;
    if (sw > 0.0) sx = w / sw;
    if (sh > 0.0) sy = h / sh;

    if (win.keyPressed(KEY_ESC)) win.close();

    // publish this window's rect + mouse to the shared world
    let mx: float = win.mx;
    let my: float = win.my;
    let inside: bool = mx >= 0.0 && mx < w && my >= 0.0 && my < h;
    let att: bool = inside && win.mouseDown();
    let rep: bool = inside && win.rightDown();
    let gmx: float = wx + mx / sx;
    let gmy: float = wy + my / sy;
    if (idx == 0) {
        wd.aOpen = true; wd.aX = wx; wd.aY = wy; wd.aW = sw; wd.aH = sh;
        wd.aMx = gmx; wd.aMy = gmy; wd.aAtt = att; wd.aRep = rep;
    } else {
        wd.bOpen = true; wd.bX = wx; wd.bY = wy; wd.bW = sw; wd.bH = sh;
        wd.bMx = gmx; wd.bMy = gmy; wd.bAtt = att; wd.bRep = rep;
    }
    if (inside && win.keyPressed(KEY_SPACE)) { wd.burst = true; wd.bx = gmx; wd.by = gmy; }
    if (win.keyPressed(KEY_R)) wd.reset = true;

    // draw every particle that is currently inside THIS window's rectangle
    let rr: float = RAD * sx * 0.8;
    let i: int = 0;
    while (i < wd.n) {
        let lx: float = (wd.x[i] - wx) * sx;
        let ly: float = (wd.y[i] - wy) * sy;
        if (lx > -14.0 && lx < w + 14.0 && ly > -14.0 && ly < h + 14.0) {
            let sp: float = V.vec2(wd.vx[i], wd.vy[i]).length();
            let k: float = M.minF32(1.0, sp / 700.0);
            let fl: float = wd.flash[i];
            let sat: float = 0.85 - 0.65 * fl;
            let val: float = M.minF32(1.0, 0.62 + 0.38 * k + 0.4 * fl);
            let hu: float = wd.hue[i];
            pen.fillRect(lx - rr * 2.4, ly - rr * 2.4, rr * 4.8, rr * 4.8, Col.hsv(hu, sat, val, 38));   // glow
            pen.line(lx, ly, lx - wd.vx[i] * 0.03 * sx, ly - wd.vy[i] * 0.03 * sy, rr * 1.2, Col.hsv(hu, sat, val, 120));
            pen.fillRect(lx - rr, ly - rr, rr * 2.0, rr * 2.0, Col.hsv(hu, sat, val, 255));               // core
        }
        i = i + 1;
    }

    // cursor effects
    if (att) pen.fillRect(mx - 18.0, my - 18.0, 36.0, 36.0, 0x40FFFF50);
    if (rep) pen.fillRect(mx - 18.0, my - 18.0, 36.0, 36.0, 0xFF804060);

    // HUD
    pen.fillRect(0.0, 0.0, w, 26.0, 0x00000090);
    let name: char* = "A";
    if (idx == 1) name = "B";
    pen.text(10.0, 7.0, name, 0xFFFFFFFF);
    labelInt(pen, &wd.label, 40.0, 7.0, "particles ", wd.n, 0x55DD77FF);
    labelInt(pen, &wd.label, 200.0, 7.0, "fps ", wd.fps as int, 0xFFEE55FF);
    labelInt(pen, &wd.label, 290.0, 7.0, "hits ", wd.collisions, 0xFF88CCFF);
    pen.text(w - 400.0, h - 20.0, "L: attract   R: repel   Space: burst   R-key: reset   Esc: close", 0xAAAAAAFF);

    win.endFrame();                            // flush + swap
}

// ------------------------------------------------------------------ render thread + main
function renderThread(arg: void*) => u32 {
    let s: Shared* = arg as Shared*;
    let wd: World* = s.wd;
    let ft: Tm.FrameTimer;                     // dt clamped to its maxDt (0.04 s), smoothed fps
    while (s.stop.load() == 0) {
        let dt: float = ft.tick() as float;
        wd.fps = ft.fps as float;

        tick(wd, dt);
        runSlot(s.a, wd, 0);
        runSlot(s.b, wd, 1);

        if (closed(s.a) && closed(s.b)) T.sleepMs(5 as u32);
    }
    G.glfwMakeContextCurrent(nullptr);         // release the context before the thread ends
    return 0 as u32;
}

function newSlot() => Slot* {
    let t = Mem.allocZeroed(sizeof(Slot)) as Slot*;
    return t;
}

function openSlot(app: C.Canvas*, t: Slot*, title: char*) => bool {
    let win: C.Win* = app.createWindowEx(title, 640, 480, C.API_GL);
    if (win == nullptr) return false;
    t.win = win;
    return true;
}

function main() => int {
    let app: C.Canvas;
    if (!app.init()) return 1;

    let a: Slot* = newSlot();
    let b: Slot* = newSlot();
    if (!openSlot(&app, a, "Window A")) { app.quit(); return 2; }
    if (!openSlot(&app, b, "Window B")) { app.quit(); return 3; }

    // Window creation leaves a GL context current on THIS thread; hand it over to the render thread.
    G.glfwMakeContextCurrent(nullptr);

    let wd = Mem.allocZeroed(sizeof(World)) as World*;
    if (!allocWorld(wd)) { app.quit(); return 5; }

    let sh = Mem.allocZeroed(sizeof(Shared)) as Shared*;
    sh.a = a;
    sh.b = b;
    sh.wd = wd;
    sh.stop.store(0);

    let th: T.Thread;
    if (!T.spawnThread(renderThread, sh as void*, &th)) { app.quit(); return 4; }

    // Main thread only pumps OS events. It may block while a window is being dragged; the render thread doesn't care.
    while (!(closed(a) && closed(b))) {
        G.glfwPollEvents();
        T.sleepMs(2 as u32);
    }

    sh.stop.store(1);
    th.join();
    wd.label.free();
    wd.grid.free();
    wd.mem.free();
    app.quit();
    return 0;
}
