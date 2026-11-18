// docs/examples/test.os
// "0m0g1" decodes into "0m0g1's OS" (hacker scramble), then stays readable while a
// scramble wave sweeps across the letters from right to left every few seconds.
// Esc = quit, R = replay.

module C = import "standard/1/Canvas.os";
module T = import "standard/1/Time.os";
module CMem = import "standard/1/c/mem.os";

let TARGET: char* = "0m0g1's OS";
let TARGET_N: int = 10;
let KEEP: int = 5;                                 // "0m0g1" is the first 5 characters
let POOL: char* = "01ABCDEF#$%&*+<>?/|[]{}=~^";     // scramble glyphs
let POOL_N: int = 26;

let SETTLE: float = 2.7;        // seconds until the intro has finished
let PERIOD: float = 3.5;        // a new wave starts every PERIOD seconds
let SWEEP: float = 1.4;         // seconds a wave takes to cross the text
let SIZE: float = 3.0;          // text scale

let window = {
    width: 800,
    height: 600
}

// RGBA colour as the Pen expects it: 0xRRGGBBAA.
function rgba(r: int, g: int, b: int, a: int) => u32 {
    return ((r as u32) << 24) | ((g as u32) << 16) | ((b as u32) << 8) | (a as u32);
}

// Cheap deterministic pseudo-random number (non-negative).
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

function main() => int {
    let app: C.Canvas;

    if (!app.init()) return 1;

    app.iconPath = "docs/examples/icon.png";

    let win = app.createWindow("0m0g1's OS", window.width, window.height);

    if (win == nullptr) { app.quit(); return 2; }

    let pen = win.context2d();

    let sw: T.Stopwatch;
    sw.start();

    let ch = CMem.malloc(2) as char*;
    ch[1] = 0 as char;

    while (app.frame()) {
        if (win.keyPressed(256)) app.quit();
        if (win.keyPressed(82)) sw.start();          // R: replay

        pen.clear(0.04, 0.05, 0.07, 1.0);            // clear every frame, otherwise the previous frame smears

        let t: float = sw.elapsedSeconds() as float;
        let frame: int = (t * 20.0) as int;          // scramble glyphs change 20x per second

        // Wave position (in letter units), moving from right (past the last letter) to left (past the first).
        let waveOn: bool = false;
        let pos: float = 0.0;
        if (t > SETTLE) {
            let c: int = ((t - SETTLE) / PERIOD) as int;
            let ph: float = (t - SETTLE) - (c as float) * PERIOD;
            if (ph < SWEEP) {
                waveOn = true;
                pos = ((TARGET_N as float) + 1.5) - (ph / SWEEP) * ((TARGET_N as float) + 3.0);
            }
        }

        let s: float = SIZE;
        let gw: float = pen.textWidth("M", s);
        let total: float = gw * (TARGET_N as float);
        let sx: float = ((pen.width() as float) - total) / 2.0;
        let sy: float = ((pen.height() as float) - pen.textHeight(s)) / 2.0;

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
                ch[0] = POOL[hash(i * 977 + frame) % POOL_N];
                let col: u32 = rgba(0, 140, 60, 255);                 // scrambling: dim green
                let lift: float = 0.0;

                if (locked) {
                    ch[0] = TARGET[i];
                    col = rgba(190, 255, 215, 255);                   // settled: pale mint, fully readable
                    if (t - lockT < 0.12) { col = rgba(255, 255, 255, 255); }   // flash when a letter locks

                    if (waveOn) {
                        let d: float = absf((i as float) - pos);
                        if (d < 2.2) {
                            let k: float = 1.0 - d / 2.2;             // 1 at the wave centre, 0 at its edge
                            lift = 10.0 * k;                          // letters hop up as the wave passes
                            col = rgba(60 + ((130.0 * (1.0 - k)) as int), 255, 140 + ((75.0 * (1.0 - k)) as int), 255);
                            if (d < 1.0) {
                                ch[0] = POOL[hash(i * 31 + frame) % POOL_N];   // scrambled inside the wave core
                                col = rgba(0, 255, 120, 255);
                            }
                        }
                    }
                }

                let x: float = sx + (i as float) * gw;
                pen.textScaled(x + 2.0, sy + 2.0 - lift, ch, rgba(0, 40, 20, 255), s);   // drop shadow
                pen.textScaled(x, sy - lift, ch, col, s);
            }
            i = i + 1;
        }

        pen.textScaled(10.0, (pen.height() as float) - 24.0, "Esc: quit | R: replay", rgba(110, 120, 140, 255), 1.0);

        app.present();
    }

    CMem.free(ch as void*);
    return 0;
}
