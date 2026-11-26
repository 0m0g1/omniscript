module Console = import "standard/1/Console.os";
struct Pt { x: int; y: int; Pt(a: int, b: int) { this.x = a; this.y = b; } }
let p: Pt* = new Pt(3, 4);
if (p.x + p.y == 7) Console.println("p23 new with constructor: PASS"); else Console.println("p23 FAIL");
delete p;
