module Console = import "standard/1/Console.os";

function main() => int {
    let a: int = 5;
    let b: int = -a;
    if (b == 0 - 5) Console.println("q6 -variable: PASS"); else Console.println("q6 FAIL");
    return 0;
}
