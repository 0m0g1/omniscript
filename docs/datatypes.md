# Data Types in OS

OS provides a comprehensive type system designed for systems programming with precise control over memory layout and performance characteristics.

## Integer Types

### Basic Integer Types

```os
let platform_int : int = 1;        // Platform-specific integer
let byte_val : i8 = 127;           // 8-bit signed integer (-128 to 127)
let short_val : i16 = 32767;       // 16-bit signed integer
let int_val : i32 = 2147483647;    // 32-bit signed integer
let long_val : i64 = 9223372036854775807; // 64-bit signed integer
```

### Extended Integer Types

```os
// Future support for larger integers
// let huge_val : i128 = 340282366920938463463374607431768211455;
// let massive_val : i256 = very_large_number;
```

### Unsigned Integer Types

```os
let ubyte : u8 = 255;              // 8-bit unsigned integer
let ushort : u16 = 65535;          // 16-bit unsigned integer  
let uint : u32 = 4294967295;       // 32-bit unsigned integer
let ulong : u64 = 18446744073709551615; // 64-bit unsigned integer
```

## Floating-Point Types

OS supports a wide range of floating-point precisions for different use cases.

### Half Precision (16-bit)
```os
let half_val : f16 = 3.14;
let half_alt : half = 2.718;       // Alternative name
```

### Single Precision (32-bit)
```os
let single : f32 = 3.14159;
let single_alt : float = 2.718;    // Alternative name
```

### Double Precision (64-bit)
```os
let double_val : f64 = 3.14159265359;
let double_alt : double = 2.718281828; // Alternative name
```

### Quad Precision (128-bit)
```os
let quad : f128 = 1.6180339887498948482;
let quad_alt : fp128 = 2.718281828459045235; // Alternative name
let quad_alt2 : long_double = 3.141592653589793238; // Alternative name
```

### Extended Precision (x86 80-bit)
```os
let extended : x86_fp80 = 2.718281828459045235;
let extended_alt : x86_80bit = 3.141592653589793238;
let extended_alt2 : x87_FP80 = 1.618033988749895;
let extended_alt3 : Intel_FP80 = 2.718281828459045;
```

### PowerPC 128-bit Precision
```os
let ppc_quad : ppc_fp128 = 3.141592653589793238;
let ppc_quad_alt : PPC_Quad = 2.718281828459045235;
```

## Character and Boolean Types

### Character Types
```os
let ascii_char : char = 'A';
let unicode_char : char = '🚀';     // Unicode support
```

### Boolean Type
```os
let flag : bool = true;
let disabled : bool = false;
```

## String Types

### Basic String Types
```os
let c_string : char* = "Hello, OS!";
let utf8_string : utf8 = "Hello, 世界!";
let utf16_string : utf16 = "Hello, мир!";
let utf32_string : utf32 = "Hello, 🌍!";
```

## Array Types

### Fixed-Size Arrays
```os
// Character array
let char_array : [5]char = ['H', 'e', 'l', 'l', 'o'];

// Integer array
let numbers : [5]i32 = [1, 2, 3, 4, 5];

// Multi-dimensional arrays
let matrix : [3][3]i32 = [
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
];
```

### Dynamic Arrays
```os
let dynamic_ints : [i32] = [1, 2, 3, 4, 5];
let dynamic_chars : [char] = "Dynamic String";
```

## Pointer Types

### Basic Pointers
```os
let value : i32 = 42;
let ptr : i32* = &value;           // Pointer to i32
let char_ptr : char* = "Hello";    // Pointer to char
```

### Multi-level Pointers
```os
let value : i32 = 42;
let ptr : i32* = &value;
let ptr_to_ptr : i32** = &ptr;
let ptr_to_ptr_to_ptr : i32*** = &ptr_to_ptr;
```

### Void Pointers
```os
let void_ptr : void* = nullptr;
let generic_ptr : void* = &some_value;
```

## Reference Types

### Basic References
```os
let value : i32 = 42;
let ref : &i32 = value;            // Reference to i32
let flag : bool = true;
let bool_ref : &bool = flag;       // Reference to bool
```

### Multi-level References
```os
let value : i32 = 42;
let ref : &i32 = value;
let ref_to_ref : &&i32 = &ref;
```

## Nullable Types

### Optional Values
```os
let nullable_int : ?i32 = null;
let optional_value : ?i32 = 42;

// Check for null
if (nullable_int == null) {
    // Handle null case
}
```

## Type Casting

OS provides flexible type casting using the `as` keyword:

### Basic Casting
```os
let float_val : f32 = 64.5;
let int_val : i32 = float_val as i32;  // Converts to 64
```

### Pointer Casting
```os
let some_value : i32 = 42;
let void_ptr : void* = &some_value as void*;
let char_ptr : char* = void_ptr as char*;
let int_ptr : i32* = char_ptr as i32*;
```

### Function Pointer Casting
```os
function my_function(x: i32) => i32 {
    return x * 2;
}

// Function pointers are represented as void*
let fn_ptr : void* = my_function as void*;

// Can cast back when calling
let result = (fn_ptr as function(i32) => i32)(10);
```

### Chained Casting
```os
let original : i32 = 42;
let final_ptr : char* = &original as void* as char*;
```

## Function Pointer Types

Currently, function pointers are handled as `void*` with casting:

```os
function add(a: i32, b: i32) => i32 {
    return a + b;
}

function multiply(a: i32, b: i32) => i32 {
    return a * b;
}

// Store function as void pointer
let operation : void* = add as void*;

// Cast back to function type when calling
let result = (operation as function(i32, i32) => i32)(5, 3);

// Change the operation
operation = multiply as void*;
let result2 = (operation as function(i32, i32) => i32)(5, 3);
```

## Struct Types

### Basic Struct
```os
struct Point {
    x: f32 = 0.0;
    y: f32 = 0.0;
}

let origin = Point{};
let point = Point{ x: 10.0, y: 20.0 };
```

### Struct with Methods
```os
struct Vector3 {
    x: f32 = 0.0;
    y: f32 = 0.0;
    z: f32 = 0.0;
    
    length() => f32 {
        return sqrt(this.x * this.x + this.y * this.y + this.z * this.z);
    }
}
```

## Enumeration Types

### Basic Enums
```os
enum Color {
    Red,
    Green,
    Blue
}

let primary_color = Color.Red;
```

### Lookup Enums
```os
enum Fruit(lookup) {
    Apple,
    Banana,
    Orange
}

let fruit = Fruit.Apple;
```

### Enum Classes
```os
enum class Status {
    Pending,
    Processing,
    Complete,
    Failed
}

let current_status = Status.Pending;
```

## Type Inference

OS can infer types in many contexts:

```os
let number = 42;           // Inferred as int
let pi = 3.14159;          // Inferred as f64
let message = "Hello";     // Inferred as char*
let flag = true;           // Inferred as bool
let array = [1, 2, 3];     // Inferred as [i32]
```

## Type Aliases

You can create type aliases for complex types:

```os
type IntPtr = i32*;
type Matrix3x3 = [3][3]f32;
type Callback = void*;  // Function pointer alias

let ptr : IntPtr = &some_int;
let transform : Matrix3x3 = [[1,0,0],[0,1,0],[0,0,1]];
let handler : Callback = my_function as void*;
```

## Constants and Compile-Time Values

### Compile-Time Constants
```os
const MAX_BUFFER_SIZE : i32 = 1024;
const PI : f64 = 3.14159265359;
const GREETING : char* = "Hello, World!";
```

### Constant Expressions
```os
const BUFFER_SIZE : i32 = 1024;
const DOUBLE_BUFFER : i32 = BUFFER_SIZE * 2;  // Computed at compile time
```

## Memory Layout Considerations

### Struct Packing
```os
struct PackedData {
    flag: bool;      // 1 byte
    value: i32;      // 4 bytes
    // Compiler may add padding for alignment
}
```

### Array Memory Layout
```os
let array : [5]i32 = [1, 2, 3, 4, 5];
// Elements are stored contiguously in memory
// array[0] is at base address
// array[1] is at base address + sizeof(i32)
// etc.
```

## Type Checking

OS performs strict type checking:

```os
let int_val : i32 = 42;
let float_val : f32 = 3.14;

// This would be an error without explicit casting:
// let result = int_val + float_val;  // Error!

// Correct way:
let result = int_val as f32 + float_val;  // OK
```

## Default Values

Many types have default values:

```os
let int_default : i32;        // Defaults to 0
let float_default : f32;      // Defaults to 0.0
let bool_default : bool;      // Defaults to false
let ptr_default : i32*;       // Defaults to nullptr
```

## Size and Alignment

You can query type properties:

```os
// These would be compile-time constants
const INT_SIZE = sizeof(i32);      // 4
const PTR_SIZE = sizeof(void*);    // Platform dependent
const ALIGN = alignof(i64);        // 8 on most platforms
```