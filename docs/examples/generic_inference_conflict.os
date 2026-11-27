// EXPECTED: a compile error, and nothing is printed.
// The message should say: Cannot infer the type arguments of 'pick': 'T' would be both i32 and i64.
// Run:  .\bin\Release-windows-x86_64\Osengine.exe generic_inference_conflict.os

module Console = import "standard/1/Console.os";

function pick<T>(a: T, b: T) => T { return a; }

let x: i32 = 1;
let y: i64 = 2;
let r = pick(x, y);
Console.println("BUG: this should not run");
