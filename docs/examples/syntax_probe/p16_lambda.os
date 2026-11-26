module Console = import "standard/1/Console.os";

function main() => int {
    let f = function(x: int) => int { return x + 1; }
    if (f(1) == 2) Console.println("p16 lambda: PASS");
    return 0;
}
