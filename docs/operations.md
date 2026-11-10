# Operators and Expressions

This page describes the operators available in OmniScript, their typing rules, and how they behave. Where behavior is not yet confirmed for compiled (LLVM) output, it is marked.

## Type rules

Operands of a binary operator must have the same type. To combine different types, cast explicitly with `as`:

```os
let i: i32 = 7;
let f: f32 = 2.5;
let r = i as f32 + f;      // ok
// let bad = i + f;        // error: mismatched types
```

Safe widening is allowed without a cast, for example `i16` to `i32`, because no information is lost. Narrowing (`i64` to `i32`, `f64` to `f32`) always needs `as`.

Floating-point literals default to `f64`. In some contexts (such as assigning a literal to an `f32` after an expression) you must cast: `x = 1.0 as f32;`.

## Arithmetic

| Operator | Meaning | Operand types |
|---|---|---|
| `+` | addition | integers, floats |
| `-` | subtraction | integers, floats |
| `*` | multiplication | integers, floats |
| `/` | division | integers, floats |
| `%` | remainder | integers |
| `-x` | negation | integers, floats |

Integer division truncates (`10 / 3` is `3`). Dividing by zero is an error.

## Compound assignment

`+=  -=  *=  /=  %=  &=  |=  ^=  <<=  >>=`

```os
let v = 10;
v += 5;    // 15
v *= 2;    // 30
```

## Increment and decrement

`i++` and `i--` on integers. Commonly used in `for` loops.

## Comparison

`==  !=  <  >  <=  >=` produce `bool`. Both sides must have the same type.

Floating-point `==` compares exactly. Prefer a tolerance check for computed values.

## Logical

| Operator | Meaning |
|---|---|
| `&&` | logical AND, short-circuits |
| `\|\|` | logical OR, short-circuits |
| `!` | logical NOT |
| `^` on `bool` | logical XOR |

## Bitwise

Operands are integers of the same type.

| Operator | Meaning |
|---|---|
| `&` | AND |
| `\|` | OR |
| `^` | XOR |
| `~` | NOT |
| `<<` | shift left |
| `>>` | shift right |

```os
let flags = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT;
if ((props.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) { ... }
```

Parenthesize bitwise tests inside comparisons, as above.

## Ternary

```os
let m = (a > b) ? a : b;
```

Both branches must have the same type.

## Casting

```os
let n = 3.9 as i32;                 // 3
let p = &value as void*;
let q = p as char*;
let chained = &value as void* as char*;
```

`as` converts between numeric types, between pointer types, and to and from `void*`.

## Pointers and references

| Syntax | Meaning |
|---|---|
| `&x` | address of `x` |
| `p[i]` | element `i` of the buffer `p` points to |
| `p[i].field` | field of element `i` |
| `&p[i]` | address of element `i` |
| `x as T*` | pointer cast |

References use `&T` in type positions. Pointer arithmetic is done through indexing rather than `p + n`.

## Nullable operands

A nullable value can only be used in an operator after a `null`/`nullptr` check. See [Optionals](optionals.md).

```os
if (x != null) { let y = x + 1; }
```

## Operator precedence

Precedence is expected to follow C: unary, then `* / %`, `+ -`, shifts, relational, equality, `&`, `^`, `|`, `&&`, `||`, ternary, assignment.

> **Verify:** confirm against the parser, and replace this note with the exact table. Use parentheses when in doubt.

## String and array operators (interpreter behavior)

Earlier versions of the interpreter supported operators on strings and arrays. These are **not guaranteed** in compiled output, and compiled strings are `char*`. Check before relying on them:

| Expression | Interpreter behavior |
|---|---|
| `string + string` | concatenation |
| `string + number` | number converted and appended |
| `string - string` | remove occurrences of the second string |
| `string * int` | repeat |
| `array + array` | concatenate |
| `array + value` / `value + array` | append / prepend |
| `array - value` | remove all occurrences |
| `array == array` | element-wise equality |

For compiled code, use C functions through the FFI (`strlen`, `strcat`, `memcpy`) or template strings (see [Strings](strings.md)).

## Errors

- Mismatched operand types are compile errors.
- Using a nullable without a check is a compile error.
- Division or modulo by zero is an error.
- Unsupported operator/type combinations are errors.
