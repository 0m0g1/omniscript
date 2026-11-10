# Data Types

OmniScript has a static type system with precise control over size and layout. Types are checked strictly: mixing types requires an explicit `as` cast (see [Operations](operations.md)).

## Aliases

| Canonical | Aliases |
|---|---|
| `i32` | `int`, `int32` |
| `u32` | `uint` |
| `i8` `i16` `i64` | `int8` `int16` `int64` |
| `f32` | `float` |
| `f64` | `double` |
| `f16` | `half` |
| `f128` | `fp128`, `long_double` |

`size_t` does not exist. Use `u64` for sizes on 64-bit targets.

## Integers

```os
let a: i8  = 127;
let b: i16 = 32767;
let c: i32 = 2147483647;
let d: i64 = 9223372036854775807;

let e: u8  = 255;
let f: u16 = 65535;
let g: u32 = 4294967295;
let h: u64 = 18446744073709551615;
```

Literals: decimal `42`, hex `0xFF`, binary `0b1010`, octal `0o755`. Larger integers (`i128` and up) are reserved for future use.

## Floating point

```os
let half: f16 = 3.14;
let single: f32 = 3.14159;
let dbl: f64 = 3.14159265359;
let quad: f128 = 1.6180339887498948482;
```

Platform-specific formats: `x86_fp80` (aliases `x86_80bit`, `x87_FP80`, `Intel_FP80`) and `ppc_fp128` (`PPC_Quad`).

Floating literals default to `f64`. Assigning one to an `f32` in a bare declaration works; inside larger expressions you may need `1.0 as f32`.

## Bool and char

```os
let ok: bool = true;
let letter: char = 'A';
let star: char = '\u2605';
```

## Strings

| Literal | Type | See |
|---|---|---|
| `"text"` | `char*` | C-compatible, null-terminated |
| `` `text ${x}` `` | template string | [Strings](strings.md) |
| `r"text"` | raw string | [Strings](strings.md) |

Also available: `utf8`, `utf16`, `utf32` string types.

## Arrays

```os
let nums: [5]i32 = [1, 2, 3, 4, 5];
let grid: [3][3]i32 = [[1,2,3],[4,5,6],[7,8,9]];
let growable: [i32] = [1, 2, 3];
```

The suffix form is also used in working code:

```os
let dyn: uint[2] = [VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR];
let clear: float[4] = [0.0, 0.0, 0.0, 1.0];
```

> **Pick one:** `[N]T` is documented, `T[N]` is used by the Vulkan examples. Confirm both parse, then keep a single canonical form in the docs.

Fixed arrays are stored contiguously. Take the address of an element with `&arr[i]`.

## Pointers

```os
let v: i32 = 42;
let p: i32* = &v;
let pp: i32** = &p;
let raw: void* = &v as void*;
let none: void* = nullptr;
```

Cast pointers with `as`. Index with `p[i]`.

## References

```os
let v: i32 = 42;
let r: &i32 = v;
```

## Nullable types

`T?` may be `null` (or `nullptr` for pointers) and must be checked before use. See [Optionals](optionals.md).

## Structs

```os
struct Point {
    x: f32 = 0.0;
    y: f32 = 0.0;
}

let origin = Point{};
let p = Point{ x: 10.0, y: 20.0 };
```

Fields have optional defaults. Methods are declared inside the struct and use `this`. Uninitialized struct locals are zero-initialized, which the Vulkan examples rely on:

```os
let ci: VkInstanceCreateInfo;
ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
```

See [Classes and Structs](classesandstructs.md).

## Enums

```os
enum Color { Red, Green, Blue }
let c = Color.Red;

enum Fruit(lookup) { Apple, Banana, Orange }
enum class Status { Pending, Processing, Complete, Failed }
```

## Function pointers

Currently represented as `void*` and cast back to a function type to call:

```os
function add(a: i32, b: i32) => i32 { return a + b; }

let op: void* = add as void*;
let r = (op as function(i32, i32) => i32)(5, 3);
```

## Type inference

```os
let n = 42;          // i32
let pi = 3.14159;    // f64
let s = "hi";        // char*
let ok = true;       // bool
let a = [1, 2, 3];   // [i32]
```

## Type aliases and constants

```os
type IntPtr = i32*;
type Callback = void*;

const MAX_BUFFER_SIZE: i32 = 1024;
const DOUBLE_BUFFER: i32 = MAX_BUFFER_SIZE * 2;   // compile-time
```

## Default values

| Type | Default |
|---|---|
| integers | `0` |
| floats | `0.0` |
| `bool` | `false` |
| pointers | `nullptr` |
| structs | all fields zero or their declared defaults |

> See the status note in [Optionals](optionals.md) about non-nullable types and initialization.

## Size and alignment

```os
const INT_SIZE = sizeof(i32);       // 4
const PTR_SIZE = sizeof(void*);     // 8 on 64-bit
const ALIGN = alignof(i64);         // 8
```

Use `sizeof(T)` rather than hard-coded byte counts when allocating, for example `malloc((n * sizeof(VkQueueFamilyProperties)) as u64)`.

## Strict checking

```os
let a: i32 = 42;
let b: f32 = 3.14;
// let r = a + b;           // error
let r = a as f32 + b;       // ok
```
