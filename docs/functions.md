# Functions

Functions in OS are first-class citizens with support for default parameters, overloading, generics, and flexible calling conventions.

## Function Declaration Syntax

The basic syntax for function declarations uses the `=>` arrow syntax:

```os
function function_name(parameters) => return_type {
    // function body
}
```

## Basic Functions

### Simple Function
```os
function greet() => void {
    printf("Hello, OS!\n");
}
```

### Function with Parameters
```os
function add(a: i32, b: i32) => i32 {
    return a + b;
}
```

### Function with Return Value
```os
function multiply(x: f32, y: f32) => f32 {
    return x * y;
}
```

## Default Parameters

Functions can have default parameter values:

```os
function add(a: f32 = 1.0, b: f32 = 1.0) => f32 {
    return a + b;
}

// Can be called in multiple ways:
let result1 = add();           // Uses defaults: add(1.0, 1.0)
let result2 = add(5.0);        // Uses: add(5.0, 1.0)
let result3 = add(3.0, 4.0);   // Uses: add(3.0, 4.0)
```

## Named Parameters

OS supports named parameter calling:

```os
function calculate(a: i32 = 1, b: i32 = 1, c: i32 = 0) => i32 {
    return a + b + c;
}

// Named parameter calls
let result = calculate(b = 5, a = 3, c = 2);
```

## Function Overloading

Functions can be overloaded with different parameter counts or types:

```os
// Two-parameter version
function add(a: i32 = 1, b: i32 = 1) => i32 {
    return a + b;
}

// Three-parameter version
function add(a: i32 = 1, b: i32 = 1, c: i32 = 0) => i32 {
    return a + b + c;
}

// Different type version
function add(a: f32 = 1.0, b: f32 = 1.0) => f32 {
    return a + b;
}
```

## Generic Functions

### Simple Generic Function
```os
function identity<T>(value: T) => T {
    return value;
}

let int_result = identity<i32>(42);
let float_result = identity<f32>(3.14);
```

### Constrained Generic Function
```os
<Number extends i8 | i16 | i32 | i64 | f32 | f64 | f128>
function add<Number>(a: Number, b: Number) => Number {
    return a + b;
}

let result = add<i32>(5, 6);
```

### Generic Function with Type Inference
```os
function join<T>(a: T, b: T) => T {
    return a + b;
}

// Type can be inferred from usage
let sum = join(5, 10);  // T inferred as i32
```

## Lambda Functions

Anonymous functions can be created using lambda syntax:

```os
let add_lambda = (a: i32, b: i32) => i32 {
    return a + b;
}

// Generic lambda
let generic_add = <T>(a: T, b: T) => T {
    return a + b;
}
```

## Function Pointers

Functions can be stored in variables as function pointers (represented as `void*`):

```os
function multiply(a: i32, b: i32) => i32 {
    return a * b;
}

let fn_ptr: void* = multiply;
let fn_ref: function* = multiply;  // Alternative syntax
```

## Variadic Functions

Functions with variable argument lists can be declared using `...`:

```os
extern "C" {
    fn printf(fmt: char*, ...) => int;
}
```

## Function Calls

### Basic Function Calls
```os
let result = add(10, 20);
```

### Named Parameter Calls
```os
let result = add(b = 20, a = 10);
```

### Generic Function Calls
```os
let result = add<i32>(10, 20);
```

## Main Function Variants

The language supports multiple main function signatures:

### Simple Main
```os
function main() => i32 {
    return 0;
}
```

### Main with Argument Count
```os
function main(argc: int) => i32 {
    return argc;
}
```

### Main with Command Line Arguments
```os
function main(argc: int, argv: char**) => i32 {
    return 0;
}
```

## Function Examples

### Mathematical Functions
```os
function factorial(n: i32) => i32 {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

function power(base: f32, exponent: i32) => f32 {
    let result: f32 = 1.0;
    for (let i = 0; i < exponent; i++) {
        result *= base;
    }
    return result;
}
```

### Generic Utility Functions
```os
function max<T>(a: T, b: T) => T {
    return (a > b) ? a : b;
}

function swap<T>(a: &T, b: &T) => void {
    let temp = a;
    a = b;
    b = temp;
}
```

### Function with Multiple Return Values (Future Feature)
```os
// Hypothetical syntax for multiple returns
function divide_with_remainder(dividend: i32, divisor: i32) => (i32, i32) {
    return (dividend / divisor, dividend % divisor);
}
```

## Function Attributes

### Inline Functions
```os
inline function fast_add(a: i32, b: i32) => i32 {
    return a + b;
}
```

### Pure Functions
```os
pure function calculate(x: i32) => i32 {
    return x * x + 2 * x + 1;
}
```

## Best Practices

1. **Use meaningful function names** that describe what the function does
2. **Prefer const parameters** when the function doesn't modify them
3. **Use generics** for functions that work with multiple types
4. **Provide default values** for commonly used parameters
5. **Keep functions focused** on a single responsibility
6. **Document complex functions** with comments

## Function Calling Conventions

The language automatically handles calling conventions, but explicit control may be available:

```os
extern "C" function c_style_function(x: i32) => i32;
extern "stdcall" function windows_api_function(x: i32) => i32;
```

---

*Function features are actively being developed and may change.*