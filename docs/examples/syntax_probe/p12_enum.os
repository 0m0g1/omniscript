module Console = import "standard/1/Console.os";
enum Color { Red, Green, Blue }
function main() => int {
    let c: Color = Color.Green;
    if (c == Color.Green) Console.println("p12 enum: PASS");
    return 0;
}
