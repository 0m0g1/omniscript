module Console = import "standard/1/Console.os";
function main() => int { Console.println("p26 main ran without being called: AUTO"); return 0; }
// (no call to main() here on purpose)
