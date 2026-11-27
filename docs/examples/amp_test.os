// docs\examples\amp_test.os -- does a binary operator on structs need an explicit `&` on the right operand?
// Run as is (test A must print 5). Then uncomment ONE of B / C at a time and read the compiler's answer.
module C = import "standard/1/Console.os";

struct Num {
    v: int = 0;
    can + (o: Num*) => int { return this.v + o.v; }     // pointer parameter, as in ownership_demo.os section 17
    can * (o: Num&) => int { return this.v * o.v; }  // test C: reference parameter (uncomment together with C below)
}

function main() => int {
    let a: Num;
    let b: Num;
    a.v = 2;
    b.v = 3;

    // A. with `&`: known to work (ownership_demo section 17)
    // let r1: int = a + &b;
    // C.println(`A  a + &b = ${r1}`);                       // expect 5

    // B. without `&`: expected to be a type error (Num is not Num*)
    // let r2: int = a + b;
    // C.println(`B  a + b = ${r2}`);

    // C. reference parameter: only if `can * (o: Num&)` above is uncommented; call site has no `&`
    let r3: int = a * b;
    C.println(`C  a * b = ${r3}`);                      // 6 would mean references make `a * b` work

    return 0;
}
