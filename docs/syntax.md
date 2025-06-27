# OS Language Syntax

This document covers the fundamental syntax rules and conventions of the OS programming language.

## Comments

OS supports several comment styles:

### Single-line Comments
```os
// This is a single-line comment
let value = 42; // Comment at end of line
```

### Multi-line Comments
```os
/*
 * This is a multi-line comment
 * spanning multiple lines
 */
```

### Nested Comments
OS supports nested comments, which is useful for commenting out blocks of code:

```os
/*
 * Outer comment
 * /* Inner comment */
 * Still in outer comment
 */
```

## Variable Declarations

### Basic Syntax
```os
let variable_name : type = value;
```

### Type Inference
When the type can be inferred from the value, you can omit the type annotation:

```os
let number = 42;        // Inferred as int
let pi = 3.14;          // Inferred as f64
let message = "Hello";  // Inferred as char*
```

### Explicit Typing
```os
let age : i32 = 25;
let temperature : f32 = 98.6;
let is_valid : bool = true;
```

## Constants

Constants are immutable values declared with `const`:

```os
const PI : f64 = 3.14159265359;
const MAX_SIZE : i32 = 1000;
const GREETING : char* = "Hello, World!";
```

## Function Declarations

### Basic Function Syntax
```os
function function_name(parameter1: type1, parameter2: type2) => return_type {
    // function body
    return value;
}
```

### Examples
```os
// Function with no parameters
function get_answer() => i32 {
    return 42;
}

// Function with parameters
function add(a: i32, b: i32) => i32 {
    return a + b;
}

// Function with no return value
function print_message(msg: char*) => void {
    printf("%s\n", msg);
}
```

## Operators

### Arithmetic Operators
```os
let a = 10;
let b = 3;

let sum = a + b;        // Addition: 13
let diff = a - b;       // Subtraction: 7
let product = a * b;    // Multiplication: 30
let quotient = a / b;   // Division: 3
let remainder = a % b;  // Modulo: 1
```

### Compound Assignment
```os
let value = 10;
value += 5;  // value = 15
value -= 3;  // value = 12
value *= 2;  // value = 24
value /= 4;  // value = 6
value %= 4;  // value = 2
```

### Comparison Operators
```os
let x = 10;
let y = 20;

let equal = (x == y);           // false
let not_equal = (x != y);       // true
let less_than = (x < y);        // true
let less_equal = (x <= y);      // true
let greater_than = (x > y);     // false
let greater_equal = (x >= y);   // false
```

### Logical Operators
```os
let a = true;
let b = false;

let and_result = a && b;    // false
let or_result = a || b;     // true
let not_result = !a;        // false
```

### Bitwise Operators
```os
let x = 0b1010;  // 10 in binary
let y = 0b1100;  // 12 in binary

let and_bits = x & y;       // 0b1000 (8)
let or_bits = x | y;        // 0b1110 (14)
let xor_bits = x ^ y;       // 0b0110 (6)
let not_bits = ~x;          // Bitwise NOT
let left_shift = x << 1;    // 0b10100 (20)
let right_shift = x >> 1;   // 0b0101 (5)
```

## Control Structures

### If Statements
```os
if (condition) {
    // code block
}

if (condition) {
    // code block
} else {
    // alternative code block
}

if (condition1) {
    // code block 1
} else if (condition2) {
    // code block 2
} else {
    // default code block
}
```

### Ternary Operator
```os
let result = condition ? value_if_true : value_if_false;
let max = (a > b) ? a : b;
```

## Loops

### For Loops
```os
// Traditional for loop
for (let i = 0; i < 10; i++) {
    // loop body
}

// For loop with different step
for (let i = 0; i < 100; i += 5) {
    // loop body
}
```

### While Loops
```os
let count = 0;
while (count < 5) {
    // loop body
    count++;
}
```

### Do-While Loops
```os
let i = 0;
do {
    // loop body
    i++;
} while (i < 5);
```

## Blocks and Scope

Code blocks are defined with curly braces `{}`:

```os
{
    let local_var = 10;
    // local_var is only accessible within this block
}
// local_var is not accessible here
```

Variables declared in a block are only accessible within that block and its nested blocks:

```os
function example() => void {
    let outer = 1;
    
    if (true) {
        let inner = 2;
        // Both outer and inner are accessible here
        outer = inner + 1;
    }
    
    // Only outer is accessible here
    // inner is out of scope
}
```

## Identifiers

### Naming Rules
- Must start with a letter (a-z, A-Z) or underscore (_)
- Can contain letters, digits (0-9), and underscores
- Case-sensitive
- Cannot be a reserved keyword

### Valid Identifiers
```os
let variable = 1;
let _private = 2;
let myVariable = 3;
let var123 = 4;
let CONSTANT_VALUE = 5;
```

### Invalid Identifiers
```os
// let 123invalid = 1;  // Cannot start with digit
// let my-variable = 2; // Cannot contain hyphens
// let function = 3;    // Cannot use reserved keywords
```

## Reserved Keywords

The following are reserved keywords in OS:

```
let         const       function    return      if          else
for         while       do          break       continue    switch
case        default     struct      class       enum        public
private     protected   static      extern      new         delete
this        true        false       null        nullptr     void
int         i8          i16         i32         i64         i128
f16         f32         f64         f128        char        bool
as          heap        stack
```

## String Literals

### Basic Strings
```os
let message = "Hello, World!";
let empty = "";
```

### String Concatenation
```os
// String concatenation is typically done through functions
// The exact syntax may vary based on implementation
```

### Escape Sequences
```os
let newline = "First line\nSecond line";
let tab = "Column1\tColumn2";
let quote = "He said \"Hello\"";
let backslash = "Path\\to\\file";
```

## Numeric Literals

### Integer Literals
```os
let decimal = 42;
let hex = 0xFF;         // Hexadecimal
let binary = 0b1010;    // Binary
let octal = 0o755;      // Octal
```

### Floating-Point Literals
```os
let simple = 3.14;
let scientific = 1.23e-4;
let large = 1.0e6;
```

## Character Literals
```os
let letter = 'A';
let digit = '7';
let newline = '\n';
let tab = '\t';
```

## Boolean Literals
```os
let is_true = true;
let is_false = false;
```

## Null and Pointer Literals
```os
let null_ptr = nullptr;
let void_ptr : void* = nullptr;
```

## Semicolons

Semicolons are required to terminate statements:

```os
let value = 42;
function_call();
return result;
```

## Line Continuation

Long statements can be broken across multiple lines:

```os
let long_calculation = first_value + 
                      second_value * 
                      third_value;

function_with_many_params(
    first_param,
    second_param,
    third_param
);
```

## Code Style Conventions

While not enforced by the compiler, these conventions are recommended:

### Variable Names
- Use camelCase for variables: `myVariable`
- Use snake_case for functions: `my_function`
- Use SCREAMING_SNAKE_CASE for constants: `MAX_SIZE`

### Indentation
- Use 4 spaces for indentation
- Be consistent throughout your codebase

### Braces
- Opening brace on the same line as the statement
- Closing brace on its own line

```os
function example() => void {
    if (condition) {
        // code here
    }
}
```