// tests/foundation.os
// Smoke test for the foundation modules. Run from the project root (the folder that contains standard/):
//   Osengine.exe tests/foundation.os --execute
// Prints only failures, then a summary. Exit code 0 = all passed.

extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fflush(f: void*) => int;

module Core = import "standard/1/Core.os";
module Mem = import "standard/1/Mem.os";
module Str = import "standard/1/String.os";
module Math = import "standard/1/Math.os";
module Random = import "standard/1/Random.os";
module Time = import "standard/1/Time.os";
module IO = import "standard/1/IO.os";
module Sys = import "standard/1/Sys.os";

let g_pass: int = 0;
let g_fail: int = 0;

function check(cond: bool, name: char*) => void {
    if (cond) {
        g_pass = g_pass + 1;
    } else {
        g_fail = g_fail + 1;
        printf("FAIL: %s\n", name);
        fflush(nullptr);
    }
}

function testCore() => void {
    check(Core.minI64(3, 5) == 3, "core minI64");
    check(Core.maxI64(3, 5) == 5, "core maxI64");
    check(Core.clampI64(10, 0, 5) == 5, "core clampI64 high");
    check(Core.clampI64(-4, 0, 5) == 0, "core clampI64 low");
    check(Core.absI64(-7) == 7, "core absI64");
    check(Core.clampF64(2.5, 0.0, 1.0) == 1.0, "core clampF64");
    let a: i32 = 1;
    let b: i32 = 2;
    Core.swapMem(&a as void*, &b as void*, sizeof(i32));
    check(a == 2 && b == 1, "core swapMem");
}

function testMath() => void {
    check(Math.sqrt(16.0) == 4.0, "math sqrt");
    check(Math.approxEq(Math.sin(Math.PI / 2.0), 1.0, 0.000001), "math sin(pi/2)");
    check(Math.floor(0.0 - 1.5) == (0.0 - 2.0), "math floor negative");
    check(Math.ceil(1.2) == 2.0, "math ceil");
    check(Math.round(2.5) == 3.0, "math round");
    check(Math.isNaN(Math.NAN), "math NAN");
    check(Math.isInf(Math.INF), "math INF");
    check(!Math.isFinite(Math.INF), "math isFinite(INF)");
    check(Math.lerp(10.0, 20.0, 0.25) == 12.5, "math lerp");
    check(Math.mod(0.0 - 1.0, 3.0) == 2.0, "math mod negative");
    check(Math.approxEq(Math.degrees(Math.radians(90.0)), 90.0, 0.000001), "math radians/degrees");
    check(Math.smoothstep(0.0, 1.0, 0.5) == 0.5, "math smoothstep");
}

function testRandom() => void {
    let r1: Random.Random;
    let r2: Random.Random;
    r1.seed(42);
    r2.seed(42);
    let same: bool = true;
    for (let i: int = 0; i < 100; i++) {
        if (r1.nextU64() != r2.nextU64()) same = false;
    }
    check(same, "random same seed -> same sequence");

    let r: Random.Random;
    r.seed(7);
    let inRange: bool = true;
    let floatOk: bool = true;
    for (let i: int = 0; i < 2000; i++) {
        let v = r.range(5, 10);
        if (v < 5 || v >= 10) inRange = false;
        let f = r.nextFloat();
        if (f < 0.0 || f >= 1.0) floatOk = false;
    }
    check(inRange, "random range stays in [5,10)");
    check(floatOk, "random nextFloat stays in [0,1)");
    check(r.range(3, 3) == 3, "random range empty returns a");

    let arr: [8]i32;
    for (let i: int = 0; i < 8; i++) arr[i] = i + 1;
    r.shuffle(&arr as void*, 8, sizeof(i32));
    let sum: i32 = 0;
    for (let i: int = 0; i < 8; i++) sum = sum + arr[i];
    check(sum == 36, "random shuffle keeps elements");
}

function testTime() => void {
    let t1 = Time.monotonicNs();
    let t2 = Time.monotonicNs();
    check(t2 >= t1, "time monotonic");
    let sw: Time.Stopwatch;
    sw.start();
    Time.sleep(60);
    let ms = sw.elapsedMs();
    check(ms >= 50.0 && ms < 500.0, "time sleep(60) measured by stopwatch");
    check(Time.unixSeconds() > 1700000000, "time unixSeconds is after 2023");
    check(Time.unixMs() > 1700000000000, "time unixMs is after 2023");
}

function testMemAndString() => void {
    let b: Mem.Buffer;
    check(b.alloc(16), "mem buffer alloc");
    b.setByte(3, 200);
    check(b.getByte(3) == 200, "mem buffer byte roundtrip");
    check(b.resize(64) && b.getByte(3) == 200, "mem buffer resize keeps data");
    b.free();

    let a: Mem.Arena;
    check(a.init(128), "mem arena init");
    let p1 = a.alloc(10);
    let p2 = a.alloc(10);
    check(p1 != nullptr && p2 != nullptr && p1 != p2, "mem arena distinct allocs");
    a.reset();
    check(a.alloc(10) == p1, "mem arena reset reuses");
    a.free();

    let s: Str.String;
    s.set("  Hello, World  ");
    s.trim();
    check(s.equalsC("Hello, World"), "string trim");
    check(s.indexOf("World") == 7, "string indexOf");
    check(s.startsWith("Hello") && s.endsWith("World"), "string starts/endsWith");
    s.replace("World", "OmniScript");
    check(s.equalsC("Hello, OmniScript"), "string replace");
    s.toUpper();
    check(s.equalsC("HELLO, OMNISCRIPT"), "string toUpper");
    s.set("1234");
    let n: i64 = 0;
    check(s.parseInt(&n) && n == 1234, "string parseInt");
    s.set("12x");
    check(!s.parseInt(&n), "string parseInt rejects junk");
    s.free();
}

function testIOAndSys() => void {
    let text: Str.String;
    check(IO.writeFile("std_test_tmp.txt", "line one\nline two\n"), "io writeFile");
    check(IO.readFile("std_test_tmp.txt", &text), "io readFile");
    check(text.length() == 18, "io file length");

    let f: IO.File;
    check(f.open("std_test_tmp.txt", IO.MODE_READ), "io open");
    let line: Str.String;
    check(f.readLine(&line) && line.equalsC("line one"), "io readLine 1");
    check(f.readLine(&line) && line.equalsC("line two"), "io readLine 2");
    check(!f.readLine(&line), "io readLine at EOF");
    f.close();
    check(IO.deleteFile("std_test_tmp.txt"), "io deleteFile");
    check(!IO.readFile("std_test_tmp.txt", &text), "io readFile missing file fails");
    text.free();
    line.free();

    let v: Str.String;
    check(Sys.setEnv("OS_STD_TEST", "abc"), "sys setEnv");
    check(Sys.env("OS_STD_TEST", &v) && v.equalsC("abc"), "sys env roundtrip");
    check(!Sys.hasEnv("OS_STD_DOES_NOT_EXIST"), "sys hasEnv false");
    check(Sys.cwd(&v) && v.length() > 0, "sys cwd");
    check(Sys.cpuCount() >= 1, "sys cpuCount");
    v.free();
}

function main() => i32 {
    testCore();
    testMath();
    testRandom();
    testTime();
    testMemAndString();
    testIOAndSys();
    printf("%d passed, %d failed\n", g_pass, g_fail);
    fflush(nullptr);
    if (g_fail > 0) return 1;
    return 0;
}
