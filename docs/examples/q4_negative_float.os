module Console = import "standard/1/Console.os";

function main() => int {
    let f: float = -1.5;
    if (f < 0.0) Console.println("q4 negative float: PASS"); else Console.println("q4 FAIL");
    return 0;
}
