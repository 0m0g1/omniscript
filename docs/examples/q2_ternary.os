module Console = import "standard/1/Console.os";

function main() => int {
    let c: bool = true;
    let r: int = c ? 10 : 20;
    if (r == 10) Console.println("q2 ternary: PASS"); else Console.println("q2 FAIL");
    return 0;
}
