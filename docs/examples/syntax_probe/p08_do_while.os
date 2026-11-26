module Console = import "standard/1/Console.os";

function main() => int {
    let i: int = 0;
    do { i = i + 1; } while (i < 3);
    if (i == 3) Console.println("p08 do/while: PASS");
    return 0;
}
