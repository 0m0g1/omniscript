# OmniScript syntax guide (v1)

Tags: **[ok]** seen working in real code, **[test]** confirmed by a probe, **[demo]** from `docs/examples/ownership_demo.os`, which you report now passes in full,
**[verify]** not yet seen working. Rewrite any line the compiler disagrees with.

---

## 1. Program layout

```
module Console = import "standard/1/Console.os";   // import with alias [ok]
include "standard/1/Ui/Api.os";                    // textual include, order matters [ok]

let LIMIT: int = 100;                              // module-level variable [ok]

function main() => int { return 0; }               // main() runs automatically [ok]
```

- **Top-down scripts need no `main`.** Statements at file level run in order, like a Python script. `main()` is optional; if present it runs automatically. **A call must come after the declaration it uses** (no hoisting, no forward references), so order the file: types, then functions, then the code that calls them. [from you] This is also why `main()` can sit at the bottom of a file and call everything above it. Mutually recursive functions would need a workaround: **[verify]** whether forward declarations exist.
- Scripts run from the project root, so relative paths resolve from **where you launch the exe**, not from the .os file. [ok]
- Comments: `// line` and `/* block */`. **Block comments nest**: `/* outer /* inner */ still a comment */`, so you can comment out code that already contains block comments. [from you]
- Statements end with `;` (missing `;` gives confusing spans). Blocks use `{ }`. Bare `{ }` blocks are not statements: use `if (true) { }`.
- Alias modules distinctly. `Http` (client, libcurl) and `HttpServer` both define `Response`; mixing them gives "Method not found" errors. Use `HC` for client, `HS` for server.

## 2. Types

| Type | Notes |
|---|---|
| `int` | 32-bit signed |
| `i64`, `u64`, `u32`, `u16`, `u8` | fixed width |
| `float`, `double`, `bool` | |
| `char*` | C string; `s[i] as int` reads a byte |
| `T*`, `void*`, `nullptr` | pointers |
| `[N]T` | fixed array: `let buf: [4096]u8;` |
| `fn(A, B) => R` | function pointer type |
| `L.List<T>` | generic list; own generics `struct Box<T>` [test] |
| `T?` | optional: `.value`, `.valueOr(d)`, compare with `null` [demo] |

Casts `x as T`, `sizeof(T)`. Literals: decimal, hex `0xFF`, `"a\nb"`, raw `r"..."`, char `'&'`.
Strings: `==` compares contents; `"a" + "b"` concatenates; `"n=" + 5` is an error, use interpolation.
Interpolation: backticks `` `n = ${n}` `` also work for multi-use quoting (`"` needs no escape inside). `"{n}"` does not interpolate. [ok]

## 3. Variables, operators, control flow

```
let x: int = 5;   let y = 5;   x = x + 1;      // reassignment must keep the type
```
`+ - * / %`, `== != < <= > >=`, `&& || !`, `& | ^ << >>`, ternary `c ? a : b`, `+= -= *= ++ --`, `&x`, `p.field` on pointers (no `->`).
`if/else`, `while`, `for (let i: int = 0; i < n; i = i + 1)`, `do {} while`, `switch/case/default`, `break`, `continue`, `return` inside loops (works; HttpServer.os still avoids it).
**Not supported yet:** `for (n in list)`, ranges `0..3`, `try/catch` (silently does nothing), `match` [verify].

## 4. Functions

```
function add(a: int, b: int) => int { return a + b; }
let twice = function(x: int) => int { return x * 2; };     // lambda
function f(a: char*, n: int = 2) => void { }               // default args
```
**Overloading**: several functions may share a name if their parameter lists differ; the call picks by argument types. [from you]
```
function show(n: int) => void { }
function show(s: char*) => void { }
function show(a: int, b: int) => void { }
show(5); show("x"); show(1, 2);
```
**[verify]** whether the return type alone can differ (usually not allowed), how it resolves `int` vs `i64` literals, and whether methods and constructors overload the same way.

Pass a function by name (with overloads, the target `fn(...)` type decides which one). **[verify]** A module-qualified function (`PA.page`) failed as an HTTP handler: use a local wrapper.

## 5. Structs, methods, constructors, destructors [demo]

```
struct Res {
    name: char*;
    constructor(n: char*) { this.name = n; }
    ~destructor() { /* cleanup */ }
    show() => void { }                         // method, uses `this`
}
let a = Res("a");              // stack object: destructor runs at scope exit, reverse order
let p: Res* = new Res("h");    // heap: constructor runs, struct defaults run
delete p;                      // runs ~destructor(), then frees
let arr: Res* = new Res[2]("z");  delete[] arr;
```

- Destructors run on every exit path (`return`, `break`, `continue`). Members are destroyed after the owner's destructor; `L.List<T>` destroys its elements.
- Returning a local by value is a **move** (no copy, no extra destructor).
- Aliasing and raw pointers are allowed (C style). The only compiler guard: copying a struct that has a destructor needs a **copy constructor** `constructor(o: Buf&)`, or pass a pointer.
- `defer stmt;` runs at scope exit, last in first out, also on early return. Use it for raw handles with no destructor: `defer db.close();`
- `arena a { let s: Str.String(a); let n: Res* = new(a) Res("x"); }` frees everything at the end of the block.
- Struct literals `Pt { x: 1, y: 2 }`, enums `enum Color { Red, Green }`, `const N: int = 5;`.
- Old manual way still works: `CMem.malloc(sizeof(T)) as T*`, then set every field yourself (defaults do not run on raw malloc). `CMem.free(p as void*)`.

### Classes

`class` is like `struct` (same fields, methods, constructors, destructors, operators), but members can carry **access modifiers**. [from you]
```
class Account {
    private balance: i64 = 0;          // modifier before the member
    public owner: char*;
    public deposit(n: i64) => void { this.balance = this.balance + n; }
}
```
Modifiers the compiler's AST (`MemberModifiers`) knows about:

| Group | Modifiers |
|---|---|
| Access (one of) | `public`, `private`, `protected` |
| Storage | `static`, `const`, `constexpr`, `mutable`, `thread_local`, `extern` |
| Inheritance | `virtual`, pure virtual, `override`, `final` |
| Function | `inline`, `noexcept`, `explicit`, `= delete` (deleted), `= default` |

The AST only proves the compiler can **represent** these. **[verify]** each with a probe: which are accepted by the parser, which are enforced by the semantic pass (e.g. does accessing a `private` member from outside give an error?), the default access when none is written, and the exact keyword spelling (`thread_local` vs `threadLocal`, pure virtual syntax). Earlier notes said "no inheritance observed"; `virtual`/`override`/`final` suggest it may exist for classes, so probe that too. Use `struct` for plain data (like `Header`, `Request`) and `class` when you want to hide internals.

## 6. Operator overloading [demo]

Inside a struct: `can <op> (params) => ret { body }`. Operands are passed as written, so parameters are pointers: `a + &b`.

```
struct Num {
    v: int = 0;
    can + (o: Num*) => int { return this.v + o.v; }
    can == (o: Num*) => bool { return this.v == o.v; }   // != is derived as !(==)
    can += (o: Num*) => void { this.v = this.v + o.v; }
    can ! () => bool { return this.v == 0; }             // unary: no parameters
    can - () => Num { ... }                              // unary minus
    can -- () => int { ... }                             // a-- and --a
    can [] (i: int) => int { ... }                       // read
    can []= (i: int, v: int) => bool { ... }             // write; a[i] += v works
}
Num can * (o: Num*) => int { return this.v * o.v; }      // add an operator outside the struct
Pair<T> can + (o: Pair<T>*) => T { ... }                 // on a generic: spell T as in the declaration
```
Primitives and plain pointers keep their normal operators. `String` supports `s += "x"`, `s == "x"`, `s[i]`; `StrView` supports `==` and `[]`.

## 7. Strings (standard library)

```
module Str = import "standard/1/String.os";
let s: Str.String;
s.append("abc"); s.appendChar(c); s.appendInt(n); s.appendString(&o); s.appendN(p, len);
s.cstr(); s.len; s.set("x"); s.clear(); s.truncate(n); s.startsWith(..); s.endsWith(..); s.equalsC(..); s.free();
```
`StrView` (`standard/1/StrView.os`): `SV.view(cstr)`, `.slice(a, b)`, `.trim()`, `.indexOf(..)`: no allocation. [demo]
Strings you build with `Str.String` are copied by `res.html(...)`, so free your own temporary afterwards (or let the destructor do it).

## 8. Foreign functions

```
extern "path/to/lib.dll" { fn sqlite3_open(path: char*, db: void**) => int; }
extern "C" fn _stricmp(a: char*, b: char*) => int;
```

## 9. HttpServer (`standard/1/HttpServer.os`)

```
module HS = import "standard/1/HttpServer.os";
function health(req: HS.Request*, res: HS.Response*) => void { res.json(`{"ok":true}`); }
function main() => int {
    let srv: HS.HttpServer;
    srv.get("/api/health", health);       // get/post/put/del/route("*", ...)
    srv.get("/pharmacy", pharmacyPage);
    srv.serveStatic("/", "docs/examples/computer_assignment/public");   // LAST: it is a GET /* catch-all
    if (!srv.listen(8090)) Console.println("bind failed");              // blocks; false = bind failed
    srv.free();
    return 0;
}
```

- Model: one thread per connection, `Connection: close` on every response, first matching route wins, 405 if only the method differs, else 404 (or `srv.notFound`).
- Patterns: exact segments, `:name` captures (`req.param("name")`, raw), trailing `/*` (`req.rest`).
- Request: `req.method/path/query/body` (`Str.String`), `req.header("x")` (case-insensitive), `req.queryParam("k", &out)` (decoded; free `out`).
- Response: `res.status = 201`, `res.header(k, v)`, `res.text/html/json(char*)`, `res.redirect(url, code)`, `res.sendFile(path)` (MIME from extension, max 128 MiB).
- Serving HTML files: put them in the static folder (`GET /` gives `index.html`), or send one from a route: `res.sendFile("public/pages/drugs.html")`.
- Static folder paths are relative to the launch directory. `..`, `\` and `:` in URLs are rejected.
- Limits: header 64 KiB, body 8 MiB (needs Content-Length; chunked gets 501), 512 connections, 15 s timeout. HTTP only.
- Handlers run concurrently: guard shared globals with a `Th.Mutex`.
- `std.os` still marks HttpServer `// stub`; it is stale.

## 10. Sqlite (`standard/1/Sqlite.os`)

```
module Sql = import "standard/1/Sqlite.os";
let db: Sql.Db;
db.open("data/app.db");                  // enables foreign_keys, WAL, 5 s busy timeout
db.exec("CREATE TABLE IF NOT EXISTS t(id INTEGER PRIMARY KEY, name TEXT NOT NULL)");
let st: Sql.Stmt;
if (db.prepare("INSERT INTO t(name) VALUES(?)", &st)) { st.bindText(1, "x"); st.step(); st.finalize(); }
if (db.prepare("SELECT id, name FROM t WHERE id > ?", &st)) {
    st.bindInt(1, 0);
    while (st.step() == Sql.ROW) { Console.println(st.colText(1)); }
    st.finalize();
}
```
- Bind indexes are 1-based, columns 0-based. `step()` returns `Sql.ROW` (100), `Sql.DONE` (101), else an error: check it on writes.
- `colText` never returns nullptr (`""` for NULL) and is valid only until the next `step/reset/finalize`: copy it before keeping it.
- `db.lastId()`, `db.changes()`, `db.scalarInt(sql, default)`, `db.error()` (read right after a failure).
- Transactions: `db.begin()` / `db.commit()` / `db.rollback()`. Threads: wrap a request's whole DB work in `db.lock()` ... `db.unlock()`.
- Money is integer cents. Always use `?` binds, never build SQL from user text.

## 11. Standard modules

`Console`, `String`, `StrView`, `Text`, `Json`, `Http` (client), `HttpServer`, `Net`, `Sqlite`, `Crypto`, `Thread` (`Mutex`, `Atomic`), `Channel`, `Fs`, `Time`, `List<T>`, `Canvas`, `Image`, `Ui`.

## 12. Known quirks

1. `[N]T` as a module-level global with `char*` fields may fail: use a local.
2. Module-qualified function as an HTTP handler fails: use a local wrapper.
3. `try/catch` does nothing: use `bool` returns and `db.error()`.
4. `Http.Response.finalUrl` is malloc'd and never freed (small leak per client request).
5. `listen` fails silently unless you check its return value.

## 13. Next steps

1. Add `///` doc comments to `HttpServer.os` and `Sqlite.os` so `--doc-show` becomes real documentation.
2. Probe: backtick string result type, multi-line backticks, `${call()}` inside backticks, literal `${`.
3. Add `for (n in list)` and ranges (still parse errors).
4. Optional later: decorators as sugar for route registration (see chat notes).
