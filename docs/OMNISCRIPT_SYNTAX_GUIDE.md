# OmniScript syntax guide (v4, compact)

## 0. Rules for the AI
- Write complete, runnable `.os` files. State assumptions in one line.
- Use only syntax and APIs in this guide. For anything else (a module's functions, a constant), do NOT guess: give the user the exact `--doc-show` command (§12b) and wait for the output.
- Prefer the safe forms marked in this guide; avoid anything tagged [?].
- If the user pastes a compiler error, fix from the error text (the engine's messages are descriptive: they name the problem, file and line).
- Unimplemented features give a compile error, never a silent skip. The author fixes the compiler, not the code: if valid-looking code fails, don't contort it; give a minimal repro and say it looks like a compiler gap. Workarounds in this guide are temporary.
- Tell the user how to run what you wrote (§12b), including `--watch` for servers.

Tags: [ok] seen working · [test] probe-confirmed · [demo] ownership_demo.os · [author] stated by language author · [parser] from parser source, not run · [?] unverified (don't rely on it; prefer the safe form shown).

## 1. Layout
```
include "standard/1/Ui/Api.os";                    // textual paste, ';' optional, no dedup, order matters
import "standard/1/Console.os";                    // namespaced under inferred name, ';' required
module Console = import "standard/1/Console.os";   // alias, names stay under Console. [ok]
let LIMIT: int = 100;
function main() => int { return 0; }               // optional; runs after top-level statements. int|void return; params none or (argc:int, argv:char**)
```
- File-level statements run in order. **No hoisting**: declare types → functions → calling code.
- Block `module Name { ... }` must be last in file. Alias form `module X = import "..."` has no such limit.
- `include` is allowed in any block, and inside struct/class bodies.
- Comments `//` and nestable `/* /* */ */`. `///` = doc comment: shown by the `--doc*` tools; leading `///` lines at file top describe the module. Bare `{ }` is not a statement (use `if (true) { }`).
- Relative paths resolve from launch dir, not the .os file.
- Alias clashing modules: `HC` = Http (client), `HS` = HttpServer (both define `Response`).
- Nested functions allowed; closure capture [?].

## 2. Imports
```
import "x.os" as X;     import { foo, bar as baz } from "x.os";     import * from "x.os";   // * flattens
```
- `import "*.h"` alone silently becomes `include`; add `as`/list to force import.
- Dotted `import std.console;` = literal path "std.console"; **use string paths**.
- Inside `module {}` bodies use `module X = import "..."` (plain import [?]). Members may take `public`/`private`.
- Everything in `standard/1/std.os` works. Module list (§12).

## 3. Types
- Ints: `int`=`i32`, `i8 i16 i64 i128 i256 i512 i1024`, `bigint`; unsigned `uint`=`u32`, `u8 u16 u64 u128..u1024`, `usize`=`size_t`.
- Floats: `half f16`, `float f32`, `double f64`, `f80`, `f128`. Other: `bool void char char16 char32`.
- Strings: use `char*` and `Str.String`. `string/utf8/utf16/utf32` are scaffolding, avoid.
- Suffixes, source order: `T*` `T**` `T&` `T?` `T[N]` (**postfix only**; `[N]T` not valid). `char*[4]` = array of 4 ptrs; `char[4]*` = ptr to array. Parens: `(fn(T)=>T)[8]`. N>0. Depth caps: 10 ptr, 10 ref, 5 optional.
- Fn pointer: `fn(int, char*) => void` (param names optional).
- Generics: `List<i64>`, `Map<string, List<i64*>>`. Mangling: `*`→ptr `&`→ref `?`→opt `[`→arr; `int`≡`i32` (`List<int>` is `List<i32>`).
- `unique_ptr/shared_ptr/weak_ptr` parse only; stdlib support planned.
- **`nullptr`** → any pointer/ref/fnptr. **`null`** → only `T?` (`let p: T* = null` is an error). `T?`: `.value`, `.valueOr(d)`, compare `== null` [demo].
- Implicit: int→wider int, int→float, float→wider float, `char` acts as small int, `T[N]`→`T*`, `T`→`T?`.
- Literals: int literal starts `i8`, retyped by context (`let x: i8 = 200` silently = -56; use `int`). Bare int=`int`, float=`double`. `0xFF`, `"a\nb"`, `r"raw"`, `'&'`.
- Interpolation: backticks only, `` `n = ${n}` ``. `"{n}"` does not interpolate; `"n=" + 5` is an error. Result type / escaping `${` [?].
- `"a" + "b"` concatenates, `==` compares contents. Casts `x as T`, `sizeof(T)`. No heterogeneous arrays.

## 4. Statements
`let x: int = 5;` `let y = 5;` (reassign keeps type). Ops: `+ - * / %` `== != < <= > >=` `&& || !` `& | ^ << >> ~` `?:` `+= -= *= /= %= ++ --` `&x` `p.field` (**no `->`**) `p[i]`.
`if/else while for(let i: int = 0; i < n; i = i + 1) do{}while switch/case/default break continue return`. `const N: int = 5;`
**Unsupported**: `for (n in list)`, ranges `0..3`, `match`, `try/catch` (tokens exist, not implemented: compile error; use `bool` returns + `db.error()`). Switch fall-through / on `char*`/enum [?].

## 5. Functions
```
function add(a: int, b: int) => int { return a + b; }
function f(a: char*, n: int = 2) => void { }       // default args; no `=> R` means void
let twice = function(x: int) => int { return x * 2; };   // lambda
function id<T>(v: T) => T { return v; }                  // call ONLY as id<i32>(x): no inference from arguments (Call.cpp instantiates from the explicit-type callee name `id_i32`)
function add<T extends i32 | i64 | f32>(a: T, b: T) => T { return a + b; }  // eagerly emits add_i32, add_i64, add_f32
extern "C" fn printf(fmt: char*, ...) => int;
intrinsic fn rsqrt(x: f32) => f32 { }
```
- `<T extends ...>` goes **after** the function name. Multiple constrained params = cartesian product. If any param is unconstrained, constraints aren't enforced and instantiation is lazy.
- Named args: `f(name: "x", count: 3)`; no positional after named; variadics can't be named.
- Overloads: by param types [Call.cpp]. Tier 0 exact (canonical spelling: `int`≡`i32`, `int64`≡`i64`) > 1 compatible (widening, decay, nullptr, T→T?) > 2 coerced. Ties: caller's own module, then first-registered. A typed variable/const (`let n: int`) matches `int` exactly, so `show(int)` vs `show(i64)` works for typed args; only a **bare literal** (`show(5)`, type `i8`) ties both at tier 1. Ranking is skipped when any named argument is used. No return-type-only overloads [?].
- Function pointers: pass by name (target `fn` type picks the overload); call via `arr[i](v)`, `p.fn(v)`, `(*fp)(v)`. **Module-qualified fn as HTTP handler fails: use a local wrapper.**
- Varargs: only extern C. User-defined variadic args are accepted then discarded (packing is commented out in Call.cpp: a compiler gap, should be an error); pass a `List<T>`.
- `T&` params: Call.cpp wraps a by-value arg in `&arg` (positional and named) after the type check. Only doubt: whether `Buf` passes the check against `Buf&` [?]. Safe: `T*` and `f(&b)`; `T&` for copy constructors. If `f(b)` fails, report it as a compiler bug.

## 6. Structs / classes
```
struct Res {
    name: char*;                                // field needs a type or an initializer
    n: int = 0;
    constructor(n: char*) { this.name = n; }
    ~destructor() { }                           // literally "~destructor", not "~Name"
    show() => void { }                          // method
}
let a = Res("a");              // stack; destructor at scope exit
let p: Res* = new Res("h");    delete p;
let arr: Res* = new Res[2]("z");  delete[] arr;
```
- Struct literals: `Pt { x: 1, y: 2 }`, `Pt { 1, 2 }`, `Pt(1, 2)`/`new Pt(1,2)`. With a constructor, args go to it; without, they init fields. Unset fields keep defaults.
- Default ctor/dtor auto-inserted independently. Dtors run on all exits (return/break/continue), reverse order, members after owner; `List<T>` destroys elements.
- Returning a local by value = move. Copying a struct that has a dtor needs `constructor(o: Buf&)`, else pass a pointer.
- `defer stmt;` runs at scope exit, LIFO. `arena a { let s: Str.String(a); let n: Res* = new(a) Res("x"); }` frees at block end.
- Raw: `CMem.malloc(sizeof(T)) as T*` (defaults don't run), `CMem.free(p as void*)`.
- `struct` members default **public**; `class` defaults **private**. Generic: `struct Box<T>`, `class Box<T> : Parent<T>`.
- Modifiers parse on fields/methods (not ctor/dtor): `public private protected static const constexpr mutable thread_local extern virtual override final inline noexcept explicit = delete = default`. Enforcement [?] (assume not enforced). Inheritance `: public Base` parses; the access qualifier is discarded, inherited behavior [?].
- Generic methods/ops: `name<F>(v: T, x: F) => F`, `X<int> can + (o: X<int>*) => X<int> {}`.

## 7. Operator overloading [demo]
`can <op> (params) => ret { }`, inside the struct or free-form (`Num can * (o: Num*) => int {}`; on generics spell `T` as declared).
Ops: `+ - * / %` `== != < > <= >=` `& | ^ << >>` `! ~` `++ --` `+= -= *= /= %=` `[]` `[]=`. `!=` derived from `==`. Unary vs binary = param count.
Pass operands as pointers: `a + &b`. `can [] (i: int)`, `can []= (i: int, v: int)` (`a[i] += v` works).

## 8. Enums
`enum Color { Red, Green }`, `enum class State { Idle }`. Scoping, explicit values, methods, `as int`, switch [?].

## 9. Strings
```
module Str = import "standard/1/String.os";   let s: Str.String;
s.append("abc"); s.appendChar(c); s.appendInt(n); s.appendString(&o); s.appendN(p, len);
s.cstr(); s.len; s.set("x"); s.clear(); s.truncate(n); s.startsWith(..); s.endsWith(..); s.equalsC(..); s.free();
```
`s += "x"`, `s == "x"`, `s[i]` work. `StrView` (`standard/1/StrView.os`, no alloc): `SV.view(cstr)`, `.slice(a,b)`, `.trim()`, `.indexOf(..)`, `==`, `[]`.
`res.html/text/json(char*)` copy their arg: pass `s.cstr()`, then free your own String.

## 10. extern (FFI)
```
extern "C" fn _stricmp(a: char*, b: char*) => int;         // "C"/"C++" = linkage, not a lib
extern "libc.so" fn strlen(s: char*) => u64;
extern "libc.so" let errno: int;   extern "libc.so" const EOF: int = -1;
extern "kernel32.dll" { fn Sleep(ms: u32) => void; const MAX_PATH: int = 260; }   // ';' optional
extern "windows.h", "user32.dll" { }                       // header import: prototypes from .h, link the given lib; body ignored
```
Lib suffix: `.dll` Windows, `.so` Linux, `.dylib` macOS, `.a/.lib` static, none = platform heuristic. Header suffixes: `.h .hpp .hxx .h++ .hh .inl .ipp`.

## 10b. Console (`module Console = import "standard/1/Console.os"`)
```
Console.println(x)    // overloads: char*, i64, f64, bool, Str.String*   (pass &s for a String)
Console.print(char*)  Console.log(char* | i32 | f32)  Console.info/warn/error(char*)   // warn+error go to STDERR; tags [info] [warn] [error]
Console.printString / printlnString / infoString / warnString / errorString (s: Str.String*)
Console.readLine(out: Str.String*) => bool     // one line from stdin, newline stripped; false = EOF/fail
Console.flush()  Console.clear()  Console.setColor(on: bool)  Console.out() / err() => void*  // C FILE* for CFile/printf
```
No string formatting: build text with backtick interpolation or `Str.String`.

## 11. Library usage notes (APIs: ask the user, see §12b)
Signatures are NOT listed here. Behaviour that `--doc-show` can't tell you:
- Import pattern: `module X = import "standard/1/<File>.os";` then `X.Type`, `X.fn()`. Method-style calls on structs (`db.open(..)`, `srv.get(..)`).
- **HttpServer**: handler signature `fn(req: HS.Request*, res: HS.Response*) => void`, must be a local function. Register `serveStatic("/", dir)` LAST (catch-all GET /*). `listen(port)` blocks and returns false on bind failure: always check. One thread per connection, so guard shared globals with a Mutex. `res.html/text/json` take `char*` and copy it. Paths relative to launch dir.
- **Sqlite**: always `?` binds (1-based; columns 0-based), never build SQL from user text. `step()` returns `Sql.ROW`(100)/`Sql.DONE`(101)/error: check on writes. `colText` is valid only until next `step/reset/finalize`: copy it. Wrap a request's DB work in `db.lock()`/`db.unlock()`. Money = integer cents.
- **List<T>**: `free()` releases the buffer, not element contents.
- **Thread**: entry must be `fn(void*) => u32` (local function). `Mutex` needs `init()` before use and `free()` after.
- **Fs / String**: out-params are pointers (`&out`). Caller frees any `Str.String` it receives.

## 12. std.os modules (all work; import path `standard/1/<File>.os`)
- T0 raw C (`standard/1/c/*.os`): CMem CString CFile CConsole CMath CTime COs CStdioExtra CThread CNet CDl CGlfw CVulkan COpenGL COpenAL CCurl CSqlite CSsl CZlib CFfmpeg CImage CFont
- T1: Core Mem String StringView(StrView.os) Math Random Time Arena
- T2: List Collections Map Set Queue Stack Deque PriorityQueue Algo Iter
- T3: Text Regex Json Csv Toml Yaml Xml Binary
- T4: Console IO Sys Path Fs Process Log Cli
- T5: Thread Channel Tasks(Async.os) Net Http HttpServer Ws Tls
- T6: Crypto Compress Db Uuid DateTime Config
- T7: Window Input OpenGL Vk(Vulkan.os) Canvas Image Font Audio Video Gui
- T8: Ffi Reflect Test Bench Debug

Not yet: traits (untested), smart pointer library types, for-in, ranges.

## 12b. Running and discovering APIs
Engine: `.\bin\Release-windows-x86_64\Osengine.exe` (below: `os`). Run from the project root; paths are relative to the launch folder (a leading `/` means the drive root, not the project).
```
os file.os                      // run (JIT is the default)      os - < file.os    // read source from stdin
os file.os --watch              // restart on change (also watches imports); use for servers. --watch-interval <ms>, --watch-no-clear
os file.os --make -o out.exe    // AOT build.  --dry = compile only.  --entry <fn>.  -O 0..3.  --target windows|linux|macos|android|ios|wasm
os a.os b.os                    // extra files are processed too
os --help | --version | --list-targets
```
- Parse errors stop everything ("nothing was compiled or executed"). Errors are descriptive: ask the user to paste the full message.
- Misspelled options get a "Did you mean" hint.

Docs tools (all exit after printing; `<module>` may be written like an import, `std/Console`, or as a path):
```
os --doc-show standard/1/Console.os [--kind method]          // everything in ONE file: signature, summary, line
os --doc-search standard/1/std.os mutex [--kind k] [--limit n]  // <ROOT file> then <query>: searches the whole library
os --doc-list standard/1/std.os [--kind function] [-o -]     // one-line-per-item index of the library
os --doc <module|file> [-o f.md] [--html]    os --doc-all standard/1/std.os [--split] [--html] [-o out]
```
- `--kind`: module function method struct class enum field constant variable type(=struct/class/enum) member(=method/field). `--all` widens output [?].
- Files without `///` comments print bare declarations; files with them also show a summary (Console does).
- Signatures may use the file's module aliases (`L.List`, `Str.String`). Paths are case-insensitive on Windows.
- **For the AI:** for any module not documented here, ask for `--doc-show standard/1/<File>.os`. To check a name exists, ask for `--doc-search standard/1/std.os <name>`.

## 13. Quirks checklist
1. `[N]T` global with `char*` fields may fail: use a local. 2. Module-qualified fn as handler fails. 3. `try/catch` is unimplemented (compile error). 4. `Http.Response.finalUrl` leaks (small). 5. `listen` fails silently unless checked. 6. `nullptr` for `T*`, `null` for `T?`. 7. `new T(arg)` without a constructor inits fields. 8. `import "*.h"` = include. 9. `i8` literal truncation. 10. Block `module` last in file. 11. Prefer string import paths. 12. `T&` params: use pointers. 13. Bare-literal calls tie across `int`/`i64` overloads: type the argument. 14. User variadics drop their args. 15. Generics need explicit `f<i32>(x)` (no inference). 16. Postfix `T[N]` only.

## 14. Open probes [?]
Prefix `[N]T`; plain `import` inside module body; nested-fn capture; `T&` type-check on `f(b)`; return-only overloads; modifier enforcement (`private` access error, `static`, `virtual`, `= delete`); enum semantics; constrained `add(5,6)` (unconstrained inference is confirmed absent); leading `<Number extends ...>` form (parser expects it after the name); backtick result type and `${` escape.