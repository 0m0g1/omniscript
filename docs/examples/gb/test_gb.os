// Smoke test for gb.os. Run from the project root:
//   .\bin\Release-windows-x86_64\Osengine.exe docs/examples/gb/test_gb.os
module Console = import "standard/1/Console.os";
module GB = import "gb.os";

let TROM: u8[32768];
let FAILS: int = 0;

function put(addr: int, v: int) => void {
    TROM[addr] = v as u8;
}

function check(name: char*, ok: bool) => void {
    if (ok) {
        Console.print("PASS ");
    } else {
        Console.print("FAIL ");
        FAILS = FAILS + 1;
    }
    Console.println(name);
}

function main() => int {
    for (let i: int = 0; i < 32768; i = i + 1) { TROM[i] = 0 as u8; }

    // 0100: LD A,'O' ; LDH (01),A ; LD A,0x81 ; LDH (02),A   -> serial 'O'
    put(0x100, 0x3E); put(0x101, 0x4F); put(0x102, 0xE0); put(0x103, 0x01);
    put(0x104, 0x3E); put(0x105, 0x81); put(0x106, 0xE0); put(0x107, 0x02);
    // 0108: LD A,5 ; ADD A,3 ; CALL 0120
    put(0x108, 0x3E); put(0x109, 0x05); put(0x10A, 0xC6); put(0x10B, 0x03);
    put(0x10C, 0xCD); put(0x10D, 0x20); put(0x10E, 0x01);
    // 010F: LD A,'K' ; LDH (01),A ; LD A,0x81 ; LDH (02),A ; JR -2
    put(0x10F, 0x3E); put(0x110, 0x4B); put(0x111, 0xE0); put(0x112, 0x01);
    put(0x113, 0x3E); put(0x114, 0x81); put(0x115, 0xE0); put(0x116, 0x02);
    put(0x117, 0x18); put(0x118, 0xFE);
    // 0120: ADD A,0x30 ; LDH (01),A ; LD A,0x81 ; LDH (02),A ; RET   -> serial '8' (5+3+0x30)
    put(0x120, 0xC6); put(0x121, 0x30); put(0x122, 0xE0); put(0x123, 0x01);
    put(0x124, 0x3E); put(0x125, 0x81); put(0x126, 0xE0); put(0x127, 0x02);
    put(0x128, 0xC9);

    let gb: GB.GameBoy;
    check("loadRom", gb.loadRom(TROM, 32768));
    check("start pc", gb.pc == 0x100);
    check("start a", gb.a == 0x01);
    check("start sp", gb.sp == 0xFFFE);

    let cyc: int = gb.step();
    check("LD A,n takes 8 cycles", cyc == 8);
    check("LD A,n loads", gb.a == 0x4F);

    for (let i: int = 0; i < 5; i = i + 1) { gb.step(); }
    check("ADD A,3 result", gb.a == 8);
    check("ADD A,3 flags clear", gb.f == 0);

    cyc = gb.step();
    check("CALL takes 24 cycles", cyc == 24);
    check("CALL target", gb.pc == 0x120);
    check("CALL pushed", gb.sp == 0xFFFC);

    for (let i: int = 0; i < 60; i = i + 1) { gb.step(); }
    let s: char* = gb.serialText();
    check("serial count", gb.serialCount() == 3);
    check("serial 'O'", s[0] == 'O');
    check("serial '8'", s[1] == '8');
    check("serial 'K'", s[2] == 'K');

    gb.runFrame(70224);
    check("frame reached VBlank", gb.frameDone);
    check("width", gb.width() == 160);
    check("height", gb.height() == 144);

    if (FAILS == 0) {
        Console.println("ALL PASSED");
    } else {
        Console.println("SOME TESTS FAILED");
    }
    return FAILS;
}
