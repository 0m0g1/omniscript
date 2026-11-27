// docs\examples\amp_test_c.os -- reference parameter: `can * (o: Num&)` called as `a * b`. Expect 6.
module C = import "standard/1/Console.os";

struct Num {
    v: int = 0;
    can * (o: Num&) => int { return this.v * o.v; }
}

function main() => int {
    let a: Num;
    let b: Num;
    a.v = 2;
    b.v = 3;
    let r: int = a * b;
    C.println(`C  a * b = ${r}   (a.v=${a.v} b.v=${b.v}, should still be 2 and 3)`);
    return 0;
}
