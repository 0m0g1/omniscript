// Play a ROM in a window. Edit ROM_PATH below, then run from the project root:
//   .\bin\Release-windows-x86_64\Osengine.exe docs/examples/gb/gb_play.os
//
// Keys: Arrows = d-pad, Z = A, X = B, Enter = Start, Backspace = Select, Esc = quit.
module Console = import "standard/1/Console.os";
module IO = import "standard/1/IO.os";
module Time = import "standard/1/Time.os";
module C = import "standard/1/Canvas.os";
module GB = import "gb.os";

// ---- settings ----
let ROM_PATH: char* = "docs/examples/gb/roms/dmg-acid2.gb";
let SCALE: int = 3;

let RBUF: u8[2097152];
let PIX: u8[92160];

// Shade 0..3 as R, G, B (light to dark green).
function shadeR(s: int) => int {
    let v: int = 0x08;
    if (s == 0) { v = 0xE0; }
    if (s == 1) { v = 0x88; }
    if (s == 2) { v = 0x34; }
    return v;
}

function shadeG(s: int) => int {
    let v: int = 0x18;
    if (s == 0) { v = 0xF8; }
    if (s == 1) { v = 0xC0; }
    if (s == 2) { v = 0x68; }
    return v;
}

function shadeB(s: int) => int {
    let v: int = 0x20;
    if (s == 0) { v = 0xD0; }
    if (s == 1) { v = 0x70; }
    if (s == 2) { v = 0x56; }
    return v;
}

// Reads a file into RBUF. Returns the byte count, or -1 on failure.
function loadFile(path: char*) => int {
    let f: IO.File;
    let n: int = -1;
    if (f.open(path, IO.MODE_READ)) {
        let sz: i64 = f.size();
        if (sz > 0 && sz <= 2097152) {
            let got: u64 = f.readBytes(RBUF as void*, sz as u64);
            n = got as int;
        }
        f.close();
    }
    return n;
}

// GLFW key code for joypad index 0 Right, 1 Left, 2 Up, 3 Down, 4 A, 5 B, 6 Select, 7 Start.
function keyCode(idx: int) => int {
    let k: int = 0;
    if (idx == 0) { k = 262; }
    if (idx == 1) { k = 263; }
    if (idx == 2) { k = 265; }
    if (idx == 3) { k = 264; }
    if (idx == 4) { k = 90; }
    if (idx == 5) { k = 88; }
    if (idx == 6) { k = 259; }
    if (idx == 7) { k = 257; }
    return k;
}

function main() => int {
    let n: int = loadFile(ROM_PATH);
    if (n <= 0) {
        Console.print("could not read ROM: ");
        Console.println(ROM_PATH);
        return 1;
    }
    let gb: GB.GameBoy;
    if (!gb.loadRom(RBUF, n)) {
        Console.println("loadRom rejected the ROM");
        return 1;
    }

    let app: C.Canvas;
    if (!app.init()) { return 2; }
    let win: C.Win* = app.createWindowEx("Game Boy", 160 * SCALE, 144 * SCALE, C.API_GL);
    if (win == nullptr) {
        app.quit();
        return 3;
    }

    let prevKeys: int = 0;
    let img: u32 = 0;
    let acc: double = 0.0;
    let last: u64 = Time.monotonicNs();
    let frameS: double = 0.0167427;          // 70224 clocks at 4194304 Hz

    while (app.frame()) {
        if (win.isOpen) {
            let pen: C.Pen2D* = win.context2d();

            if (win.keyPressed(256)) { win.close(); }

            // input: only send changes
            let cur: int = 0;
            for (let i: int = 0; i < 8; i = i + 1) {
                if (win.keyDown(keyCode(i))) { cur = cur | (1 << i); }
            }
            for (let i: int = 0; i < 8; i = i + 1) {
                let m: int = 1 << i;
                if ((cur & m) != (prevKeys & m)) { gb.setKey(i, (cur & m) != 0); }
            }
            prevKeys = cur;

            // emulate in real time (up to 4 frames of catch-up)
            let now: u64 = Time.monotonicNs();
            acc = acc + ((now - last) as double) / 1000000000.0;
            last = now;
            if (acc > 0.1) { acc = 0.1; }
            let ran: int = 0;
            while (acc >= frameS && ran < 4) {
                gb.runFrame(70224);
                acc = acc - frameS;
                ran = ran + 1;
            }

            // framebuffer shades -> RGBA
            let fb: u8* = gb.framebuffer();
            for (let i: int = 0; i < 23040; i = i + 1) {
                let s: int = fb[i];
                let o: int = i * 4;
                PIX[o] = shadeR(s) as u8;
                PIX[o + 1] = shadeG(s) as u8;
                PIX[o + 2] = shadeB(s) as u8;
                PIX[o + 3] = 255 as u8;
            }

            // the previous frame's image was already presented: free it, then make this one
            if (img != 0) { pen.freeImage(img); }
            img = pen.createImage(PIX, 160, 144);

            pen.clear(0.0, 0.0, 0.0, 1.0);
            pen.image(img, 0.0, 0.0, (160 * SCALE) as float, (144 * SCALE) as float);
        }
        app.present();
    }

    app.quit();
    return 0;
}
