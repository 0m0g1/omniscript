# OmniScript: making it high-level without losing low-level

Goal: someone can write a web server, a parser or a UI without ever writing `malloc`/`free`, and someone who wants raw control
can still have it. Safe by default, manual by choice.

Status of this document: a plan from what I have seen of the language through the browser, pharmacy server and Ui code (about
a hundred files). I have not seen the compiler source. Items marked **[verify]** are assumptions to confirm with a tiny test
before building on them (same method as the `qt/` quirk tests).

---

## 1. Where the language is today (as observed)

- Structs with methods, pointers (`T*`), `as` casts, modules (`module X = import "..."`), function pointers (`fn(A*) => void`).
- Manual memory: `CMem.malloc/free`, `sizeof(T)`, and owning library types (`Str.String`, `Db`, `Stmt`, ...) with a manual `.free()`.
- Struct field defaults run for stack locals (confirmed, q7) but not for malloc'd memory (confirmed, q8).
- Works (confirmed by tests): negative literals, ternaries, `return` inside loops, module-level `char* = nullptr`, function pointers by name.
- Open compiler bug: passing a module-qualified function (`PA.loginPage`) as a handler gave "Member not found in class" **[verify with a test]**.

What hurts high-level users today, measured on my own code: every `String` needs `.free()`, every helper like `U.sub`/`U.cat` allocates
and nobody owns the result (so it leaks), early `return` skips cleanup, and long-lived objects need hand-written destroy functions.

---

## 2. Principles

1. **Safe by default.** Plain `let`, `String`, `List`, `Map` need no free and cannot be double-freed.
2. **Cost is visible.** Everything is deterministic (scope-based), no hidden GC pauses, so the same language works for the UI and drivers.
3. **Manual is explicit.** `CMem.malloc`/`free` and raw pointers stay, behind a name that says so (`unsafe`).
4. **One feature at a time, each with a test file.** Nothing is "done" until a `qt`-style test prints PASS on your engine.

---

## 3. Phases

### Phase 0 - Facts and small fixes (1-2 days)
- Write and run the **[verify]** tests: module-qualified function pointer, struct init syntax (`Pt{}`?), generics support, closures, string interpolation, slices.
- Fix compiler gaps found so far: **defaults on malloc'd structs** (give a `new T` that allocates + runs defaults), the module-qualified handler bug.
- Output: `docs/language_facts.md` listing what is and isn't supported, so nobody (me included) guesses again.

### Design decision (2026-11-26): C++ style, not Rust style
Freedom over safety: constructors, `~Destructors`, `new`/`delete`/`new[]`/`delete[]`, free aliasing and raw pointers, no borrow checker.
The only compiler guard: copying a struct that has a destructor needs a copy constructor, a pointer, or `[trivialCopy]`
(returning a local by value is an automatic move). `defer`, arenas, `Owned<T>`/`Shared<T>` (library), optionals `T?` with `.value`/`.valueOr`.
Reference programs: `ownership_demo.os` (16 sections, self-checking) and `destructor_demo.os`. Where this section differs from the
Phase 1 text below (the `drop()` name, move-only values), this section wins.

### Phase 1 - Scope-based ownership (core, biggest win)
- **Destructor**: a struct may define `drop() => void`. The compiler calls it when a local goes out of scope: end of block, `return`, `break`, `continue`.
- **Move on return / assignment of owning values**: returning a `String` transfers ownership; the local is not dropped. Rule: an owning value has exactly one owner.
- **Copy is explicit**: `s.copy()` or `clone(s)`. Passing to a function borrows by default (callee cannot free it).
- **`defer expr;`** runs at scope exit, for anything without a destructor.
- Library: give `String`, `List`, `Map`, `Db`, `Stmt`, `Response` etc. a `drop()` that does what `.free()` does now. Keep `.free()` as an alias for early release.
- Tests: leak counter (malloc/free tally in `CMem` debug mode), early-return, nested blocks, double-drop impossible.

Compiler work: a pass that knows which types have `drop`, tracks moved-from locals, and inserts drops on every exit edge.

### Phase 2 - Library that doesn't allocate behind your back
- `StrView` (pointer + length) for `sub`, `trim`, `split`, `lines`; only `String` owns. Parsers (HTML, CSS, Markdown, CSV, JSON, the JS lexer) become allocation-free.
- `List<T>` / `Map<K,V>` with iteration, `push`, `get`, `has`, `remove`; values dropped with the container.
- `Option`/null-safe returns: functions return `T?` or a `Result`, so `nullptr` checks stop being the only error channel.
- Rewrite `util.os` helpers to the view-returning versions; keep the old names as copies (`dup`, `cat`).

### Phase 3 - Arenas for bulk lifetimes
- `arena a { ... }` or `let a = Arena(); a.alloc(T)`: one block, freed all at once.
- Use cases already planned: one arena per page (HTML + CSS + JS), per HTTP request (HttpServer), per JS run.
- Library types take an optional allocator: `String(a)`.

### Phase 4 - Everyday ergonomics
- Type inference: `let x = f();` **[verify]**.
- String interpolation: `"Total: {n} items"` (no more chains of `U.cat(U.cat(...))`).
- `for (item in list) { }` and ranges `for (i in 0..n)`.
- Lambdas/closures for handlers and callbacks (`server.get("/", (req, res) => { ... })`), capturing by reference, no wrapper functions.
- Default and named arguments; `switch`/`match` on ints and strings.
- Better errors: `try`/`?` propagation for `Result` types instead of manual null checks.

### Phase 5 - Optional managed objects (only if needed)
- `class` = reference-counted heap object, `struct` = value/scoped. Weak references for cycles (DOM parent pointers).
- Decide after Phases 1-3 are used on the browser and server: it may turn out not to be needed.

### Phase 6 - Tooling
- Clear diagnostics with line, column and a one-line hint ("`x` was moved here and used there").
- Formatter, `--check` (compile without running), leak report on exit in debug builds.
- A short "OmniScript for beginners" guide that uses only Phases 1-4 features.

---

## 4. Proof it worked (measurable)

| Metric | Today | Target |
|---|---|---|
| `.free()` / `CMem.free` calls in browser + pharmacy code | many (count with grep) | 0 outside `unsafe` and the library |
| Leaks after loading 20 pages (debug counter) | unknown | 0 |
| Lines in `markdown.os` / `csv.os` | current | about 30% fewer (views + interpolation) |
| A web endpoint in the pharmacy app | `h_*` wrapper + manual strings | one lambda, no free |

Migrate in this order, one file at a time, sending changed files individually: stdlib (`String`, `List`) -> `util.os` -> viewers ->
`pharmacy/server/*` -> Ui (last, it is the performance-critical one).

---

## 5. Order and rough effort (solo, with testing)

1. Phase 0: facts + fixes - 1-2 days
2. Phase 1: destructors, moves, `defer` - 1-2 weeks (the hardest compiler change)
3. Phase 2: views and collections - about 1 week
4. Phase 3: arenas - 2-3 days
5. Phase 4: interpolation, `for-in`, lambdas, `Result` - about 2 weeks, in any order
6. Phase 5 and 6: after real use

Highest value per effort: **Phase 1 + `StrView` from Phase 2 + string interpolation.** That alone removes most manual memory and most of
the visible clumsiness.

---

## 6. Decisions (answered)

1. **Engine:** a JIT and native codegen, both. Drops are inserted in the shared front end (IR) so both back ends behave the same.
   Arenas can use bump allocation in both.
2. **Generics:** exist, so Phase 2 (`List<T>`, `Map<K,V>`, `Option<T>`/`Result<T,E>`) can start right away. **[verify]** the syntax.
3. **Backward compatibility: yes.** `drop()` is opt-in per type; old `.free()` calls stay valid (a freed value is marked moved so the
   auto-drop does not free it again). New features add syntax, they never change what old code means.
   Exception, since the language is early and has no real users: where an existing rule is wrong, change it now with a one-line migration
   note instead of carrying it forever. Candidates: `new T` so malloc'd structs run defaults, module-qualified function pointers, and whether
   `sub`/`trim` return views instead of copies. Add an `#edition` marker only once other people depend on the language.
4. **Hybrid identity:** high-level code uses scoped types, views, lambdas and interpolation; low-level code keeps `unsafe`/`CMem`,
   raw pointers and explicit layout. Both compile to the same IR, so they mix in one file.
5. **Threads:** open. Start with the rule "owned values move between threads, never share without a Mutex/Atomic" and enforce later.
