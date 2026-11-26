module Console = import "standard/1/Console.os";

function main() => int {
    let i: int = 0;
    let n: int = 0;
    while (true) {
        i = i + 1;
        if (i > 8) break;
        if (i % 2 == 0) continue;
        n = n + 1;
    }
    if (n == 4) Console.println("p05 break/continue: PASS"); else Console.println("p05 FAIL");
    return 0;
}
