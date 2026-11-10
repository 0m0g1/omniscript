<!-- # OmniScript Standard Library: Module Plan -->

A list of the modules OmniScript needs so that users are not blocked building anything: command-line tools, servers, games, desktop apps, data processing, scripts and systems code. Every API below is a **target**, not something that exists yet. Names are proposals.

This plan lists what each module contains, not which module imports which. The one rule is that a module may only import modules in the same or an earlier tier, and never forms a cycle.

**Escape hatches (the "never blocked" guarantee).** If the standard library lacks something, a user can always:
1. import the raw C module in `std/c/` and call the C API directly,
2. load any C library with `extern "lib.dll" { ... }` or a header import,
3. drop into raw pointers with `std/mem`.

Everything below is built so that the escape hatches stay available.

---

## Tier 0: raw C bindings (`std/c/`)

1:1 with C, no safety, no logic. Public and documented. Each of these is just declarations and constants.

| Module | Binds |
|---|---|
| `std/c/mem` | malloc, free, realloc, memcpy, memmove, memset, memcmp |
| `std/c/string` | strlen, strcmp, strncmp, strcpy, strstr, strchr |
| `std/c/file` | fopen, fread, fwrite, fgets, fseek/ftell (64-bit), remove, rename |
| `std/c/console` | printf, fprintf, snprintf, fflush, stderr access |
| `std/c/math` | sin, cos, tan, sqrt, pow, floor, ceil, fabs, exp, log |
| `std/c/time` | time, clock, gettimeofday / QueryPerformanceCounter, Sleep / nanosleep |
| `std/c/os` | getenv, setenv, exit, system, getcwd, chdir, mkdir (platform split inside) |
| `std/c/thread` | pthread / Win32 thread, mutex, condition variable, atomics |
| `std/c/net` | socket, bind, listen, accept, connect, send, recv, close (WinSock / POSIX) |
| `std/c/dl` | dlopen, dlsym, LoadLibrary, GetProcAddress |
| `std/c/glfw` | window, input, Vulkan surface |
| `std/c/vulkan` | generated from the Vulkan header |
| `std/c/opengl` | OpenGL 3.3 core functions and constants (via loader) |
| `std/c/openal` | audio device, buffers, sources |
| `std/c/curl` | libcurl easy interface |
| `std/c/sqlite` | sqlite3 open, prepare, step, finalize |
| `std/c/ssl` | OpenSSL: hashes, ciphers, TLS |
| `std/c/zlib` | deflate, inflate, crc32 |
| `std/c/ffmpeg` | avformat, avcodec, avutil, swscale |

---

## Tier 1: core (everything else builds on these)

| Module | Key API |
|---|---|
| `std/core` | `panic(msg)`, `assert(cond, msg)`, `unreachable()`, `swap(a, b)`, `min`, `max`, `clamp`. Uses `T?` for optional values; a `Result<T, E>` type with `ok(v)`, `err(e)`, `isOk()`, `unwrap()`, `unwrapOr(d)` **[needs generics + traits]** |
| `std/mem` | `malloc`, `free`, `realloc`, `memcpy`, `memset` (re-exported); `Buffer` (`alloc`, `free`, `resize`, `slice`); `Arena` (bump allocator: `alloc(n)`, `reset()`, `free()`) |
| `std/string` | `String` (owned, UTF-8): `from(char*)`, `length()`, `concat`, `substring`, `indexOf`, `contains`, `startsWith`, `endsWith`, `split(sep)`, `trim`, `toUpper`, `toLower`, `replace`, `parseInt`, `parseFloat`, `format(fmt, ...)`. `StringBuilder`: `append`, `appendChar`, `toString` |
| `std/math` | `abs`, `sqrt`, `pow`, `sin`, `cos`, `tan`, `atan2`, `floor`, `ceil`, `round`, `lerp`, `PI`, `E`; `Vec2`, `Vec3`, `Vec4`, `Mat4` with `add`, `dot`, `cross`, `normalize`, `mul`, `perspective`, `lookAt` |
| `std/random` | `Random` with `seed(n)`, `nextInt()`, `range(a, b)`, `nextFloat()`, `shuffle(list)`, `pick(list)` |
| `std/time` | `now()`, `Stopwatch` (`start`, `elapsedMs`), `sleep(ms)`, `Duration` |

---

## Tier 2: collections and iteration

All depend on generics **[needs generics]**.

| Module | Key API |
|---|---|
| `std/collections` | `List<T>`: `push`, `pop`, `get(i)`, `set(i, v)`, `length`, `insert`, `removeAt`, `clear`, `sort`, `contains`, `indexOf`. `Map<K, V>`: `put`, `get` (returns `V?`), `has`, `remove`, `keys()`, `values()`, `length`. `Set<T>`: `add`, `has`, `remove`, `union`, `intersect`. `Queue<T>`: `enqueue`, `dequeue`, `peek`. `Stack<T>`: `push`, `pop`, `peek`. `Deque<T>`, `PriorityQueue<T>` |
| `std/iter` | `for (x in collection)` support; `map`, `filter`, `reduce`, `zip`, `enumerate`, `range(a, b)`, `take`, `skip`, `any`, `all`, `collect()` **[needs iterator trait]** |
| `std/algo` | `sort(list)`, `sortBy(list, cmp)`, `binarySearch`, `reverse`, `unique`, `min`, `max` on collections |

---

## Tier 3: text and data formats

| Module | Key API |
|---|---|
| `std/text` | `utf8` helpers (`length`, `codepoints`, `validate`), `base64Encode/Decode`, `hexEncode/Decode`, `urlEncode/Decode` |
| `std/regex` | `Regex.compile(pattern)` (returns `Regex?`), `matches`, `find`, `findAll`, `replace`, `captures` |
| `std/json` | `Json.parse(String)` (returns `Json?`), `stringify`, `get(key)`, `at(i)`, `asString`, `asInt`, `asBool`, plus builders: `object()`, `array()`, `set(key, v)` |
| `std/csv` | `Csv.parse(String)`, `rows()`, `header()`, `write(file)` |
| `std/toml` / `std/yaml` | `parse`, `stringify`, `get` |
| `std/xml` | minimal `parse`, `find(tag)`, `attr(name)` |
| `std/binary` | `ByteReader` and `ByteWriter`: `readU8/U16/U32/U64`, `writeU32`, endianness control |

---

## Tier 4: input/output, filesystem, process

| Module | Key API |
|---|---|
| `std/console` | `console.log`, `println`, `print`, `info`, `warn`, `error`, `printf`, `readLine()`, `clear()` |
| `std/io` | `File.open(path, Mode)`, `close`, `size`, `readLine`, `readAll`, `readBytes(n)`, `write`, `writeLine`, `seek`, `flush`, `raw()`. `stdin`, `stdout`, `stderr` streams. `readFile(path)` and `writeFile(path, text)` helpers for one-liners |
| `std/path` | `join`, `basename`, `dirname`, `extension`, `normalize`, `isAbsolute`, `exists`, `isFile`, `isDirectory` |
| `std/fs` | `listDir(path)`, `createDir`, `removeFile`, `removeDir`, `rename`, `copy`, `stat(path)` (size, modified time), `walk(path)`, `tempFile()` |
| `std/sys` | `args()`, `env(name)`, `setEnv`, `exit(code)`, `cwd()`, `chdir`, `platform()`, `arch()`, `cpuCount()` |
| `std/process` | `run(cmd, args)` returns exit code and captured output; `spawn`, `wait`, `kill`, pipes |
| `std/log` | `Logger` with levels (`debug`, `info`, `warn`, `error`), timestamps, optional file output |
| `std/cli` | argument parsing: `Cli.new(name)`, `flag("verbose")`, `option("output")`, `positional("input")`, `parse()`, `helpText()` |

---

## Tier 5: concurrency and networking

| Module | Key API |
|---|---|
| `std/thread` | `Thread.spawn(fn)`, `join`, `sleep`, `yield`, `Mutex` (`lock`, `unlock`), `RwLock`, `Condition`, `Atomic<T>` (`load`, `store`, `add`, `compareExchange`), `ThreadPool` (`submit(fn)`) |
| `std/channel` | `Channel<T>`: `send`, `receive`, `tryReceive`, `close` |
| `std/async` | `async fn`, `await`, `Future<T>`, `spawn`, `sleep`, timers **[needs compiler support]** |
| `std/net` | `TcpListener.bind(addr)`, `accept`, `TcpStream.connect(addr)`, `read`, `write`, `close`; `UdpSocket`; `resolve(hostname)`; `Address` parsing |
| `std/http` | client: `Http.get(url)`, `post(url, body)`, headers, status, body (backed by libcurl). Server: `HttpServer.listen(port)`, `route(method, path, handler)`, `Request` / `Response` types |
| `std/ws` | `WebSocket.connect(url)`, `send`, `receive`, server side `accept` |
| `std/tls` | `TlsStream` wrapping a `TcpStream` (via OpenSSL) |

---

## Tier 6: security, storage, compression

| Module | Key API |
|---|---|
| `std/crypto` | `sha256`, `sha1`, `md5`, `hmac`, `randomBytes(n)`, `aesEncrypt/Decrypt`, `bcrypt` or `argon2` password hashing, `constantTimeEquals` |
| `std/compress` | `gzip`, `gunzip`, `deflate`, `inflate`, `zip` read and write |
| `std/db` | `Database.open(path)` (SQLite): `execute(sql)`, `query(sql, params)` returns rows, `prepare`, `transaction`, `close` |
| `std/uuid` | `uuid4()`, `parse`, `toString` |
| `std/datetime` | `DateTime.now()`, `parse(text)`, `format(fmt)`, `addDays`, `diff`, time zones, `unixSeconds()` |
| `std/config` | load settings from file and environment, with defaults |

---

## Tier 7: graphics, windowing, audio, media

| Module | Key API |
|---|---|
| `std/window` | `Window.create(title, w, h)` (no API), `createOpenGL`, `shouldClose`, `poll`, `swapBuffers`, `framebufferSize`, `setTitle`, `close`, public `handle` |
| `std/input` | `Keyboard.isDown(key)`, `Mouse.position()`, `Mouse.isDown(button)`, `Gamepad`, event callbacks, text input |
| `std/opengl` | GL 3.3 functions plus named constants; helpers: `compileShader(src)`, `linkProgram(vs, fs)`, `Texture.load`, `Buffer`/`VertexArray` types |
| `std/vulkan` | generated bindings; helpers: `createInstance(extensions)`, `pickDevice(surface)`, `createSwapchain(...)`. The raw API stays available |
| `std/canvas` | simple 2D drawing for beginners: `Canvas.create(w, h)`, `clear(color)`, `drawRect`, `drawCircle`, `drawLine`, `drawText`, `drawImage`, `present()` |
| `std/image` | `Image.load(path)` (PNG, JPG), `save`, `width`, `height`, `pixels`, `resize`, `crop` |
| `std/font` | `Font.load(path, size)`, `measure(text)`, `render(text)` |
| `std/audio` | `Audio.init()`, `Sound.load(path)`, `play`, `stop`, `setVolume`, `loop`, 3D position |
| `std/video` | `Video.open(path)`, `nextFrame()`, `seek`, `duration` (via FFmpeg) |
| `std/gui` | widgets on top of `canvas`: `Button`, `Label`, `TextBox`, `Layout`, event handlers |

---

## Tier 8: language tooling and interop

| Module | Key API |
|---|---|
| `std/ffi` | `loadLibrary(path)`, `getSymbol(lib, name)`, `callbacks` helpers, C string helpers: `toCString(String)`, `fromCString(char*)`; documented header-import workflow |
| `std/reflect` | `typeName(x)`, `sizeOf<T>()`, `fields(T)` for serialization **[needs compiler support]** |
| `std/test` | `test("name", fn)`, `assertEq(a, b)`, `assertTrue`, `assertNull`, test runner with pass/fail summary |
| `std/bench` | `bench("name", fn)`: runs repeatedly and prints mean and spread |
| `std/debug` | `dbg(value)`, `backtrace()`, `breakpoint()`, memory leak report |

---

## The prelude (`std/std`)

What `include "std/std.os"` brings in, so beginners can write scripts without a list of imports at the top:

`core`, `console`, `string`, `math`, `collections`, `io`, `path`, `sys`, `time`, `random`.

Everything else is imported by name.

---

## Can people build X? (the usability check)

For each kind of project, the modules that should be enough with no escape hatch needed. If a row has a gap, that's a module to prioritize.

| Project | Modules |
|---|---|
| Command-line tool | `console`, `cli`, `io`, `fs`, `path`, `string`, `collections`, `json` |
| Script / automation | `process`, `fs`, `sys`, `regex`, `http`, `log` |
| Web server / API | `http`, `json`, `db`, `thread`, `async`, `crypto`, `config`, `log` |
| Web client / scraper | `http`, `json`, `regex`, `xml`, `string` |
| 2D game | `window`, `canvas` (or `opengl`), `input`, `audio`, `image`, `font`, `math`, `random`, `time` |
| 3D game / engine | `window`, `vulkan` or `opengl`, `input`, `audio`, `math`, `image`, `thread`, `collections` |
| Desktop app | `window`, `gui`, `canvas`, `io`, `json`, `datetime` |
| Data processing | `collections`, `iter`, `algo`, `csv`, `json`, `string`, `thread`, `db`, `compress` |
| Systems / embedded | `mem` (with `Arena`), `c/*`, `thread`, `binary`, `ffi`, `net` |
| Networking tool | `net`, `tls`, `thread`, `binary`, `crypto` |
| Library wrapper (C FFI) | `ffi`, `c/*`, header import |
| Compiler / interpreter | `string`, `collections`, `io`, `fs`, `regex`, `binary` |

---

## Build order

The order that unblocks the most users soonest:

1. **Foundation:** `std/c/*` (mem, file, console, string, math), `core`, `mem`, `console`, `io`, `string`, `math`.
2. **Everyday scripting:** `collections` (List and Map first), `path`, `fs`, `sys`, `time`, `random`, `process`, `json`, `cli`, `test`. After this a person can write real command-line tools.
3. **Window and graphics:** `window`, `input`, `opengl`, `image`, `canvas`, then `vulkan` helpers, `audio`, `font`. After this a person can write games.
4. **Network:** `net`, `http` client, `thread`, `channel`, then `http` server, `tls`, `ws`, `async`. After this a person can write servers.
5. **Data and storage:** `db`, `csv`, `regex`, `compress`, `crypto`, `datetime`, `uuid`.
6. **Polish:** `gui`, `video`, `reflect`, `bench`, `debug`, `log`, `config`, `yaml`, `toml`, `xml`.

**Language features these depend on:** generics for collections and `Result<T, E>`; an iterator trait for `for...in`; destructors or `defer` for closing files and sockets; user-defined variadics for `console.printf` and `String.format`; `async` / `await` for `std/async`. Check each against the compiler before scheduling the modules that need it.

---

## A sample: one program touching several modules

This is the target feel of the API. It counts words in a file given on the command line and writes the top ten as JSON. None of this runs yet.

```os
include "std/std.os";
include "std/json.os";
include "std/cli.os";

let cli = Cli.new("wordtop");
cli.positional("input");
cli.option("output");
let args = cli.parse();
if (args == null) { console.error(cli.helpText()); return 1; }

let text = readFile(args.get("input"));
if (text == null) {
    console.error("could not read input file");
    return 1;
}

let counts = Map<String, i32>{};
for (word in text.toLower().split(" ")) {
    let w = word.trim();
    if (w.length() == 0) continue;
    counts.put(w, (counts.get(w) ?? 0) + 1);
}

let sorted = counts.entries().sortBy((a, b) => b.value - a.value);
let out = Json.array();
for (entry in sorted.take(10)) {
    let item = Json.object();
    item.set("word", entry.key);
    item.set("count", entry.value);
    out.push(item);
}

writeFile(args.get("output") ?? "top.json", out.stringify());
console.info("done");
```

It assumes generics, `for...in`, lambdas, the `??` operator and nullable narrowing. Those are the language features the standard library will pressure the compiler to support.

---

## License

OmniScript is free for anyone to use. Put the chosen license (MIT or Apache 2.0 are the usual choices for this) in a `LICENSE` file at the repo root, and mention it in the standard library's README, so users know what they may do with the code.
