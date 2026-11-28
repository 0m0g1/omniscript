// Headless runner. Edit the settings below, then run from the project root:
//   .\bin\Release-windows-x86_64\Osengine.exe docs/examples/gb/gb_run.os
//
// MODE 0: run the Blargg ROMs one after another, print PASS / FAIL / TIMEOUT and the serial text.
// MODE 1: write a Gameboy Doctor trace of one cpu_instrs ROM (TRACE_ROM 1..11) to gb_trace.log, then
//         python tools/gameboy-doctor/gameboy-doctor gb_trace.log cpu_instrs <TRACE_ROM>
module Console = import "standard/1/Console.os";
module IO = import "standard/1/IO.os";
module Str = import "standard/1/String.os";
module GB = import "gb.os";

// ---- settings ----
let MODE: int = 0;
let TRACE_ROM: int = 1;
let TRACE_LINES: int = 1300000;
let MAX_FRAMES: int = 6000;
let ROM_ROOT: char* = "docs/examples/gb/roms/gb-test-roms/";

let RBUF: u8[2097152];

// Path of ROM number i (0..10 = cpu_instrs individual 01..11, 11 = instr_timing, 12 = combined cpu_instrs.gb with MBC1), relative to ROM_ROOT.
function romRel(i: int) => char* {
    let r: char* = "";
    if (i == 0) { r = "cpu_instrs/individual/01-special.gb"; }
    if (i == 1) { r = "cpu_instrs/individual/02-interrupts.gb"; }
    if (i == 2) { r = "cpu_instrs/individual/03-op sp,hl.gb"; }
    if (i == 3) { r = "cpu_instrs/individual/04-op r,imm.gb"; }
    if (i == 4) { r = "cpu_instrs/individual/05-op rp.gb"; }
    if (i == 5) { r = "cpu_instrs/individual/06-ld r,r.gb"; }
    if (i == 6) { r = "cpu_instrs/individual/07-jr,jp,call,ret,rst.gb"; }
    if (i == 7) { r = "cpu_instrs/individual/08-misc instrs.gb"; }
    if (i == 8) { r = "cpu_instrs/individual/09-op r,r.gb"; }
    if (i == 9) { r = "cpu_instrs/individual/10-bit ops.gb"; }
    if (i == 10) { r = "cpu_instrs/individual/11-op a,(hl).gb"; }
    if (i == 11) { r = "instr_timing/instr_timing.gb"; }
    if (i == 12) { r = "cpu_instrs/cpu_instrs.gb"; }
    return r;
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

function contains(s: char*, n: int, pat: char*, plen: int) => bool {
    let found: bool = false;
    for (let i: int = 0; i + plen <= n; i = i + 1) {
        let ok: bool = true;
        for (let j: int = 0; j < plen; j = j + 1) {
            if (s[i + j] != pat[j]) { ok = false; }
        }
        if (ok) { found = true; }
    }
    return found;
}

function fullPath(rel: char*, out: Str.String*) => void {
    out.set(ROM_ROOT);
    out.append(rel);
}

// 1 = Passed, 2 = Failed, 0 = timeout, -1 = could not load the ROM.
function runOne(path: char*) => int {
    let n: int = loadFile(path);
    let res: int = -1;
    if (n > 0) {
        let gb: GB.GameBoy;
        if (gb.loadRom(RBUF, n)) {
            res = 0;
            let frames: int = 0;
            let done: bool = false;
            while (!done && frames < MAX_FRAMES) {
                gb.runFrame(70224);
                frames = frames + 1;
                let cnt: int = gb.serialCount();
                if (contains(gb.serialText(), cnt, "Passed", 6)) { res = 1; done = true; }
                if (contains(gb.serialText(), cnt, "Failed", 6)) { res = 2; done = true; }
            }
            Console.println(gb.serialText());
        }
    }
    return res;
}

function runBlargg() => int {
    let passed: int = 0;
    let total: int = 13;
    let path: Str.String;
    for (let i: int = 0; i < total; i = i + 1) {
        fullPath(romRel(i), &path);
        Console.print(romRel(i));
        Console.println(" ...");
        let r: int = runOne(path.cstr());
        if (r == 1) { Console.println("  => PASS"); passed = passed + 1; }
        if (r == 2) { Console.println("  => FAIL"); }
        if (r == 0) { Console.println("  => TIMEOUT (no Passed/Failed text)"); }
        if (r == -1) { Console.println("  => could not load ROM"); }
    }
    Console.print("passed ");
    Console.println(passed);
    return 0;
}

function runTrace() => int {
    let path: Str.String;
    fullPath(romRel(TRACE_ROM - 1), &path);
    let n: int = loadFile(path.cstr());
    if (n <= 0) {
        Console.println("could not load ROM");
        return 1;
    }
    let gb: GB.GameBoy;
    if (!gb.loadRom(RBUF, n)) {
        Console.println("loadRom rejected the ROM");
        return 1;
    }
    gb.setDoctor(true);
    let out: Str.String;
    let log: IO.File;
    if (!log.open("gb_trace.log", IO.MODE_WRITE)) {
        Console.println("could not open gb_trace.log");
        return 1;
    }
    let lines: int = 0;
    while (lines < TRACE_LINES) {
        if (gb.willExecute()) {
            gb.trace(&out);
            lines = lines + 1;
            if (lines % 4096 == 0) {
                log.writeString(&out);
                out.clear();
            }
        }
        gb.step();
    }
    log.writeString(&out);
    log.close();
    Console.print("wrote gb_trace.log, lines: ");
    Console.println(lines);
    Console.println(gb.serialText());
    return 0;
}

function main() => int {
    let rc: int = 0;
    if (MODE == 0) { rc = runBlargg(); }
    if (MODE == 1) { rc = runTrace(); }
    return rc;
}
