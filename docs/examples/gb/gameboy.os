// docs/examples/gb/gameboy.os
// Game Boy and Game Boy Advance emulator application. The core is picked by file extension:
// ".gba" uses gba.os, anything else uses gb.os.
//
// Run:    .\bin\Release-windows-x86_64\Osengine.exe docs/examples/gb/gameboy.os
//         (set ROM_PATH below, or pass the ROM as an argument to the built program)
// Build:  .\bin\Release-windows-x86_64\Osengine.exe docs/examples/gb/gameboy.os --make -o gameboy.exe
//         then drop a .gb / .gba file onto gameboy.exe, or run: gameboy.exe game.gba
//
// Keys:   Arrows = d-pad, Z = A, X = B, Enter = Start, Backspace = Select, A = L, S = R (GBA)
//         P pause, R reset, Tab (hold) fast-forward, C colour palette (GB), M mute (GB), Esc quit
// Saves:  GB battery-backed games keep their save RAM in "<rom path>.sav" next to the ROM
//         (MBC3 games with a clock also store the clock and catch up on the real time that passed).
//         GBA saves are not supported by the core yet.
// Sound:  GB only. Four-channel APU, 44.1 kHz stereo through OpenAL (gbsound_al.os).
// Carts:  GB: no mapper, MBC1, MBC2, MBC3 (+clock), MBC5, up to 2 MB. GBA: up to 32 MB.
module Console = import "standard/1/Console.os";
module IO = import "standard/1/IO.os";
module Str = import "standard/1/String.os";
module Time = import "standard/1/Time.os";
module C = import "standard/1/Canvas.os";
module GB = import "gb.os";
module GBA = import "gba.os";
module SND = import "gbsound_al.os";

// ---- settings ----
// used when no argument is given; "" = none
// let ROM_PATH: char* = r"docs\examples\gb\roms\pakupakugb.gb";
let ROM_PATH: char* = r"docs\examples\gb\roms\Pokemon_ FireRed Version\Pokemon - FireRed Version (USA, Europe) (Rev 1).gba";
let SCALE: int = 3;

let RBUF: u8[2097152];
let SAVEBUF: u8[131072];
let RTCB: u8[16];
let PIX: u8[92160];
let PAL: u8[48];

function setPal(p: int, s: int, r: int, g: int, b: int) => void {
    let i: int = (p * 4 + s) * 3;
    PAL[i] = r as u8;
    PAL[i + 1] = g as u8;
    PAL[i + 2] = b as u8;
}

// 4 palettes of 4 shades (light to dark): green, grey, pocket, warm.
function initPalettes() => void {
    setPal(0, 0, 0xE0, 0xF8, 0xD0);
    setPal(0, 1, 0x88, 0xC0, 0x70);
    setPal(0, 2, 0x34, 0x68, 0x56);
    setPal(0, 3, 0x08, 0x18, 0x20);
    setPal(1, 0, 0xFF, 0xFF, 0xFF);
    setPal(1, 1, 0xAA, 0xAA, 0xAA);
    setPal(1, 2, 0x55, 0x55, 0x55);
    setPal(1, 3, 0x00, 0x00, 0x00);
    setPal(2, 0, 0xC4, 0xCF, 0xA1);
    setPal(2, 1, 0x8B, 0x95, 0x6D);
    setPal(2, 2, 0x4D, 0x53, 0x3C);
    setPal(2, 3, 0x1F, 0x1F, 0x1F);
    setPal(3, 0, 0xFC, 0xE8, 0xC0);
    setPal(3, 1, 0xD8, 0xA0, 0x60);
    setPal(3, 2, 0x80, 0x50, 0x30);
    setPal(3, 3, 0x20, 0x10, 0x08);
}

function paletteName(p: int) => char* {
    let n: char* = "Palette: green";
    if (p == 1) { n = "Palette: grey"; }
    if (p == 2) { n = "Palette: pocket"; }
    if (p == 3) { n = "Palette: warm"; }
    return n;
}

// True when the path ends in ".gba" (any letter case).
function isGbaPath(path: char*) => bool {
    let n: int = 0;
    while (path[n] != 0 as char) { n = n + 1; }
    let r: bool = false;
    if (n >= 4) {
        let c0: int = path[n - 4];
        let c1: int = path[n - 3];
        let c2: int = path[n - 2];
        let c3: int = path[n - 1];
        // '.' = 46, 'g' = 103, 'b' = 98, 'a' = 97; "| 32" lowercases letters
        r = (c0 == 46 && (c1 | 32) == 103 && (c2 | 32) == 98 && (c3 | 32) == 97);
    }
    return r;
}

// Reads a GB file into RBUF. Returns the byte count, or -1 on failure.
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

// Reads a GBA file straight into the core's ROM buffer. Returns the byte count, or -1 on failure.
function loadGbaFile(adv: GBA.GameBoyAdvance*, path: char*) => int {
    let f: IO.File;
    let n: int = -1;
    if (f.open(path, IO.MODE_READ)) {
        let sz: i64 = f.size();
        if (sz > 0 && sz <= 33554432) {
            let got: u64 = f.readBytes(adv.romPtr() as void*, sz as u64);
            n = got as int;
        }
        f.close();
    }
    return n;
}

// Loads the .sav file (if any) into cart RAM and the MBC3 clock, and remembers it as the last saved state.
function loadSave(gb: GB.GameBoy*, path: char*) => void {
    let n: int = gb.ramBytes;
    for (let i: int = 0; i < 131072; i = i + 1) { SAVEBUF[i] = 0 as u8; }
    if (gb.battery && (n > 0 || gb.rtc)) {
        let f: IO.File;
        if (f.open(path, IO.MODE_READ)) {
            let sz: i64 = f.size();
            let want: i64 = n as i64;
            if (sz < want) { want = sz; }
            if (want > 0) { f.readBytes(gb.ramPtr() as void*, want as u64); }
            if (gb.rtc && sz >= ((n + 16) as i64)) {
                f.readBytes(RTCB as void*, 16 as u64);
                let now: int = Time.unixSeconds() as int;
                gb.rtcImport(RTCB, now);
            }
            f.close();
        }
        let p: u8* = gb.ramPtr();
        for (let i: int = 0; i < n; i = i + 1) { SAVEBUF[i] = p[i]; }
    }
}

// Writes the .sav file if cart RAM (or the clock) changed since the last write. force also writes the
// clock of MBC3 games, so a closed game picks up the right time next start.
function syncSave(gb: GB.GameBoy*, path: char*, force: bool) => void {
    let n: int = gb.ramBytes;
    if (gb.battery && (n > 0 || gb.rtc)) {
        let p: u8* = gb.ramPtr();
        let diff: bool = false;
        if (force && gb.rtc) { diff = true; }
        if (gb.rtcDirty) { diff = true; }
        for (let i: int = 0; i < n; i = i + 1) {
            if (p[i] != SAVEBUF[i]) { diff = true; }
        }
        if (diff) {
            let f: IO.File;
            if (f.open(path, IO.MODE_WRITE)) {
                if (n > 0) { f.writeBytes(p as void*, n as u64); }
                if (gb.rtc) {
                    let now: int = Time.unixSeconds() as int;
                    gb.rtcExport(RTCB, now);
                    f.writeBytes(RTCB as void*, 16 as u64);
                }
                f.close();
                for (let i: int = 0; i < n; i = i + 1) { SAVEBUF[i] = p[i]; }
                gb.rtcDirty = false;
            }
        }
    }
}

// GLFW key code for GB joypad index 0 Right, 1 Left, 2 Up, 3 Down, 4 A, 5 B, 6 Select, 7 Start.
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

// GLFW key code for GBA index 0 A, 1 B, 2 Select, 3 Start, 4 Right, 5 Left, 6 Up, 7 Down, 8 R, 9 L.
function gbaKeyCode(idx: int) => int {
    let k: int = 0;
    if (idx == 0) { k = 90; }
    if (idx == 1) { k = 88; }
    if (idx == 2) { k = 259; }
    if (idx == 3) { k = 257; }
    if (idx == 4) { k = 262; }
    if (idx == 5) { k = 263; }
    if (idx == 6) { k = 265; }
    if (idx == 7) { k = 264; }
    if (idx == 8) { k = 83; }
    if (idx == 9) { k = 65; }
    return k;
}

function main(argc: int, argv: char**) => int {
    initPalettes();
    let romPath: char* = ROM_PATH;
    if (argc > 1) { romPath = argv[1]; }
    let haveRom: bool = false;
    let msg: char* = "";
    let loaded: bool = false;
    let isGba: bool = isGbaPath(romPath);

    let gb: GB.GameBoy;
    let adv: GBA.GameBoyAdvance;
    let savePath: Str.String;
    let winTitle: Str.String;
    winTitle.set("Game Boy");
    if (isGba) { winTitle.set("Game Boy Advance"); }

    if (romPath[0] != 0 as char) {
        haveRom = true;
        if (isGba) {
            let n: int = loadGbaFile(&adv, romPath);
            if (n <= 0) {
                msg = "Could not read the ROM file.";
            } else if (!adv.romReady(n)) {
                msg = "Unsupported GBA ROM.";
            } else {
                loaded = true;
                let name: char* = adv.title();
                if (name[0] != 0 as char) {
                    winTitle.append(" - ");
                    winTitle.append(name);
                }
            }
        } else {
            let n: int = loadFile(romPath);
            if (n <= 0) {
                msg = "Could not read the ROM file.";
            } else if (!gb.loadRom(RBUF, n)) {
                msg = "Unsupported ROM (needs no mapper, MBC1/2/3/5, max 2 MB).";
            } else {
                loaded = true;
                savePath.set(romPath);
                savePath.append(".sav");
                loadSave(&gb, savePath.cstr());
                if (gb.title()[0] != 0 as char) {
                    winTitle.append(" - ");
                    winTitle.append(gb.title());
                }
            }
        }
    }

    // screen size and button count of the selected machine
    let scrW: int = 160;
    let scrH: int = 144;
    let nkeys: int = 8;
    if (isGba) {
        scrW = 240;
        scrH = 160;
        nkeys = 10;
    }

    let app: C.Canvas;
    if (!app.init()) { return 2; }
    let win: C.Win* = app.createWindowEx(winTitle.cstr(), scrW * SCALE, scrH * SCALE, C.API_GL);
    if (win == nullptr) {
        app.quit();
        return 3;
    }

    let snd: SND.PcmOut;
    let sndOk: bool = false;
    if (!isGba) { sndOk = snd.open(44100); }
    let mute: bool = false;
    let prevKeys: int = 0;
    let img: u32 = 0;
    let acc: double = 0.0;
    let rtcAcc: double = 0.0;
    let saveAcc: double = 0.0;
    let helpT: double = 6.0;
    let toast: char* = "";
    let toastT: double = 0.0;
    let paused: bool = false;
    let pal: int = 0;
    let last: u64 = Time.monotonicNs();
    let frameS: double = 0.0167427;          // same frame time on both machines
    let w: float = (scrW * SCALE) as float;
    let h: float = (scrH * SCALE) as float;

    while (app.frame()) {
        if (win.isOpen) {
            let pen: C.Pen2D* = win.context2d();

            let now: u64 = Time.monotonicNs();
            let dt: double = ((now - last) as double) / 1000000000.0;
            last = now;
            if (dt > 0.1) { dt = 0.1; }

            if (win.keyPressed(256)) { win.close(); }

            if (loaded) {
                if (win.keyPressed(80)) {
                    paused = !paused;
                    if (paused) { toast = "Paused"; } else { toast = "Resumed"; }
                    toastT = 1.5;
                }
                if (win.keyPressed(82)) {
                    if (isGba) {
                        adv.reset();
                    } else {
                        syncSave(&gb, savePath.cstr(), false);
                        gb.reset();
                        if (gb.battery && gb.ramBytes > 0) {
                            let rp: u8* = gb.ramPtr();
                            for (let i: int = 0; i < gb.ramBytes; i = i + 1) { rp[i] = SAVEBUF[i]; }
                        }
                    }
                    prevKeys = 0;
                    toast = "Reset";
                    toastT = 1.5;
                }
                if (!isGba && win.keyPressed(67)) {
                    pal = (pal + 1) % 4;
                    toast = paletteName(pal);
                    toastT = 1.5;
                }
                if (!isGba && win.keyPressed(77)) {
                    mute = !mute;
                    if (mute) { toast = "Sound off"; } else { toast = "Sound on"; }
                    toastT = 1.5;
                }

                // joypad: only send changes
                let cur: int = 0;
                for (let i: int = 0; i < nkeys; i = i + 1) {
                    let code: int = keyCode(i);
                    if (isGba) { code = gbaKeyCode(i); }
                    if (win.keyDown(code)) { cur = cur | (1 << i); }
                }
                for (let i: int = 0; i < nkeys; i = i + 1) {
                    let m: int = 1 << i;
                    if ((cur & m) != (prevKeys & m)) {
                        if (isGba) {
                            adv.setKey(i, (cur & m) != 0);
                        } else {
                            gb.setKey(i, (cur & m) != 0);
                        }
                    }
                }
                prevKeys = cur;

                // emulation: how many frames to run now
                let todo: int = 0;
                if (!paused) {
                    if (win.keyDown(258)) {
                        todo = 6;
                        acc = 0.0;
                    } else {
                        acc = acc + dt;
                        if (acc > 0.1) { acc = 0.1; }
                        while (acc >= frameS && todo < 4) {
                            acc = acc - frameS;
                            todo = todo + 1;
                        }
                    }
                } else {
                    acc = 0.0;
                }
                for (let k: int = 0; k < todo; k = k + 1) {
                    if (isGba) {
                        adv.runFrame(280896);          // 228 lines of 1232 clocks
                    } else {
                        gb.runFrame(70224);
                    }
                }

                if (!isGba) {
                    // sound: hand this frame's samples to the output device (dropped when muted, paused or fast-forwarding)
                    let na: int = gb.audioCount();
                    if (na > 0) {
                        if (sndOk && !mute && !paused && !win.keyDown(258)) { snd.push(gb.audioPtr(), na); }
                        gb.audioClear();
                    }

                    // MBC3 real-time clock follows the wall clock
                    rtcAcc = rtcAcc + dt;
                    if (rtcAcc >= 1.0) {
                        let secs: int = rtcAcc as int;
                        gb.rtcAdvance(secs);
                        rtcAcc = rtcAcc - (secs as double);
                    }

                    // battery save every 5 seconds when the RAM changed
                    saveAcc = saveAcc + dt;
                    if (saveAcc >= 5.0) {
                        saveAcc = 0.0;
                        syncSave(&gb, savePath.cstr(), false);
                    }

                    // framebuffer shades -> RGBA using the chosen palette
                    let fb: u8* = gb.framebuffer();
                    let po: int = pal * 12;
                    for (let i: int = 0; i < 23040; i = i + 1) {
                        let s: int = fb[i];
                        let o: int = i * 4;
                        let q: int = po + s * 3;
                        PIX[o] = PAL[q];
                        PIX[o + 1] = PAL[q + 1];
                        PIX[o + 2] = PAL[q + 2];
                        PIX[o + 3] = 255 as u8;
                    }
                }

                // the previous frame's image was already presented: free it, then make this one
                if (img != 0) { pen.freeImage(img); }
                if (isGba) {
                    img = pen.createImage(adv.framebuffer(), 240, 160);   // already RGBA
                } else {
                    img = pen.createImage(PIX, 160, 144);
                }
            }

            pen.clear(0.0, 0.0, 0.0, 1.0);
            if (loaded) {
                pen.image(img, 0.0, 0.0, w, h);
                if (paused) {
                    pen.fillRect(0.0, 0.0, w, 26.0, 0x000000B0);
                    pen.textScaled(10.0, 5.0, "PAUSED", 0xFFFFFFFF, 2.0);
                } else if (toastT > 0.0) {
                    pen.fillRect(0.0, 0.0, w, 26.0, 0x000000B0);
                    pen.textScaled(10.0, 5.0, toast, 0xFFFFFFFF, 2.0);
                }
                if (helpT > 0.0) {
                    pen.fillRect(0.0, h - 34.0, w, 34.0, 0x000000B0);
                    pen.text(8.0, h - 30.0, "Arrows  Z=A  X=B  Enter=Start  Bksp=Select", 0xFFFFFFFF);
                    if (isGba) {
                        pen.text(8.0, h - 16.0, "A=L  S=R  P pause  R reset  Tab fast  Esc quit", 0xFFFFFFFF);
                    } else {
                        pen.text(8.0, h - 16.0, "P pause  R reset  Tab fast  C colours  M mute  Esc quit", 0xFFFFFFFF);
                    }
                }
            } else {
                pen.textScaled(20.0, 40.0, "Game Boy", 0xE0F8D0FF, 3.0);
                if (haveRom) {
                    pen.textScaled(20.0, 110.0, msg, 0xFF6666FF, 2.0);
                    pen.text(20.0, 150.0, romPath, 0xAAAAAAFF);
                } else {
                    pen.textScaled(20.0, 110.0, "No ROM loaded.", 0xFFFFFFFF, 2.0);
                    pen.text(20.0, 150.0, "Drop a .gb or .gba file onto gameboy.exe,", 0xAAAAAAFF);
                    pen.text(20.0, 166.0, "or run:  gameboy.exe game.gba", 0xAAAAAAFF);
                }
            }
            if (toastT > 0.0) { toastT = toastT - dt; }
            if (helpT > 0.0) { helpT = helpT - dt; }
        }
        app.present();
    }

    if (loaded && !isGba) { syncSave(&gb, savePath.cstr(), true); }
    // report what the GBA core could not handle
    if (loaded && isGba) {
        Console.print("undefined instructions: ");
        Console.println(adv.badOps);
        Console.print("unimplemented BIOS calls: ");
        Console.println(adv.swiMiss);
        Console.print("last unimplemented BIOS call: ");
        Console.println(adv.lastSwi);
        Console.print("PC: ");
        Console.println(adv.getPc());
    }
    snd.close();
    app.quit();
    return 0;
}
