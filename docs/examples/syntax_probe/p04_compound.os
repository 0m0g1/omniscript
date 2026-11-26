module Console = import "standard/1/Console.os";

function main() => int {
    let a: int = 1;
    a += 4;
    a++;
    a--;
    a -= 1;
    a *= 2;
    if (a == 8) Console.println("p04 += ++ -- -= *=: PASS"); else Console.println("p04 FAIL");
    return 0;
}
