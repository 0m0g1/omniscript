module Console = import "standard/1/Console.os";
function find() => int {
    let i: int = 0;
    while (i < 10) { if (i == 4) { return i; } i = i + 1; }
    return 99;
}
function main() => int {
    if (find() == 4) Console.println("q3 return in loop: PASS"); else Console.println("q3 FAIL");
    return 0;
}
