module Console = import "standard/1/Console.os";
module Util = import "standard/1/String.os";
function hello() => void { Console.println("hi"); }
function call(f: fn() => void) => void { f(); }
function main() => int {
    call(hello);
    Console.println("q9 function pointer by name: PASS");
    return 0;
}
