module Console = import "standard/1/Console.os";
module CMem = import "standard/1/c/mem.os";
struct Pt { x: int = 7; y: int = 9; }
function main() => int {
    let p: Pt* = CMem.malloc(sizeof(Pt)) as Pt*;
    Console.println("q8 malloc struct defaults (16 = run, else garbage/0):");
    Console.println(p.x + p.y);
    return 0;
}
