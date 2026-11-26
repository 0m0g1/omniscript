module Console = import "standard/1/Console.os";

struct Res {
    id: int;
    ~destructor() { Console.println("destroy"); }
}

fn early(flag: int) -> int {
    let r = Res{ id: 1 };
    if (flag == 1) { return 7; }      // expect: destroy, then return
    return 8;                          // expect: destroy
}

let a = Res{ id: 1 };
let b = Res{ id: 2 };
for (let i: int = 0; i < 2; i++) {
    let t = Res{ id: 10 };
    if (i == 1) { break; }             // expect: destroy
}                                      // i==0: destroy at end of iteration
early(1);
early(0);
Console.println("end");
// expected order at top-level end: ... end, destroy (b), destroy (a)
