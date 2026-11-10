# Optional (Nullable) Types

A nullable type is a type that may hold a value or may be absent. You declare one by appending `?` to any type. The compiler requires a null check before the value is used, which removes a whole class of null-dereference bugs, especially when calling C libraries.

## Declaring nullable types

```os
let count: int? = 42;
let missing: int? = null;

let name: char*? = "OmniScript";
let nothing: char*? = nullptr;
```

| Kind of type | Null keyword | Examples |
|---|---|---|
| Non-pointer | `null` | `int?`, `f32?`, `Point?` |
| Pointer | `nullptr` | `char*?`, `void*?` |

`?` works on primitives, pointers, and structs.

## Non-nullable types

A type without `?` is intended to always hold a valid value:

```os
let a: int;                // error: must be initialized
let b: int = null;         // error: int is not nullable
let p: char* = nullptr;    // error: char* is not nullable
```

> **Status note:** other pages show uninitialized locals and `nullptr` defaults for non-nullable types (for example `let ptr: i32*;` in [Data Types](datatypes.md), and zero-initialized struct locals in the Vulkan examples). Confirm which behavior the current compiler enforces and make the two pages agree.

## Null checks and unwrapping

Compare against the matching null keyword. Inside the checked branch the value is unwrapped implicitly:

```os
let x: int? = 42;

if (x != null) {
    let y = x + 10;    // x is used as int here
}
```

Using a nullable value without a check is a compile error:

```os
let x: int? = null;
let y = x + 10;        // error: check x != null first
```

Early-return style works too:

```os
function show(x: int?) => i32 {
    if (x == null) {
        return 1;
    }
    printf("%d\n", x);
    return 0;
}
```

The ternary operator works as a compact check:

```os
let value = x != null ? x : 0;
```

## How it is implemented

A nullable type is a small struct with a flag and a value:

```os
struct int? {
    isValid: bool;
    value: int;
}
```

`T*?` is the same shape with `value: T*`. This is an implementation detail: use `null`, `nullptr`, and comparisons rather than touching `isValid` directly.

## FFI: pointer returns are nullable

Pointers returned from external functions are wrapped as `T*?`, so you must check them before use:

```os
extern "C" fn printf(fmt: char*, ...) => int;
extern "dependencies/SDL2/lib/SDL2.dll" {
    fn SDL_Init(flags: uint) => int;
    fn SDL_CreateWindow(title: char*, x: int, y: int, w: int, h: int, flags: uint) => void*?;
    fn SDL_DestroyWindow(window: void*) => void;
    fn SDL_Quit() => void;
}

function main() => i32 {
    if (SDL_Init(0x00000020) < 0) {
        return 1;
    }

    let window = SDL_CreateWindow("Demo", 100, 100, 800, 600, 0x00000004);
    if (window == nullptr) {
        printf("SDL_CreateWindow failed\n");
        SDL_Quit();
        return 1;
    }

    SDL_DestroyWindow(window);   // window is unwrapped after the check
    SDL_Quit();
    return 0;
}
```

`?` on a return type in an `extern` declaration is optional: the wrapping is applied automatically to pointer returns.

## More examples

### Safe string length

```os
extern "C" {
    fn printf(fmt: char*, ...) => int;
    fn strlen(s: char*) => u64;
}

function safeLength(s: char*?) => i32 {
    return s != nullptr ? strlen(s) as i32 : 0;
}

function main() => i32 {
    let a: char*? = "Hello, OmniScript!";
    let b: char*? = nullptr;
    printf("%d %d\n", safeLength(a), safeLength(b));   // 18 0
    return 0;
}
```

### Nullable structs

```os
struct Coordinate {
    x: int;
    y: int;
}

function describe(c: Coordinate?, label: char*?) => i32 {
    if (c == null) {
        printf("no coordinate\n");
        return 1;
    }
    if (label == nullptr) {
        printf("no label\n");
        return 1;
    }
    printf("%s at (%d, %d)\n", label, c.x, c.y);
    return 0;
}

function main() => i32 {
    let c: Coordinate? = Coordinate{ x: 42, y: 99 };
    describe(c, "Andromeda");        // Andromeda at (42, 99)
    describe(null, "Andromeda");     // no coordinate
    describe(c, nullptr);            // no label
    return 0;
}
```

## Rules summary

1. `T?` means "T or nothing". `T` means "always T".
2. Use `null` for non-pointers and `nullptr` for pointers.
3. Check before use. After the check the value is unwrapped automatically.
4. FFI pointer returns are `T*?` automatically.
5. Nullable wrapping has a small size cost (the flag), so prefer plain types where absence is impossible.
