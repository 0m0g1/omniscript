// docs/examples/defaults_probe.os
// Checks whether non-zero struct field defaults survive `let x: T;` (a plain local, no arguments).
// Run:  .\bin\Release-windows-x86_64\Osengine.exe docs\examples\defaults_probe.os
//
// Each line prints a number. Expected values are in the comments; 0 where a non-zero value is expected
// means that default was lost. The exit code is the number of lost defaults (0 = all fine).
module Tm = import "standard/1/Time.os";
module C = import "standard/1/Console.os";

struct Probe {
    a: int = 7;
    b: double = 0.04;
    c: float = 0.04;
    d: bool = true;
    e: i64 = 123456789;
    z: int = 0;                  // zero default: should always read 0
}

function main() => int {
    let lost: int = 0;

    // 1. A struct declared in this file.
    let p: Probe;
    C.log(p.a);                          // 7
    C.log((p.b * 1000.0) as int);        // 40
    C.log((p.c * 1000.0) as int);        // 40
    C.log(p.d as int);                   // 1
    C.log(p.e as int);                   // 123456789
    C.log(p.z);                          // 0
    if (p.a != 7) lost = lost + 1;
    if ((p.b * 1000.0) as int != 40) lost = lost + 1;
    if ((p.c * 1000.0) as int != 40) lost = lost + 1;
    if (!p.d) lost = lost + 1;
    if (p.e as int != 123456789) lost = lost + 1;

    // 2. A struct from an imported module (the case that froze the particles).
    let ft: Tm.FrameTimer;
    C.log((ft.maxDt * 1000.0) as int);   // 40
    if ((ft.maxDt * 1000.0) as int != 40) lost = lost + 1;

    // 3. Struct literal with an explicit value, to confirm initialisers still work.
    let q = Probe{ a: 99 };
    C.log(q.a);                          // 99
    C.log((q.b * 1000.0) as int);        // 40: fields not mentioned in the literal should keep their defaults
    if (q.a != 99) lost = lost + 1;
    if ((q.b * 1000.0) as int != 40) lost = lost + 1;

    C.log(lost);                         // 0 when everything is fine
    return lost;
}
