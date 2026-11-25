module Console = import "standard/1/Console.os";

function main() => int {
    let a: int = -1;
    if (a < 0) Console.println("q1 negative literal: PASS"); else Console.println("q1 FAIL");
    return 0;
}
