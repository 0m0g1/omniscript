# OmniScript Documentation

## Introduction
OmniScript is a high-level, JavaScript-like programming language that supports both high-level and low-level capabilities. It is designed to be compiled using LLVM and supports cross-platform functionality, including mobile, desktop, and embedded systems.

OmniScript is an object-oriented language where everything, including functions, is treated as objects with built-in methods. This documentation covers the core features of OmniScript, including its syntax, libraries, and usage.

## Table of Contents
- [OmniScript Documentation](#omniscript-documentation)
  - [Introduction](#introduction)
  - [Table of Contents](#table-of-contents)
  - [Getting Started](#getting-started)
    - [Prerequisites](#prerequisites)
    - [Installation](#installation)
  - [Language Syntax](#language-syntax)
    - [Variables and Constants](#variables-and-constants)

## Getting Started
To start using OmniScript, you need to install the compiler and set up the development environment. 

### Prerequisites
- LLVM (via MSYS2 or other setups)
- A compatible editor (e.g., Visual Studio Code)
- A basic understanding of C++ (for building the compiler)

### Installation
1. Clone the OmniScript repository from GitHub.
2. Follow the instructions for setting up the build environment.
3. Run the compiler using the command:
    ```bash
    ./build_omniscript.sh
    ```

## Language Syntax

### Variables and Constants
OmniScript supports both mutable and immutable variables. Variables are declared using the `let` keyword, while constants are declared with the `const` keyword.

```omn
let myVariable = 10;   // Mutable variable
const PI = 3.14159;    // Immutable constant
Control Flow
If Statements
omn
Copy code
if (x > 0) {
    console.log("Positive");
} else {
    console.log("Non-positive");
}
For Loops
OmniScript supports the traditional for loop syntax:

omn
Copy code
for (let i = 0; i < 10; i++) {
    console.log(i);
}
While Loops
omn
Copy code
while (x < 10) {
    x++;
}
Functions
Functions are first-class objects in OmniScript. They can be defined and called just like any other object. Functions are declared using the function keyword:

omn
Copy code
function add(a, b) {
    return a + b;
}
Functions can also be stored as objects:

omn
Copy code
let add = function(a, b) {
    return a + b;
};
console.log(add(2, 3)); // Output: 5
Classes and Objects
OmniScript allows the creation of objects and classes similar to JavaScript's class syntax.

omn
Copy code
class Person {
    constructor(name) {
        this.name = name;
    }

    greet() {
        console.log("Hello, " + this.name);
    }
}

let person = new Person("Alice");
person.greet(); // Output: Hello, Alice
Error Handling
OmniScript supports basic try-catch error handling:

omn
Copy code
try {
    let result = riskyFunction();
} catch (error) {
    console.log("An error occurred: " + error.message);
}
Standard Library
Console
OmniScript includes a set of console methods that emulate JavaScript's console API:

omn
Copy code
console.log("This is a log message.");
console.error("This is an error message.");
console.warn("This is a warning message.");
Math
The Math object provides common mathematical functions:

omn
Copy code
let result = Math.max(10, 20); // Returns 20
let rounded = Math.round(3.14); // Returns 3
String
The String object provides methods for manipulating strings:

omn
Copy code
let str = "Hello, world!";
let upper = str.toUpperCase();  // "HELLO, WORLD!"
let substring = str.substring(0, 5);  // "Hello"
Compilation and Running
To compile and run OmniScript code:

Write your OmniScript code in a .os file.
Compile the code using the OmniScript compiler.
Run the resulting executable on your target platform.
Example Compilation Command:
bash
Copy code
omniscript --compile myscript.os --output myscript.exe
Example Code
Hello World
omn
Copy code
console.log("Hello, OmniScript!");
Simple Function Example
omn
Copy code
function multiply(a, b) {
    return a * b;
}

let result = multiply(4, 5);
console.log(result);  // Output: 20