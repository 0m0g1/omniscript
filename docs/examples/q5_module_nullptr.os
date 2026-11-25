module Console = import "standard/1/Console.os";
let g: char* = nullptr;
function main() => int {
    if (g == nullptr) Console.println("q5 module-level nullptr: PASS"); else Console.println("q5 FAIL");
    g = "hello";
    Console.println(g);
    return 0;
}
