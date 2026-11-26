# OmniScript syntax guide (v0, reconstructed from real code)

Every rule here comes from code that ran in OmniBrowser / the pharmacy server / the Ui library, or from a test you ran.
Tags: **[ok]** seen working, **[test]** confirmed by a probe, **[verify]** I have not seen it work (probe file `syntax_probe/` tests it).
Rewrite whichever line the compiler disagrees with; this file should become the language's first real documentation.

---

## 1. Program layout

```
module Console = import "standard/1/Console.os";   // import with an alias [ok]
include "standard/1/Ui/Api.os";                    // textual include, order matters [ok]

let LIMIT: int = 100;                              // module-level variable [ok]

function main() => int {                           // entry point returns int [ok]
    Console.println("hello");
    return 0;
}
```

- Scripts run from the project root; relative paths (`"browser/util.os"`) resolve from there. **[ok]**
- Comments: `// line` and `/* block */` **[test]**.
- Statements end with `;`. Blocks use `{ }`.
- `import` gives a namespace: `U.slen(x)`, `Net.load(url)`. Use `module X = import` for functions, `include` to paste declarations.

## 2. Types

| Type | Notes |
|---|---|
| `int` | 32-bit signed **[ok]** |
| `i64`, `u64`, `u32`, `u8` | fixed width **[ok]** |
| `float`, `double` | `-1.5`, `0.0` literals **[test]** |
| `bool` | `true`, `false` **[ok]** |
| `char*` | C string, NUL-terminated; `s[i] as int` reads a byte **[ok]** |
| `T*` | pointer; `nullptr` is null **[ok]** |
| `void*` | untyped pointer **[ok]** |
| `[N]T` | fixed array field/local: `hist: [32]char*;` **[ok]** |
| `fn(A, B) => R` | function pointer type **[ok]** |
| `L.List<T>` | generic from the standard library **[ok]**; own generic struct `struct Box<T> { v: T; }` + `Box<int>` **[test]**; generic functions `pick<int>(..)` **[verify]** (p11 not in the results) |

Casts: `x as T` — `(n as float)`, `ptr as void*`, `p as Event*`. **[ok]**
Sizes: `sizeof(T)`. **[ok]**
Literals: decimal, hex `0xFF` **[ok]**, strings `"a\nb"` with `\"` escapes **[ok]**, char via `s[i] as int` compared to a code (`== 38` for `&`).

## 3. Variables

```
let x: int = 5;          // typed [ok]
let y = 5;               // inferred [test]
x = x + 1;               // assignment; reassigning must keep the type [ok]
```

- Type mismatch on reassignment is an error (`Reassignment type mismatch`). **[ok]**
- A local struct is declared without a value: `let o: Str.String;` (defaults run for stack structs **[test]**).
- No negative-literal problem: `let a: int = -1;` works **[test]**; `-a` works **[test]**.

## 4. Operators

`+ - * / %` **[test]**, comparisons `== != < <= > >=`, logic `&& || !`, bit ops `& | ^ << >>` **[test]**, ternary `c ? a : b` **[test]**.
Compound assignment `+= -= *=` and `++ --` **[test]**.
Pointers: `&x` address-of **[ok]**, `&arr[i]` **[ok]**, `p.field` works on pointers (no `->`) **[ok]**.

## 5. Control flow

```
if (cond) stmt; else if (cond2) { ... } else { ... }     // [ok]
while (cond) { ... }                                      // [ok]
return value;                                             // allowed inside loops [test]
break; continue;                                          // [test]
for (let i: int = 0; i < n; i = i + 1) { }                // [test]
do { ... } while (cond);                                  // [test]
switch (x) { case 1: ...; break; default: ...; }          // [test]
```

`match`: **[verify]**.

## 6. Functions

Anonymous functions (lambdas): `let f = function(x: int) => int { return x + 1; }` is the form you gave me; my arrow form `(x: int) => x + 1` also passed (p16). Check which one the compiler documents. **[test]**
String interpolation: backticks with `${expr}`: `` `n = ${n}` `` **[from your example]**. Plain `"n = {n}"` does NOT interpolate (p17 printed `{n}` literally). **[test]**
`try { } catch (e) { }`: p14 printed nothing at all, neither the try body nor the catch, so it is not implemented (or silently dropped). **[test: fails]**

```
function add(a: int, b: int) => int { return a + b; }
function log(msg: char*) => void { ... }
```

- Return type after `=>`. `void` for none. **[ok]**
- Pass a function by name: `call(hello)` **[test]**. Module-qualified (`PA.loginPage`) failed as an HTTP handler **[ok: error seen]**; use a local wrapper until fixed.
- Function pointer call: `f(args)` where `f: fn(A) => R` **[ok]**.

## 7. Structs and methods

```
struct Pt {
    x: int = 7;                 // default (runs on stack locals [test], NOT on malloc'd memory [test])
    y: int = 9;
    len() => int { return this.x + this.y; }       // method, no `function`, uses `this` [ok]
}

let p: Pt;                      // stack
p.x = 3;
let q: Pt* = CMem.malloc(sizeof(Pt)) as Pt*;   // heap: set every field yourself
```

- Enums `enum Color { Red, Green, Blue }` used as `Color.Green` **[test]**; `const MAXN: int = 5;` **[test]**.
- Methods are declared inside the struct body: `name(args) => ret { ... }`. **[ok]**
- Call with `.`: `p.len()`. Also on pointers.
- No inheritance observed. Interfaces/traits **[verify]**.

## 8. Strings (standard library)

```
module Str = import "standard/1/String.os";
let s: Str.String;
s.append("abc"); s.appendChar(c); s.appendInt(n); s.appendString(&other);
s.cstr(); s.len; s.clear(); s.free();
```

`U` (`browser/util.os`) helpers: `slen`, `sub(s, a, b)`, `dup`, `cat`, `streq`, `startsAt(s, at, prefix)`, `indexOf(s, from, needle)`,
`lastIndexOfCh`, `lower`, `trim`, `isSpace`, `isDigit`, `fromInt`, `toNum`. These allocate; today you must remember who owns the result.

## 9. Memory (current, manual)

```
module CMem = import "standard/1/c/mem.os";
let p = CMem.malloc(sizeof(T)) as T*;
CMem.memset(p as void*, 0, sizeof(T));
CMem.free(p as void*);
```
See `OMNISCRIPT_HIGHLEVEL_PLAN.md` for the planned scope-based ownership.

## 10. Foreign functions (C DLLs)

```
extern "path/to/lib.dll" {
    fn sqlite3_open(path: char*, db: void**) => int;
}
```
Paths are relative to the run folder. Out-parameters are pointers to pointers (`void**`). **[ok]**

## 11. Standard modules seen

`Console` (`println`, `log`, `error`), `String`, `Text` (`urlEncode`), `Json` (`parseStrict`, `stringify`), `Http`, `HttpServer`,
`Sqlite`, `Crypto`, `Thread` (`Mutex`, `Atomic`), `Channel` (`ThreadPool`), `Canvas` (Pen2D), `Image`, `Fs`, `Time`, `List<T>`, `Ui`.

## 12. Known quirks (confirmed)

1. Struct field defaults do not run for malloc'd memory. Zero it with `CMem.memset` and assign fields.
2. Passing a module-qualified function as an HTTP handler fails; wrap it in a local function.
3. `lastFinal: char* = nullptr` at module level gave a type mismatch on reassignment in one place (q5 shows plain `nullptr` works), so the cause was something else, still unexplained.

*(Disproved, do not repeat: "no ternaries", "no negative literals", "no `return` in loops".)*

---

## 13. Next steps for this guide

1. Re-run p11 (generic function; missing from your output), p14 (try/catch printed nothing) and p18-style probes; update the tags.
2. Turn the confirmed list into `docs/language_facts.md` (Phase 0 of the high-level plan).
3. Add: generics declaration syntax, enums/constants, error handling, threads, and the module/visibility rules once confirmed.
