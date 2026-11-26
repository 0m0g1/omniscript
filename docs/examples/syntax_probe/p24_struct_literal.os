module Console = import "standard/1/Console.os";
struct Pt { x: int; y: int; }
let p: Pt = Pt { x: 1, y: 2 };
if (p.x + p.y == 3) Console.println("p24 struct literal: PASS");
