module Console = import "standard/1/Console.os";

function main() => int {
    let a: [3]int;
    a[0] = 1; a[1] = 2; a[2] = 3;
    if (a[0] + a[1] + a[2] == 6) Console.println("p15 fixed array: PASS");
    return 0;
}
