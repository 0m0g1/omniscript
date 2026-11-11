# OmniScript Standard Library: Plan (v2, post first draft)

Status of the library after the first full draft. The original module plan (`OMNISCRIPT_STD_MODULE_PLAN.md`) is still the source of truth for scope and tiers; this document tracks **what exists, what is verified, what is blocked, and what to do next**.

Every draft file was written against the dialect inferred from `Console.os`, `List.os`, `String.os`, `Core.os` and the `c/` bindings, and **has not been compiled**. Treat all "draft" modules as unverified until checked.

---

## 1. Goals and escape hatches

Goal: nobody is blocked building a CLI tool, server, game, desktop app, data pipeline or systems program. Escape hatches stay available at every tier:

1. Import a raw `c/` module and call the C API directly.
2. Load any C library with `extern "lib.dll" { ... }` or a header import (`Ffi` for runtime loading).
3. Use raw pointers with `Mem` and `c/mem`.

---

## 2. Conventions (decisions already made; new modules must follow them)

**Layout and naming**
- Files: `standard/1/<PascalCase>.os`. Raw bindings: `standard/1/c/<lowercase>.os`.
- First line of every file is its path as a comment, followed by a short tier/purpose comment.
- Import style: `module X = import "standard/1/X.os";`. Keep aliases short but never equal to a generic parameter name (`T`, `K`, `V`). Use `Th`, `M`, `L`, `D`, etc.
- Tier rule: a module may import only modules in its own or an earlier tier, and never form a cycle. Check this with a script (section 8).

**Error handling and ownership**
- No `Result<T,E>` or `T?` yet. Fallible methods return `bool` and write results through out-pointers; "not found" lookups on pointers return `nullptr`.
- Types that own memory expose `free()`. There are no destructors or `defer` yet, so callers free by hand.
- Never copy an owning struct with `=` (double free). Use `clone(&out)` where provided.
- For types that own children (List of String, Json, Xml, Csv.Table), free the elements first, then the container. Helpers like `Text.freeParts` and `Csv.Table.free` exist for this.

**Generics and callbacks**
- No traits yet. Hashing, equality and ordering are passed as `fn(...)` parameters (`Map.init(hash, eq)`, `Algo.sort(a, n, cmp)`).
- Ready-made helpers: `hashU64/eqU64`, `hashI64/eqI64`, `hashCStr/eqCStr`, `cmpI64/cmpU64/cmpF64`, and `StrMap<V>` for `char*` keys (keys are not copied).

**Platform**
- Windows first (Win32 threads, WinSock2, BCrypt, `QueryPerformanceCounter`, `_popen`, `__acrt_iob_func`). Every platform-specific spot is marked with a `PLATFORM` comment naming the POSIX swap.

**Keywords to avoid as identifiers** (from the lexer): `if else while for continue break return function fn let var namespace using new delete struct class extends variant any enum public private protected override virtual static final const true false nullptr null xor include import from module mod extern intrinsic volatile as type`.
Already renamed for this reason: `any/all` → `anyOf/allOf`, `delete` → `del`, `fn<F>` → `symbolAs<F>`, parameters `type`→`kind`, `from`→`src`.

**Style**
- `snake`-free: functions and methods camelCase, types PascalCase, constants either `UPPER_SNAKE` (C-derived) or camelCase.
- Light comments only: purpose, ownership, and `TODO` markers with what is needed.

---

## 3. Compiler feature checklist

Run one tiny probe file per row. Record the result in the Status column. A failing row blocks every module listed.

| # | Feature | Used by | Status |
|---|---|---|---|
| F1 | Generic structs `S<T>` with methods | List, Map, Set, Queue, Stack, Deque, PriorityQueue, Channel | List works (reference) |
| F2 | Generic free functions `f<T>(...)` and explicit `f<T>(...)` calls | Algo | unverified |
| F3 | Function-pointer types `fn(A) => R` as params | Algo, Map, PriorityQueue, Thread | unverified |
| F4 | `fn(...)` types as struct fields and globals, default `= nullptr` | Map, PriorityQueue, OpenGL, Channel.Job | unverified |
| F5 | Cast function ↔ `void*` (`f as void*`, `p as fn(...)=>R`) | Thread, Http, OpenGL.load, Ffi | unverified |
| F6 | Fixed arrays `[N]T`, `&arr as T*`, indexing | Json, Net, Input, Uuid, DateTime, Vulkan | unverified |
| F7 | Struct literal `T{}` and `let x: T;` zero-init | Csv, most structs | unverified |
| F8 | Nested struct fields by value (`m: Map<K,V>` inside `StrMap`) | StrMap, Set, Queue, Stack, ThreadPool | unverified |
| F9 | Methods named like globals (`push`, `get`) without clashes | all | unverified |
| F10 | `0x...` and large u64 literals (`0xFFFFFFFFFFFFFFFF`, FNV constants) | Map, Net | unverified |
| F11 | `u8/u16/u32/u64/i32/i64/float/double` casts with `as`, shifts and bitwise ops on them | Binary, Uuid, Text | unverified |
| F12 | `module X = import` inside generic-using files, qualified types `X.Type<T>` | Set, Queue, Stack, Channel | unverified |
| F13 | `extern "C" fn` with varargs (`...`) and `fn` pointer params | Console, Process, Http | printf works |
| F14 | `continue`/`break` inside `while (true)` | Regex, Cli, Json, Xml | unverified |
| F15 | Taking address of struct fields (`&this.cs`) and of array elements (`&a[i]`) | Thread, Algo, Vulkan | unverified |
| F16 | `sizeof(T)` for generic `T` and struct types | List (works), Map, Json | List works |

**Language features that block modules entirely** (not draft-able yet): iterator trait + closures (Iter), `async/await` (Async), destructors/`defer` (auto-free), user variadics (`printf`-style `String.format`), compiler type metadata (Reflect), traits (typed `Map` without fn params), `T?`/`Result<T,E>`/`??`.

---

## 4. Module status

Legend: **ref** = existing, compiles (your reference files); **draft** = written, uncompiled; **stub** = signatures + TODO bodies; **blocked** = waits on a compiler feature.

### Tier 0: raw C (`standard/1/c/`)
| Module | Status | Notes |
|---|---|---|
| mem, string, file, console, math, time, os | ref/yours | not part of the draft; the draft assumes `mem` has `calloc` and `memset`, `string` has `strtod strtoull strrchr strchr strncmp` |
| stdio_extra | draft | sscanf, snprintf, fopen/fclose/fread/fwrite (merge into `file`/`console` later) |
| thread | draft | Win32 threads, critical sections, condition variables, Interlocked* |
| net | draft | WinSock2 incl. UDP, `inet_pton/ntop` |
| dl | draft | LoadLibrary/GetProcAddress |
| glfw | draft | windowing, input, Vulkan surface |
| vulkan | draft | ~50 core + KHR functions |
| opengl | draft | GL 1.1 symbols; GL ≥ 2 loaded at runtime through `glfwGetProcAddress` |
| openal | draft | binds miniaudio-style API; swap for real OpenAL if wanted |
| curl, sqlite, ssl, zlib, image (stb), font (stb_truetype) | draft | link requirements in section 6 |
| ffmpeg | draft | struct field access needs a C shim |

### Tier 1: core
| Module | Status |
|---|---|
| Core, Mem, String, Math, Random, Time | ref/yours (Mem `Arena/Buffer`, Math `Vec/Mat4`, Result, Test, Fs, IO, Sys assumed present) |

### Tier 2: collections and iteration
| Module | Status | Depends on |
|---|---|---|
| List | ref | F1 |
| Map (+StrMap, hash helpers) | draft | F1, F3, F4, F8, F10 |
| Set | draft | Map |
| Deque | draft | F1 |
| Queue, Stack | draft | Deque, List |
| PriorityQueue | draft | List, F3, F4 |
| Algo (sort, binarySearch, indexOf, reverse, forEach, count, anyOf, allOf, minOf, maxOf, cmp helpers) | draft | F2, F3 |
| Collections (facade importing all of the above) | draft | |
| Iter | stub/blocked | iterator trait, closures |

### Tier 3: text and data formats
| Module | Status | Known gaps |
|---|---|---|
| Text (split, join, pad, parse, escape, utf8, hex, url) | draft | |
| Regex | draft | no groups/captures, top-level `|` only, 256-char alternative limit |
| Json | draft | DOM with `JsonValue*` ownership; numbers via `%.17g` |
| Csv | draft | quoted fields, CRLF, custom delimiter |
| Xml | draft | no DTD/namespaces; entities limited to five |
| Binary (ByteWriter/ByteReader, toHex) | draft | |
| Toml, Yaml | stub | target: parse into `Json.JsonValue` |

### Tier 4: I/O, filesystem, process
| Module | Status | Known gaps |
|---|---|---|
| Console, IO, Sys, Fs | ref/yours | |
| Path | draft | `exists/isFile/isDirectory` belong in `Fs.stat` (avoid Path→Fs cycle) |
| Process | draft | `spawn/wait/kill/pipes` TODO (CreateProcess + pipes) |
| Log | draft | file sink append only; no rotation |
| Cli | draft | no subcommands; `usage()` text only |

### Tier 5: concurrency and networking
| Module | Status | Known gaps |
|---|---|---|
| Thread (Thread, Mutex, Cond, Atomic i64, yield, cpuCount) | draft | no RwLock; Atomic not generic |
| Channel (+ThreadPool) | draft | |
| Net (TCP, UDP, resolve) | draft | IPv4 only for listen/UDP; blocking sockets |
| Http (client) | draft | curl option constants hand-copied: verify against your libcurl version |
| HttpServer, Ws, Tls | stub | build on Net + Crypto |
| Async | stub/blocked | compiler support |

### Tier 6: security, storage
| Module | Status | Known gaps |
|---|---|---|
| Crypto (sha256/sha1/md5/hmac/random/base64/constantTimeEquals) | draft | AES and password hashing TODO |
| Compress (zlib one-shot, crc32, adler32) | draft | gzip/zip containers and streaming TODO |
| Db (SQLite wrapper) | draft | no row-struct mapping |
| Uuid (v4, v7-style) | draft | |
| DateTime | draft | UTC only; no time zones, no `format(fmt)` yet |
| Config (INI) | draft | no env overlay yet |

### Tier 7: graphics, windowing, audio, media
| Module | Status | Known gaps |
|---|---|---|
| Window, Input | draft | no event callbacks, no gamepad, no text input |
| OpenGL | draft | `Mesh` supports one attribute; no uniform helpers beyond raw fn pointers |
| Vulkan | draft | bring-up only (instance → swapchain → sync + pool). Render pass, pipeline, framebuffers, draw loop TODO |
| Image, Font | draft | Font needs a glyph atlas on top |
| Audio | draft | needs a miniaudio C shim; no 3D |
| Canvas, Gui, Video | stub | Canvas first, Gui sits on it |

### Tier 8: tooling and interop
| Module | Status |
|---|---|
| Ffi | draft |
| Bench, Debug | draft |
| Test | ref/yours |
| Reflect | stub/blocked |

---

## 5. Recommended implementation order

Enable one group at a time in `std.os` (commented lines are in tier order). Compile after each group; fix before moving on.

0. **Probes.** Write the section 3 probe files. Resolve F1–F16 results first; a failing feature changes the rest of the order.
1. **Collections core:** Map → Deque → Stack → Queue → Set → PriorityQueue → Algo → Collections facade. Map is the best canary (F3, F4, F8, F10 at once).
2. **Pure-code data modules:** Text → Path → Binary → Json → Csv → Regex → Xml. No external libraries, easiest to verify.
3. **System utilities:** Process → Log → Cli → Config → DateTime → Uuid → Bench → Debug.
4. **Concurrency and network:** Thread → Channel (+ThreadPool) → Net → Http. Test Thread before Channel.
5. **Windowing and graphics:** Window → Input → OpenGL → Image → Font → Audio → Vulkan (finish the draw loop).
6. **Security and storage:** Crypto → Compress → Db (finish gzip/zip and AES after).
7. **Stubs, as the compiler allows:** Canvas → Gui → Yaml/Toml → HttpServer → Ws/Tls → Video → Iter/Async/Reflect.

Per module, the loop is: enable in `std.os` → compile → fix → write a `Test` program → mark **tested** in section 4.

---

## 6. External dependencies and shims

| Module | Needs |
|---|---|
| Thread, Net, Process, Uuid, Bench | Windows system DLLs (kernel32, ws2_32, bcrypt); no extra install |
| Http | libcurl |
| Db | sqlite3 |
| Crypto | OpenSSL libcrypto |
| Compress | zlib |
| Window, Input, OpenGL | GLFW 3, opengl32 |
| Vulkan | vulkan-1 (loader) + GLFW |
| Image, Font | stb_image, stb_image_write, stb_truetype compiled into a small C shim exporting plain C symbols |
| Audio | miniaudio compiled into a shim exporting the `ma_*` symbols used in `c/openal.os` (or rewrite against OpenAL) |
| Video | FFmpeg libs + a shim for struct field access |

---

## 7. Known gaps and TODO register

- **Regex:** capture groups, `findAll`, `replace`, `compile` object, bounded repeats `{n,m}`, longer alternatives.
- **Compress:** gzip/gunzip, zip read/write, streaming (needs `z_stream` layout).
- **Crypto:** AES (GCM via EVP), password hashing (PBKDF2 in libcrypto; bcrypt/argon2 need a library).
- **Process:** `spawn/wait/kill`, pipes, env overlays for children.
- **Thread:** `RwLock`, generic `Atomic<T>`, thread-local storage.
- **Net:** IPv6, non-blocking/poll, timeouts, `Address` parsing type.
- **Http:** headers on responses, streaming bodies, multipart, cookies, redirects control.
- **DateTime:** time zones, `format(fmt)`, parse with offsets, `unixSeconds()` helper.
- **Config:** environment overlay and defaults API.
- **Input:** callbacks, gamepad, text input (needs GLFW callbacks → function pointers into OmniScript).
- **OpenGL:** multi-attribute VAO builder, uniform helpers, `Texture` type (plan lists `Texture.load`, `Buffer`, `VertexArray`).
- **Vulkan:** render pass, pipeline, framebuffers, command recording, draw loop, swapchain recreate, validation layers.
- **Font:** glyph atlas, `measure`, `render(text)`.
- **Audio:** 3D position, streaming.
- **Path:** `exists/isFile/isDirectory` (via `Fs`).
- **Base64 placement:** plan puts it in `Text` (Tier 3); it lives in `Crypto` (Tier 6). Either implement standalone in `Text` or accept the deviation.
- **`Result<T,E>` / `T?` migration:** once supported, convert out-pointer APIs in a single pass.
- **Destructors/`defer` migration:** add auto-free for String, List, Map, etc.

---

## 8. Quality gates

1. **Compiles:** the module builds when enabled in `std.os`.
2. **Tested:** one program in `Test` per module exercising the happy path and one failure path. Suggested starter tests:
   - Map: insert 10k keys, remove half, verify `len` and lookups; StrMap with string keys.
   - Json: parse → stringify round-trip on nested input; malformed input returns `nullptr`.
   - Csv: quotes, embedded commas/newlines, CRLF.
   - Regex: anchors, classes, `+ * ?`, alternation, no-match.
   - Channel/ThreadPool: N producers, M consumers, total sum matches.
   - Net: loopback TCP echo; UDP send/recv on 127.0.0.1.
   - Crypto: known SHA-256/HMAC vectors; base64 round-trip.
   - DateTime: Unix epoch, leap-year dates, `parseIso` ↔ `iso`.
3. **Tier-rule check:** a small script that parses `module X = import "…"` lines and fails if a module imports a later tier or forms a cycle. Run it before each commit.
4. **Keyword check:** grep each new file for the lexer keywords used as identifiers.
5. **Leak check:** run Map/Json/Xml/Csv tests under a leak-reporting allocator (`Debug`) and confirm every `free()` path.
6. **Docs:** each module's header comment states ownership rules, one usage snippet, and its platform notes.

---

## 9. Milestones

| Milestone | Definition of done |
|---|---|
| M0: Probes | F1–F16 recorded; blocked modules identified |
| M1: Everyday scripting | Collections, Text, Path, Binary, Json, Csv, Process, Cli, Log compile and pass tests; user can write real CLI tools |
| M2: Network | Thread, Channel, Net, Http compile; loopback and one real HTTP GET verified |
| M3: Games | Window, Input, OpenGL, Image, Font, Audio; a spinning-triangle and a sprite demo run |
| M4: Data and storage | Crypto, Compress, Db, DateTime, Uuid, Config, Regex, Xml verified |
| M5: Vulkan | triangle rendered through `Vulkan.os` helpers |
| M6: Polish | Canvas, Gui, HttpServer, Ws, Tls, Yaml, Toml, Video; Iter/Async/Reflect as compiler allows |

---

## 10. Language-feature requests (in priority order)

1. Generic free functions and explicit instantiation (unblocks Algo).
2. First-class function types everywhere (fields, globals, parameters, casts).
3. `defer` or destructors (removes most manual `free()` calls and leaks).
4. `T?` and `Result<T,E>` (replaces bool + out-pointer APIs).
5. Traits or constraints (typed hashing/comparison for `Map`, `Set`, `sort`).
6. User variadics (`String.format`, `Console.printf`).
7. Iterator trait + closures (Iter, `for (x in c)`).
8. Compiler type metadata (Reflect, serialization).
9. `async/await` (Async).

---

## 11. License

OmniScript is free for anyone to use. Add the chosen license (MIT or Apache 2.0) as `LICENSE` at the repo root and mention it in the standard library README.
