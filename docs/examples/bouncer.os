// bouncer.os
// Simple Canvas test app: click to spawn bouncing balls, Clear button, FPS + ball counter.
//   Left click   spawn a ball at the cursor
//   Clear button remove all balls
//   Backspace    remove the last ball
//   Esc          quit
module C = import "standard/1/Canvas.os";
module CMem = import "standard/1/c/mem.os";

let MAXB: int = 256;
let RADIUS: float = 14.0;

function palette(i: int) => u32 {
    let k: int = i % 6;
    if (k == 0) return 0xFF5555FF;
    if (k == 1) return 0xFFAA33FF;
    if (k == 2) return 0xFFEE55FF;
    if (k == 3) return 0x55DD77FF;
    if (k == 4) return 0x55AAFFFF;
    return 0xBB77FFFF;
}

// Writes the decimal digits of v (v >= 0) into buf, NUL-terminated. buf needs >= 12 bytes.
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

function inRect(px: float, py: float, x: float, y: float, w: float, h: float) => bool {
    return px >= x && px <= x + w && py >= y && py <= y + h;
}

function main() => int {
    let app: C.Canvas;
    if (!app.init()) return 1;

    let win: C.Win* = app.createWindow("Omniscript - Bouncer", 800, 600);
    if (win == nullptr) { app.quit(); return 2; }
    let pen: C.Pen2D* = win.context2d();

    // ball state
    let bx = CMem.malloc((MAXB * 4) as u64) as float*;
    let by = CMem.malloc((MAXB * 4) as u64) as float*;
    let vx = CMem.malloc((MAXB * 4) as u64) as float*;
    let vy = CMem.malloc((MAXB * 4) as u64) as float*;
    let nb: int = 0;
    let spawned: int = 0;

    let num = CMem.malloc(16) as char*;
    let last: float = app.time() as float;
    let fps: float = 0.0;

    while (app.frame()) {
        let now: float = app.time() as float;
        let dt: float = now - last;
        last = now;
        if (dt > 0.05) dt = 0.05;
        if (dt > 0.0) fps = fps * 0.9 + (1.0 / dt) * 0.1;

        let w: float = win.width() as float;
        let h: float = win.height() as float;

        // ---- input ----
        if (win.keyPressed(256)) win.close();                     // Esc
        if (win.backspace() && nb > 0) nb = nb - 1;

        let btnX: float = 10.0; let btnY: float = h - 42.0;
        let btnW: float = 90.0; let btnH: float = 32.0;
        let overBtn: bool = inRect(win.mx, win.my, btnX, btnY, btnW, btnH);

        if (win.mousePressed()) {
            if (overBtn) { nb = 0; }
            else if (nb < MAXB && win.my > 30.0) {
                let a: float = (spawned as float) * 2.4;
                bx[nb] = win.mx; by[nb] = win.my;
                vx[nb] = C.mcos(a) * 220.0;
                vy[nb] = C.msin(a) * 220.0;
                nb = nb + 1;
                spawned = spawned + 1;
            }
        }

        // ---- update ----
        let i: int = 0;
        while (i < nb) {
            bx[i] = bx[i] + vx[i] * dt;
            by[i] = by[i] + vy[i] * dt;
            if (bx[i] < RADIUS)     { bx[i] = RADIUS;     vx[i] = 0.0 - vx[i]; }
            if (bx[i] > w - RADIUS) { bx[i] = w - RADIUS; vx[i] = 0.0 - vx[i]; }
            if (by[i] < 30.0 + RADIUS) { by[i] = 30.0 + RADIUS; vy[i] = 0.0 - vy[i]; }
            if (by[i] > h - RADIUS) { by[i] = h - RADIUS; vy[i] = 0.0 - vy[i]; }
            i = i + 1;
        }

        // ---- draw ----
        pen.clear(0.08, 0.09, 0.12, 1.0);

        i = 0;
        while (i < nb) {
            pen.circle(bx[i], by[i], RADIUS, palette(i));
            i = i + 1;
        }

        // top bar
        pen.fillRect(0.0, 0.0, w, 30.0, 0x1E2230FF);
        pen.text(10.0, 7.0, "Click to spawn balls | Backspace: undo | Esc: quit", 0xCCCCCCFF);

        // counters
        itoa(num, nb);
        pen.text(w - 200.0, 7.0, "Balls:", 0xFFFFFFFF);
        pen.text(w - 140.0, 7.0, num, 0xFFEE55FF);
        itoa(num, fps as int);
        pen.text(w - 100.0, 7.0, "FPS:", 0xFFFFFFFF);
        pen.text(w - 60.0, 7.0, num, 0x55DD77FF);

        // clear button
        let btnCol: u32 = 0x3A4260FF;
        if (overBtn) btnCol = 0x56628FFF;
        pen.fillRect(btnX, btnY, btnW, btnH, btnCol);
        pen.rect(btnX, btnY, btnW, btnH, 1.0, 0x8892BBFF);
        pen.text(btnX + 22.0, btnY + 8.0, "Clear", 0xFFFFFFFF);

        // cursor ring
        pen.circle(win.mx, win.my, 4.0, 0xFFFFFFAA);

        app.present();
    }

    CMem.free(bx as void*); CMem.free(by as void*);
    CMem.free(vx as void*); CMem.free(vy as void*);
    CMem.free(num as void*);
    app.quit();
    return 0;
}
