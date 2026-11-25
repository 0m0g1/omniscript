module Console = import "standard/1/Console.os";
struct Pt { x: int = 7; y: int = 9; }
function main() => int {
    let p: Pt;
    Console.println("q7 stack struct defaults (want 16):");
    Console.println(p.x + p.y);
    return 0;
}
