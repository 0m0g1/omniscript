module Console = import "standard/1/Console.os";
function greet(name: char*, times: int = 2) => int { return times; }
if (greet("a") == 2 && greet("a", 5) == 5) Console.println("p25 default arguments: PASS");
