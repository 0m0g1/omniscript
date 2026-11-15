// standard/1/tests/test_core.os  (Tier 1: Core, Math, Random, Time, Mem, String)
// API usage is taken from foundation.os (known to work): structs are declared by value
// (`let r: Rnd.Random;`) and initialised with seed()/init()/set(), not `.new()`.
module H = import "standard/1/tests/Harness.os";
module Core = import "standard/1/Core.os";
module Math = import "standard/1/Math.os";
module Str = import "standard/1/String.os";
module Rnd = import "standard/1/Random.os";
module Time = import "standard/1/Time.os";
module Mem = import "standard/1/Mem.os";

function main() => int {
    H.suite("Core");
    H.check(Core.minI64(3, 7) == 3, "minI64");
    H.check(Core.maxI64(3, 7) == 7, "maxI64");
    H.check(Core.clampI64(15, 0, 10) == 10, "clampI64 high");
    H.check(Core.clampI64(0 - 5, 0, 10) == 0, "clampI64 low");
    H.check(Core.clampI64(5, 0, 10) == 5, "clampI64 inside");
    H.check(Core.absI64(0 - 7) == 7, "absI64");
    H.check(Core.minF64(1.5, 2.5) == 1.5, "minF64");
    H.check(Core.maxF64(1.5, 2.5) == 2.5, "maxF64");
    H.check(Core.clampF64(2.5, 0.0, 1.0) == 1.0, "clampF64");
    let a: i32 = 1;
    let b: i32 = 2;
    Core.swapMem(&a as void*, &b as void*, sizeof(i32));
    H.check(a == 2 && b == 1, "swapMem");

    H.suite("Math");
    H.check(Math.sqrt(16.0) == 4.0, "sqrt");
    H.check(Math.approxEq(Math.sin(Math.PI / 2.0), 1.0, 0.000001), "sin(pi/2)");
    H.check(Math.floor(0.0 - 1.5) == (0.0 - 2.0), "floor negative");
    H.check(Math.ceil(1.2) == 2.0, "ceil");
    H.check(Math.round(2.5) == 3.0, "round");
    H.check(Math.isNaN(Math.NAN), "NAN");
    H.check(Math.isInf(Math.INF), "INF");
    H.check(!Math.isFinite(Math.INF), "isFinite(INF)");
    H.check(Math.lerp(10.0, 20.0, 0.25) == 12.5, "lerp");
    H.check(Math.modulo(0.0 - 1.0, 3.0) == 2.0, "modulo negative");
    H.check(Math.approxEq(Math.degrees(Math.radians(90.0)), 90.0, 0.000001), "radians/degrees");
    H.check(Math.smoothstep(0.0, 1.0, 0.5) == 0.5, "smoothstep");

    H.suite("Random");
    let r1: Rnd.Random;
    let r2: Rnd.Random;
    r1.seed(42);
    r2.seed(42);
    let same: bool = true;
    let i: int = 0;
    while (i < 100) {
        if (r1.nextU64() != r2.nextU64()) same = false;
        i = i + 1;
    }
    H.check(same, "same seed = same sequence");
    let r: Rnd.Random;
    r.seed(7);
    let inRange: bool = true;
    let floatOk: bool = true;
    i = 0;
    while (i < 2000) {
        let v = r.range(5, 10);
        if (v < 5 || v >= 10) inRange = false;
        let f = r.nextFloat();
        if (f < 0.0 || f >= 1.0) floatOk = false;
        i = i + 1;
    }
    H.check(inRange, "range(5,10) stays in [5,10)");
    H.check(floatOk, "nextFloat stays in [0,1)");
    H.check(r.range(3, 3) == 3, "range of an empty interval returns a");
    let arr: [8]i32;
    i = 0;
    while (i < 8) { arr[i] = i + 1; i = i + 1; }
    r.shuffle(&arr as void*, 8, sizeof(i32));
    let sum: i32 = 0;
    i = 0;
    while (i < 8) { sum = sum + arr[i]; i = i + 1; }
    H.check(sum == 36, "shuffle keeps the elements");

    H.suite("Time");
    let t1 = Time.monotonicNs();
    let t2 = Time.monotonicNs();
    H.check(t2 >= t1, "monotonic clock");
    let sw: Time.Stopwatch;
    sw.start();
    Time.sleep(60);
    let ms = sw.elapsedMs();
    H.check(ms >= 50.0 && ms < 500.0, "sleep(60) measured by stopwatch");
    H.check(Time.unixSeconds() > 1700000000, "unixSeconds is after 2023");
    H.check(Time.unixMs() > 1700000000000, "unixMs is after 2023");

    H.suite("Mem");
    let buf: Mem.Buffer;
    H.check(buf.alloc(16), "buffer alloc");
    buf.setByte(3, 200);
    H.check(buf.getByte(3) == 200, "buffer byte roundtrip");
    H.check(buf.resize(64) && buf.getByte(3) == 200, "buffer resize keeps data");
    buf.free();
    let arena: Mem.Arena;
    H.check(arena.init(128), "arena init");
    let p1 = arena.alloc(10);
    let p2 = arena.alloc(10);
    H.check(p1 != nullptr && p2 != nullptr && p1 != p2, "arena distinct allocs");
    arena.reset();
    H.check(arena.alloc(10) == p1, "arena reset reuses memory");
    arena.free();

    H.suite("String");
    let s: Str.String;
    s.set("  Hello, World  ");
    s.trim();
    H.check(s.equalsC("Hello, World"), "trim");
    H.check(s.length() == 12, "length");
    H.check(s.indexOf("World") == 7, "indexOf");
    H.check(s.startsWith("Hello") && s.endsWith("World"), "startsWith / endsWith");
    s.replace("World", "OmniScript");
    H.check(s.equalsC("Hello, OmniScript"), "replace");
    s.toUpper();
    H.check(s.equalsC("HELLO, OMNISCRIPT"), "toUpper");
    s.set("1234");
    let n: i64 = 0;
    H.check(s.parseInt(&n) && n == 1234, "parseInt");
    s.set("12x");
    H.check(!s.parseInt(&n), "parseInt rejects junk");
    s.free();

    return H.finish();
}
