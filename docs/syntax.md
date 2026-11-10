# Syntax

The fundamental syntax rules of OmniScript.

## Comments

```os
// single line

/* multi-line */

/*
 * nested comments are supported
 * /* inner */
 * still commented
 */
```

## Statements

Every statement ends with a semicolon. Blocks use braces and introduce a scope.

```os
let value = 42;
doThing();
{
    let local = 1;   // visible only in this block
}
```

Long expressions can span lines.

## Variables and constants

```os
let age: i32 = 25;
let pi = 3.14159;                    // type inferred (f64)
const MAX_SIZE: i32 = 1000;          // immutable, compile-time
let maybe: i32? = null;              // nullable
```

## Functions

Definitions use `function` and `=>` for the return type:

```os
function add(a: i32, b: i32) => i32 {
    return a + b;
}

function log(msg: char*) => void {
    printf("%s\n", msg);
}
```

Defaults, named arguments, overloading, generics, and lambdas are covered in [Functions](functions.md).

`fn` is used only for **external declarations** and function types:

```os
extern "C" fn printf(fmt: char*, ...) => int;
```

## Externs and includes

```os
extern "C" fn malloc(size: u64) => void*;

extern "path/lib.dll", "path/lib.a" {
    fn libInit() => int;
}

include "std/vulkan.os";
```

See [External Bindings](externalbindings.md) and [FFI](ffi.md).

## Structs, classes, enums

```os
struct Point { x: f32 = 0.0; y: f32 = 0.0; }
class Counter { private n: i32 = 0; public next() => i32 { this.n++; return this.n; } }
enum Color { Red, Green, Blue }
```

Struct methods omit `function` and use `name(params) => type { ... }`.

## Control flow

```os
if (a > b) { ... } else if (a == b) { ... } else { ... }
let m = (a > b) ? a : b;

for (let i = 0; i < 10; i++) { ... }
while (cond) { ... }
do { ... } while (cond);
```

`break` and `continue` are supported. Details in [Control Flow](controlflow.md).

## Operators

Arithmetic, comparison, logical, bitwise, and `as` casts are described in [Operations](operations.md).

## Literals

```os
42   0xFF   0b1010   0o755          // integers
3.14   1.23e-4   1.0e6              // floats
true   false                        // bool
'A'   '\n'   '\u2605'               // chars
"text"   `template ${x}`   r"raw"   // strings
nullptr   null                      // null values
```

Escapes in regular strings: `\n \t \r \\ \"`.

## Identifiers

- Start with a letter or `_`; contain letters, digits, `_`.
- Case-sensitive. Cannot be a keyword.
- `module` is reserved in the language, so the Vulkan importer renames fields called `module` to `module_`.

## Keywords

```
let        const      function   fn         return     if
else       for        while      do         break      continue
switch     case       default    struct     class      enum
type       public     private    protected  static     extern
include    import     new        delete     this       true
false      null       nullptr    void       as         heap
stack      inline     pure       constructor destructor
int  uint  i8  i16  i32  i64  i128  u8  u16  u32  u64
float double f16 f32 f64 f128  char  bool
```

`switch`/`case`, `static`, `protected`, and inheritance are listed in the docs as future features.

## Style conventions (not enforced)

- 4-space indentation, opening brace on the same line.
- `camelCase` for variables and functions (the repo examples use this), `PascalCase` for types, `SCREAMING_SNAKE_CASE` for constants.

## Known limitations

- A `return` inside a `while` body is typed as `void` by the compiler. Workaround: set a flag in the loop and return after it.
- Float literals default to `f64`; you may need `as f32` in expressions.
- Handles from C APIs are plain `void*`, so passing the wrong handle type is not caught by the compiler.
- Fixed-array syntax is documented as `[N]T` while examples use `T[N]`.
