module Console = import "standard/1/Console.os";

function main() => int {
    let s: int = 0;
    for (let i: int = 0; i < 5; i = i + 1) { s = s + i; }
    if (s == 10) Console.println("p06 for loop: PASS"); else Console.println("p06 FAIL");
    return 0;
}
