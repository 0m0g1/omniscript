// Generic type-argument inference test.
// Run from the project root:
//   .\bin\Release-windows-x86_64\Osengine.exe generic_inference_test.os
// Expected: every line starts with PASS, then "abc" on the last line.
// size_of<T> returns sizeof(T), so the printed number shows which instance the compiler picked
// (i8=1, i32/f32=4, i64/double=8).

module Console = import "standard/1/Console.os";

function check(name: char*, got: i64, want: i64) => void {
    if (got == want) { Console.print("PASS  "); } else { Console.print("FAIL  "); }
    Console.print(name);
    Console.print("  size=");
    Console.println(got);
}

function size_of<T>(v: T) => i64 { return sizeof(T) as i64; }
function pick<T>(a: T, b: T) => T { return a; }
function first<T>(p: T*) => T { return p[0]; }
function id<T>(v: T) => T { return v; }

let a32: i32 = 1;
let a64: i64 = 2;
let af: f32 = 1.5;
let b8: i8 = 3;

check("i32 variable", size_of(a32), 4);
check("i64 variable", size_of(a64), 8);
check("f32 variable", size_of(af), 4);
check("bare int literal defaults to int", size_of(5), 4);
check("bare float literal is double", size_of(1.5), 8);
check("i8 variable stays i8", size_of(b8), 1);        // relies on the NamedStatement check in Call.cpp

let r1 = pick(a64, 5);                                 // literal must not override i64
check("pick(i64, literal) -> i64", size_of(r1), 8);
let r2 = pick(5, a64);                                 // order must not matter
check("pick(literal, i64) -> i64", size_of(r2), 8);

let r3 = first(&a64);                                  // T* binds T = i64
check("first(&i64) -> i64", size_of(r3), 8);

let r4 = id<i64>(5);                                   // explicit form still works
check("explicit id<i64>(5)", size_of(r4), 8);

let s = id("abc");                                     // T = char*
Console.println(s);
