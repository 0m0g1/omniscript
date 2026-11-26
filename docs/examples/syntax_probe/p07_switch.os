module Console = import "standard/1/Console.os";

function main() => int {
    let x: int = 2;
    let r: int = 0;
    switch (x) {
        case 1: r = 10; break;
        case 2: r = 20; break;
        default: r = 30;
    }
    if (r == 20) Console.println("p07 switch: PASS"); else Console.println("p07 FAIL");
    return 0;
}
