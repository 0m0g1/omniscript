// standard/1/tests/int_probe.os
module Con = import "standard/1/Console.os";
module Str = import "standard/1/String.os";

function chk(name: char*, got: i64, want: i64) => void {
    let s: Str.String;
    if (got == want) s.append("ok   "); else s.append("FAIL ");
    s.append(name);
    s.append(" got=");
    s.appendInt(got);
    s.append(" want=");
    s.appendInt(want);
    Con.printlnString(&s);
    s.free();
}

function main() => int {
    let a: u32 = 0xDEADBEEF;
    let c: u32 = 0xFFFFFFFF;
    let b: u64 = 0xFFFFFFFFFFFFFFFF;

    chk("u32 literal as i64", a as i64, 3735928559);
    chk("u32 == hex literal", (a == 0xDEADBEEF) as i64, 1);
    chk("u32 widened to u64", (a as u64) as i64, 3735928559);
    chk("u32 wrap: 0xFFFFFFFF + 2", (c + 2) as i64, 1);
    chk("u32 logical >> 28", (a >> 28) as i64, 13);
    chk("u32 << 4 truncates", (a << 4) as i64, 3940282096);
    chk("u32 rotr 7", ((a >> 7) | (a << 25)) as i64, 3753728893);
    chk("u32 & mask", (a & 0xFF) as i64, 239);
    chk("u32 ^ and ~", ((a ^ 0xFFFFFFFF) == (~a)) as i64, 1);
    chk("u64 max > 1000", (b > 1000) as i64, 1);
    chk("u64 max + 1 == 0", ((b + 1) == 0) as i64, 1);
    chk("u64 max as i64 == -1", b as i64, -1);
    return 0;
}