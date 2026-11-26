module Console = import "standard/1/Console.os";

let a: char* = "abc";
let b: char* = "abc";
if (a == b) Console.println("p20 string == compares contents: PASS"); else Console.println("p20 string == is pointer compare (FAIL means use streq)");
