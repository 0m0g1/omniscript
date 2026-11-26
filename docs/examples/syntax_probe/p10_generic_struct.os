module Console = import "standard/1/Console.os";
struct Box<T> {
    v: T;
    get() => T { return this.v; }
}
function main() => int {
    let b: Box<int>;
    b.v = 42;
    if (b.get() == 42) Console.println("p10 generic struct: PASS"); else Console.println("p10 FAIL");
    return 0;
}
