/// Smoke test for gb.os. Needs no ROM file: it assembles a tiny 32 KB ROM in memory.
/// The ROM prints "O5K" over the serial port using LD, LDH, ADD, CALL and RET.
/// Run from the folder that holds gb.os:   os test_gb.os

module Console = import "standard/1/Console.os";
module GB = import "gb.os";

let fails: int = 0;

function check(name: char*, ok: bool) => void {
    if (ok) {
        Console.print("PASS  ");
    } else {
        Console.print("FAIL  ");
        fails = fails + 1;
    }
    Console.println(name);
}

function put(p: u8*, at: int, v: int) => void {
    p[at] = v as u8;
}

function main() => int {
    let rom: u8[32768];
    let i: int = 0;
    while (i < 32768) { rom[i] = 0 as u8; i = i + 1; }
    // header bytes 0x147 (type), 0x148 (size), 0x149 (ram) stay 0 = ROM only, 32 KB

    // 0100: print 'O'
    put(rom, 0x100, 0x3E); put(rom, 0x101, 0x4F);   // LD A,'O'
    put(rom, 0x102, 0xE0); put(rom, 0x103, 0x01);   // LDH (FF01),A
    put(rom, 0x104, 0x3E); put(rom, 0x105, 0x81);   // LD A,0x81
    put(rom, 0x106, 0xE0); put(rom, 0x107, 0x02);   // LDH (FF02),A
    // 0108: 0x30 + 5 = '5'
    put(rom, 0x108, 0x3E); put(rom, 0x109, 0x30);   // LD A,0x30
    put(rom, 0x10A, 0xC6); put(rom, 0x10B, 0x05);   // ADD A,5
    put(rom, 0x10C, 0xE0); put(rom, 0x10D, 0x01);   // LDH (FF01),A
    put(rom, 0x10E, 0x3E); put(rom, 0x10F, 0x81);   // LD A,0x81
    put(rom, 0x110, 0xE0); put(rom, 0x111, 0x02);   // LDH (FF02),A
    // 0112: call subroutine that prints 'K'
    put(rom, 0x112, 0xCD); put(rom, 0x113, 0x20); put(rom, 0x114, 0x01);   // CALL 0120
    put(rom, 0x115, 0x18); put(rom, 0x116, 0xFE);   // JR -2 (spin)
    // 0120: subroutine
    put(rom, 0x120, 0x3E); put(rom, 0x121, 0x4B);   // LD A,'K'
    put(rom, 0x122, 0xE0); put(rom, 0x123, 0x01);   // LDH (FF01),A
    put(rom, 0x124, 0x3E); put(rom, 0x125, 0x81);   // LD A,0x81
    put(rom, 0x126, 0xE0); put(rom, 0x127, 0x02);   // LDH (FF02),A
    put(rom, 0x128, 0xC9);                          // RET

    let gb: GB.GameBoy;

    check("loadRom accepts a ROM-only cartridge", gb.loadRom(rom, 32768));
    check("start state: PC=0100, SP=FFFE, A=01", gb.pc == 0x100 && gb.sp == 0xFFFE && gb.a == 0x01);

    let c: int = gb.step();
    check("first step (LD A,n) takes 8 clock cycles", c == 8);
    check("A holds 'O' after it", gb.a == 0x4F);

    let n: int = 0;
    while (gb.serialCount() < 3 && n < 2000) {
        gb.step();
        n = n + 1;
    }
    check("serial log holds 3 bytes", gb.serialCount() == 3);
    check("serial text is O5K", gb.serialText() == "O5K");
    Console.print("serial output: ");
    Console.println(gb.serialText());

    let spent: int = gb.runFrame(70224);
    check("runFrame spends at least 70224 cycles", spent >= 70224);
    check("VBlank was reached (frameDone)", gb.frameDone);
    check("LY stays within 0..153", gb.ly >= 0 && gb.ly <= 153);
    check("framebuffer is 160x144", gb.width() == 160 && gb.height() == 144);

    if (fails == 0) {
        Console.println("ALL PASS");
    } else {
        Console.print("failures: ");
        Console.println(fails as i64);
    }
    return fails;
}
