// Runs dmg-acid2 headless and compares the framebuffer with the reference image.
//   .\bin\Release-windows-x86_64\Osengine.exe docs/examples/gb/gb_acid2.os
//
// Prints the number of differing pixels and an 20x18 map of the 8x8 tiles: '.' = identical,
// a digit = that many wrong pixels in the tile, '#' = more than 9.
module Console = import "standard/1/Console.os";
module IO = import "standard/1/IO.os";
module Str = import "standard/1/String.os";
module IMG = import "standard/1/Image.os";
module GB = import "gb.os";

// ---- settings ----
let ROM_PATH: char* = "docs/examples/gb/roms/dmg-acid2.gb";
let REF_PATH: char* = "docs/examples/gb/roms/dmg-acid2-reference-dmg.png";
let FRAMES: int = 40;

let RBUF: u8[2097152];
let TILE: int[360];

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

// Reference pixel -> shade 0..3. Uses the larger of the two middle bytes of the packed colour, so it
// works for both 0xRRGGBBAA and 0xAARRGGBB with the grey or green DMG palettes.
function refShade(px: u32) => int {
    let a: int = ((px >> 8) & 0xFF) as int;
    let b: int = ((px >> 16) & 0xFF) as int;
    let v: int = a;
    if (b > v) { v = b; }
    let s: int = 3;
    if (v >= 0x40) { s = 2; }
    if (v >= 0x90) { s = 1; }
    if (v >= 0xE0) { s = 0; }
    return s;
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
    for (let f: int = 0; f < FRAMES; f = f + 1) { gb.runFrame(70224); }

    let ref: IMG.Image;
    if (!ref.load(REF_PATH)) {
        Console.print("could not load reference image: ");
        Console.println(REF_PATH);
        return 1;
    }
    if (!ref.in_bounds(159, 143) || ref.in_bounds(160, 144)) {
        Console.println("reference image is not 160x144");
        return 1;
    }
    Console.print("reference pixel (0,0) packed value: ");
    Console.println(ref.get(0, 0) as i64);

    for (let i: int = 0; i < 360; i = i + 1) { TILE[i] = 0; }
    let fb: u8* = gb.framebuffer();
    let bad: int = 0;
    for (let y: int = 0; y < 144; y = y + 1) {
        for (let x: int = 0; x < 160; x = x + 1) {
            let want: int = refShade(ref.get(x, y));
            let got: int = fb[y * 160 + x];
            if (want != got) {
                bad = bad + 1;
                let t: int = (y / 8) * 20 + (x / 8);
                TILE[t] = TILE[t] + 1;
            }
        }
    }
    ref.free();

    Console.print("differing pixels: ");
    Console.println(bad);
    let line: Str.String;
    for (let ty: int = 0; ty < 18; ty = ty + 1) {
        line.clear();
        for (let tx: int = 0; tx < 20; tx = tx + 1) {
            let c: int = TILE[ty * 20 + tx];
            if (c == 0) {
                line.appendChar('.');
            } else if (c > 9) {
                line.appendChar('#');
            } else {
                line.appendChar((48 + c) as char);
            }
        }
        Console.println(&line);
    }
    if (bad == 0) { Console.println("MATCH"); }
    return 0;
}
