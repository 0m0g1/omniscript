module Console = import "standard/1/Console.os";

function main() => int {
    let a: int = 6;
    if ((a & 3) == 2 && (a | 1) == 7 && (a ^ 2) == 4 && (1 << 3) == 8 && (16 >> 2) == 4) Console.println("p09 bit ops: PASS"); else Console.println("p09 FAIL");
    return 0;
}
