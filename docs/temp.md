markdown
# OmniScript (os) Language Tutorial

## Introduction
OmniScript (os) is a systems programming language with modern features, combining low-level control with high-level abstractions. It supports interoperability with C, object-oriented programming, and a standard library.

## Basic Syntax

### Comments
```os
// Single-line comment
/* Multi-line
   comment */
Variables
os
let x: int = 5;          // Signed integer
let y: uint = 10;        // Unsigned integer
let z: f32 = 3.14;       // 32-bit float
let name: char* = "os";  // String (char pointer)
```

### Primitive Types

Integer types: `i8`, `i16`, `i32`, `i64`, `i128`, `i256`, `i512`, `i1024`

Unsigned integers: `u8`, `u16`, `u32`, `u64`, etc.

Floating-point: `f16`, `f32`, `f64`, `f80`, `f128`

Boolean: bool (`true`/`false`)

Pointer: `void*`

String: `char*`

### Control Flow
If-Else
```os
if (condition) {
    // code
} else if (another_condition) {
    // code
} else {
    // code
}
```

### Loops
os
// For loop
for (let i = 0; i < 10; i++) {
    std.IO.Console.log(i);
}

### While loop
while (condition) {
    // code
}

### Functions
```os
// Basic function
function add(a: int, b: int) => int {
    return a + b;
}
```
```os
// Function with default parameters
function greet(name: char* = "world") => void {
    std.IO.Console.printf("Hello, %s!", name);
}
```

```
// Variadic function
function sum(...nums: int) => int {
    let total = 0;
    for (let i = 0; i < nums_count; i++) {
        total += nums[i];
    }
    return total;
}
```

# Object-Oriented Programming
## Classes
```os
class String {
    private text: char*;
    private size: int = 0;

    // Constructor
    constructor(text: char* = "") => void {
        this.text = text;
        this.size = this.calculateLength();
    }

    // Method
    public length() => int {
        return this.size;
    }

    // Private method
    private calculateLength() => int {
        let len = 0;
        while (this.text[len] != '\0') {
            len++;
        }
        return len;
    }
}

// Usage
let str = String("hello");
let len = str.length();
```

## Standard Library
### Math Operations
```os
import "std";

let factorial = std.Math.factorial(5);
let power = std.Math.powf(2.0, 3.0);
let pi = std.Math.pi;
```

### I/O Operations
```os
std.IO.Console.printf("Value: %d, %.2f\n", 42, 3.14159);
std.IO.Console.Beep(440, 300);  // Play a tone at 440Hz for 300ms
```

## Interoperability
###C Interop
```os
// Standard C functions
extern "C" fn printf(...fmt: char*) => int;
extern "C" fn malloc(size: size_t) => void*;
extern "C" fn free(ptr: void*) => void;
```

### External Libraries
```os
extern "dependencies/glfw/glfw3.dll" {
    fn glfwInit() => int;
    fn glfwCreateWindow(width: int, height: int, title: char*, 
                        monitor: void*, share: void*) => void*;
    // ... other functions
}
```

### Memory Management
```os
// Manual memory management
let buffer: void* = malloc(1024);
// ... use buffer ...
free(buffer);

// Automatic management with classes
class ManagedBuffer {
    private ptr: void*;
    
    constructor(size: int) {
        this.ptr = malloc(size);
    }
    
    destructor() {
        free(this.ptr);
    }
}
```

### Advanced Features
Type Aliases
```os
type float = f32;
type byte = u8;
```

### Compiler Directives
```os
// TODO: Add compiler flag support
#compile_option "optimize=3"
```

Template Strings (Future)

```os
// Planned feature
let name = "world";
let greeting = `Hello ${name}!`;
```

# Example Programs
Window Creation (GLFW)
```os
extern "glfw3.dll" {
    fn glfwInit() => int;
    fn glfwCreateWindow(width: int, height: int, 
                        title: char*, monitor: void*, 
                        share: void*) => void*;
    // ... other GLFW functions
}

fn main() {
    if (glfwInit() == 0) {
        std.IO.Console.error("GLFW init failed");
        return;
    }
    
    let window = glfwCreateWindow(800, 600, "os Window", null, null);
    // ... window management ...
}
```

### Factorial Calculator
```os
import "std";

fn main() {
    let n: int = std.Math.factorial(5);
    
    if (n == -1) {
        std.IO.Console.error("Invalid input");
    } else {
        std.IO.Console.printf("5! = %i", n);
    }
}
```

Best Practices
Use explicit types for public APIs

Prefer standard library functions when available

Use classes for complex data structures

Always check return values from external functions

Use const for values that shouldn't change

Document public APIs with comments

Next Steps
Explore the full standard library documentation

Learn about build system integration

Study advanced memory management techniques

Experiment with FFI (Foreign Function Interface) text
