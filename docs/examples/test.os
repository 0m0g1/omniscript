struct Inner { n: int = 0; }
struct Outer { s: Inner; k: int = 0; }
function touch(i: Inner*) => void { i.n = 5; }
function main() => int {
    let o: Outer;
    let po: Outer* = &o;
    touch(&po.s);          // &member through a pointer
    return o.s.n;          // expect 5
}