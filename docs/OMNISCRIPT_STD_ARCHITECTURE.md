# OmniScript Standard Library: Design and Context

OmniScript is one language. The standard library is **not** presented as "high level" and "low level". Users pick a module by the job they want done, and the modules get closer to the machine as they go:

| Module | Job | Feels like |
|---|---|---|
| `std/console` | printing and logging | JavaScript's `console` |
| `std/io` | files and streams | Godot's `FileAccess` |
| `std/mem` | raw memory | C's `malloc` / `free` |
| `std/sys` | process, environment | C's `exit` / `getenv` |
| `std/window` | windows and events | GLFW without the setup |

A beginner never needs to go past `std/console`. A systems programmer reaches for `std/mem`, or for the raw C modules in `std/c/` (section 4). Nothing in the tutorial uses the words "high level" or "low level".

OmniScript is free for anyone to use however they like, so nothing in the library is hidden: every friendly module is built on a public module underneath it, and users can drop down, mix the two, or write their own wrapper.

**Legend**
- **[works today]**: syntax seen in the working example scripts, including the Vulkan and OpenGL showcases (see section 5.1).
- **[needs compiler support]**: assumed by this design but not yet verified in the compiler.

---

## 1. Tutorial (what users see)

### 1.1 Hello, world

```os
include "std/console.os";

console.log("Hello, OmniScript");
```

`console` has a few ways to print:

```os
include "std/console.os";

console.log("with a newline");
console.println("same thing, different name");
console.print("no newline");
console.printf("%d + %d = %d\n", 2, 3, 2 + 3);   // see the note in 3.1
console.info("green tag");
console.warn("yellow tag");
console.error("red tag, goes to stderr");
```

### 1.2 Reading a file

```os
include "std/io.os";
include "std/console.os";

let f = File.open("notes.txt", Mode.Read);
if (f == null) {
    console.error("could not open notes.txt");
    return 1;
}

let text = f.readAll();
if (text != null) {
    console.log(text.data);
    text.free();
}
f.close();
```

`File.open` returns `File?`, so the compiler makes you handle a missing file. `close()` and `free()` are explicit until the language has destructors or `defer` (see section 6).

### 1.3 Going closer to the machine

When you need control, drop down to `std/mem`. It is the same language and the same style, with fewer guard rails.

```os
include "std/mem.os";
include "std/console.os";

let n: i32 = 1000;
let buf = malloc((n * sizeof(i64)) as u64) as i64*;
if (buf == nullptr) { return 1; }

for (let i: i32 = 0; i < n; i++) {
    buf[i] = (i as i64) * (i as i64);
}

console.printf("last = %lld\n", buf[n - 1]);
free(buf as void*);
```

### 1.4 A window

```os
include "std/window.os";
include "std/console.os";

let win = Window.create("My App", 1280, 720);
if (win == null) {
    console.error("failed to create window");
    return 1;
}

while (!win.shouldClose()) {
    win.poll();
}
win.close();
```

### 1.5 When you want the real C API

Every friendly module sits on a public raw module, and the wrappers expose their handles. You can mix the two freely.

```os
include "std/io.os";
include "std/c/file.os";     // the raw fopen/fread/fseek bindings

let f = File.open("data.bin", Mode.ReadBinary);
if (f == null) { return 1; }

// Use the wrapper for open and close, and the raw API for something the wrapper doesn't offer.
_fseeki64(f.raw(), 128, SEEK_SET);

f.close();
```

`std/c/*` makes no promises beyond the C API itself: no null checks, no ownership, no safety net. You are responsible for that.

---

## 2. Module reference

| Module | Provides |
|---|---|
| `std/console` | `console.log`, `println`, `print`, `printf`, `info`, `warn`, `error`; `printf` is also available directly |
| `std/io` | `File`, `Mode`; also includes `std/mem` so `Buffer` is available |
| `std/mem` | `malloc`, `free`, `realloc`, `memcpy`, `memset`; `Buffer` (an owned byte buffer) |
| `std/sys` | `exit`, `getenv`, `getArgs` |
| `std/path` | `join`, `mkdir`, `remove`, `rename` |
| `std/window` | `Window` (GLFW wrapper): a plain window for Vulkan, or an OpenGL 3.3 core window |
| `std/vulkan` | Vulkan structs, functions and macros, generated from the C header |
| `std/opengl` | OpenGL 3.3 core functions and constants |
| `std/c/*` | raw, 1:1 C bindings (`std/c/file`, `std/c/mem`, `std/c/glfw`, ...), for people who want the real C API |
| `std/std` | convenience: includes `console`, `io`, `mem`, `sys` |

---

## 3. Module sources

### 3.1 `std/console.os`

`printf` is declared as a C varargs function and is re-exported as is. A user-written variadic `console.printf` method that forwards to it needs `vprintf` and `va_list` support **[needs compiler support]**. Until then, tell users to call `printf(...)` directly, and keep `console.printf` out of the docs if it can't forward.

```os
include "std/c/console.os";   // printf, fprintf, fflush, stderr access

const COLOR_RED: char*    = "\x1b[31m";
const COLOR_GREEN: char*  = "\x1b[32m";
const COLOR_YELLOW: char* = "\x1b[33m";
const COLOR_RESET: char*  = "\x1b[0m";

struct Console {
    log(msg: char*) => void {
        printf("%s\n", msg);
        fflush(nullptr);
    }

    println(msg: char*) => void {
        printf("%s\n", msg);
        fflush(nullptr);
    }

    print(msg: char*) => void {
        printf("%s", msg);
        fflush(nullptr);
    }

    info(msg: char*) => void {
        printf("%s[INFO]%s %s\n", COLOR_GREEN, COLOR_RESET, msg);
    }

    warn(msg: char*) => void {
        printf("%s[WARN]%s %s\n", COLOR_YELLOW, COLOR_RESET, msg);
    }

    // Errors go to stderr, not stdout.
    error(msg: char*) => void {
        let err = stderrHandle();
        fprintf(err, "%s[ERROR]%s %s\n", COLOR_RED, COLOR_RESET, msg);
        fflush(err);
    }
}

let console = Console{};
```

### 3.2 `std/mem.os`

```os
include "std/c/mem.os";   // malloc, free, realloc, memcpy, memset become available

// An owned byte buffer: replaces "return a raw malloc pointer".
struct Buffer {
    data: char* = nullptr;
    size: u64 = 0;

    // Allocates n bytes plus a zero terminator, so the contents are also a valid C string.
    static alloc(n: u64) => Buffer? {
        let p = malloc(n + 1) as char*;
        if (p == nullptr) return null;
        memset(p as void*, 0, n + 1);
        return Buffer{ data: p, size: n };
    }

    free() => void {
        if (this.data != nullptr) {
            free(this.data as void*);
            this.data = nullptr;
            this.size = 0;
        }
    }
}
```

### 3.3 `std/io.os`

```os
include "std/c/file.os";
include "std/mem.os";

enum Mode { Read, Write, Append, ReadBinary, WriteBinary, AppendBinary }

fn modeString(m: Mode) => char* {
    if (m == Mode.Read)         return "r";
    if (m == Mode.Write)        return "w";
    if (m == Mode.Append)       return "a";
    if (m == Mode.ReadBinary)   return "rb";
    if (m == Mode.WriteBinary)  return "wb";
    return "ab";
}

struct File {
    _handle: void* = nullptr;

    // Returns null if the file cannot be opened.
    static open(path: char*, mode: Mode) => File? {
        let h = fopen(path, modeString(mode));
        if (h == nullptr) return null;
        return File{ _handle: h };
    }

    // Must be called when done. Safe to call twice.
    close() => void {
        if (this._handle != nullptr) {
            fclose(this._handle);
            this._handle = nullptr;
        }
    }

    isOpen() => bool {
        return this._handle != nullptr;
    }

    // Escape hatch: the underlying C FILE*, for mixing with direct calls from std/c/file.
    raw() => void* {
        return this._handle;
    }

    size() => u64 {
        if (this._handle == nullptr) return 0;
        let cur = _ftelli64(this._handle);
        _fseeki64(this._handle, 0, SEEK_END);
        let total = _ftelli64(this._handle);
        _fseeki64(this._handle, cur, SEEK_SET);
        return total as u64;
    }

    // Reads one line into the caller's buffer. Returns false at end of file or on error.
    readLine(buf: char*, capacity: int) => bool {
        if (this._handle == nullptr) return false;
        return fgets(buf, capacity, this._handle) != nullptr;
    }

    // Reads the whole file into an owned, zero-terminated Buffer. The caller frees it.
    readAll() => Buffer? {
        if (this._handle == nullptr) return null;
        let total = this.size();
        let buf = Buffer.alloc(total);
        if (buf == null) return null;
        fread(buf.data as void*, 1, total, this._handle);
        return buf;
    }

    write(data: char*, length: u64) => u64 {
        if (this._handle == nullptr) return 0;
        return fwrite(data as void*, 1, length, this._handle);
    }
}
```

### 3.4 `std/window.os`

One module serves both showcases. `Window.create` makes a window with no client API (what Vulkan needs). `Window.createOpenGL` makes an OpenGL 3.3 core window with the context current.

```os
include "std/c/window.os";

enum Api { None, OpenGL }

// Shared setup. Returns null on failure.
fn _openWindow(title: char*, w: int, h: int, api: Api) => void* {
    if (glfwInit() == 0) return nullptr;

    if (api == Api.OpenGL) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    } else {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);   // Vulkan: no GL context
    }

    let handle = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (handle == nullptr) {
        glfwTerminate();
        return nullptr;
    }
    if (api == Api.OpenGL) glfwMakeContextCurrent(handle);
    return handle;
}

struct Window {
    handle: void* = nullptr;     // public: Vulkan needs it for surface creation
    width: int = 0;
    height: int = 0;

    static create(title: char*, w: int, h: int) => Window? {
        let h2 = _openWindow(title, w, h, Api.None);
        if (h2 == nullptr) return null;
        return Window{ handle: h2, width: w, height: h };
    }

    static createOpenGL(title: char*, w: int, h: int) => Window? {
        let h2 = _openWindow(title, w, h, Api.OpenGL);
        if (h2 == nullptr) return null;
        return Window{ handle: h2, width: w, height: h };
    }

    shouldClose() => bool {
        return glfwWindowShouldClose(this.handle) != 0;
    }

    poll() => void {
        glfwPollEvents();
    }

    swapBuffers() => void {          // OpenGL only
        glfwSwapBuffers(this.handle);
    }

    framebufferSize(w: int*, h: int*) => void {
        glfwGetFramebufferSize(this.handle, w, h);
    }

    close() => void {
        if (this.handle != nullptr) {
            glfwDestroyWindow(this.handle);
            this.handle = nullptr;
            glfwTerminate();
        }
    }
}
```

---

## 4. Raw C modules (`std/c/`, public)

`std/c/` holds the raw C bindings. It is public and documented: a user who wants raw `fopen`, or the real GLFW API, or who wants to write their own `File` type, imports it directly. The friendly modules are built on it.

```text
std/
├── console.os   io.os   mem.os   sys.os   path.os   window.os   std.os
└── c/                       # public raw C bindings
    ├── console.os           # printf, fprintf, fflush, stderrHandle
    ├── file.os              # fopen, fclose, fread, fwrite, fgets, 64-bit seek/tell, SEEK_*
    ├── mem.os               # malloc, free, realloc, memcpy, memset
    └── window.os            # glfw* functions and GLFW_* constants
```

Rules:
1. `std/c/` contains only `extern "C"` blocks, constants, and plain structs. No `if`, no loops, no methods, no allocation, no null checks.
2. `std/c/` never imports a friendly module. Dependencies run one way: friendly modules import the modules underneath them, never the reverse. No cycles. If two modules seem to need each other, move the shared part into a third module below both.
3. Declare each C function once, in `std/c/`. Friendly modules import it, so two declarations cannot disagree.
4. Every wrapper exposes its raw handle (`File.raw()`, `Window.handle`), so users can mix wrapper and raw calls.
5. Document `std/c/*` as "1:1 with C, no safety". If other people build on it, it only changes when the C API does.
6. Tutorials start with the friendly modules. `std/c/*` appears later, under "when you want the real C API".

### 4.1 `std/c/file.os`

Windows CRT names are shown for 64-bit offsets. On POSIX use `fseeko` and `ftello` with an `i64` offset.

```os
extern "C" {
    fn fopen(path: char*, mode: char*) => void*;
    fn fclose(f: void*) => int;
    fn fread(buf: void*, size: u64, count: u64, f: void*) => u64;
    fn fwrite(buf: void*, size: u64, count: u64, f: void*) => u64;
    fn fgets(buf: char*, size: int, f: void*) => char*;
    fn feof(f: void*) => int;
    fn _fseeki64(f: void*, offset: i64, origin: int) => int;
    fn _ftelli64(f: void*) => i64;
}

const SEEK_SET: int = 0;
const SEEK_CUR: int = 1;
const SEEK_END: int = 2;
```

### 4.2 `std/c/console.os`

Getting stderr differs by platform. On Windows (UCRT) it is `__acrt_iob_func(2)`; on POSIX use `fdopen(2, "w")`. That detail stays here.

```os
extern "C" {
    fn printf(fmt: char*, ...) => int;
    fn fprintf(stream: void*, fmt: char*, ...) => int;
    fn fflush(stream: void*) => int;
    fn __acrt_iob_func(index: u32) => void*;   // Windows UCRT
}

fn stderrHandle() => void* {
    return __acrt_iob_func(2);
}
```

### 4.3 `std/c/mem.os`

```os
extern "C" {
    fn malloc(size: u64) => void*;
    fn free(ptr: void*) => void;
    fn realloc(ptr: void*, new_size: u64) => void*;
    fn memcpy(dest: void*, src: void*, size: u64) => void*;
    fn memset(dest: void*, value: int, size: u64) => void*;
}
```

### 4.4 `std/c/window.os`

Uses the library-path form of `extern`, as the showcases do. The path below is machine-specific, so keep it in this one file (or in the resolver registered as `"glfw"` in `setupExternalResolvers`, if that can be referenced by name) and never repeat it in a public module.

```os
extern "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dll" {
    fn glfwInit() => int;
    fn glfwTerminate() => void;
    fn glfwWindowHint(hint: int, value: int) => void;
    fn glfwCreateWindow(w: int, h: int, title: char*, monitor: void*, share: void*) => void*;
    fn glfwDestroyWindow(window: void*) => void;
    fn glfwWindowShouldClose(window: void*) => int;
    fn glfwPollEvents() => void;
    fn glfwMakeContextCurrent(window: void*) => void;
    fn glfwSwapBuffers(window: void*) => void;
    fn glfwSwapInterval(interval: int) => void;
    fn glfwGetFramebufferSize(window: void*, width: int*, height: int*) => void;
    fn glfwGetProcAddress(name: char*) => void*;
    fn glfwVulkanSupported() => int;
    fn glfwGetRequiredInstanceExtensions(count: uint*) => char**;
    fn glfwCreateWindowSurface(inst: void*, win: void*, alloc: void*, surf: void**) => int;
}

const GLFW_NO_API: int = 0;
const GLFW_CLIENT_API: int = 0x00022001;
const GLFW_CONTEXT_VERSION_MAJOR: int = 0x00022002;
const GLFW_CONTEXT_VERSION_MINOR: int = 0x00022003;
const GLFW_OPENGL_PROFILE: int = 0x00022008;
const GLFW_OPENGL_API: int = 0x00030001;
const GLFW_OPENGL_CORE_PROFILE: int = 0x00032001;
```

`malloc` is used in the Vulkan showcase without any visible declaration, so the built-in C resolver seems to provide it. Check that redeclaring `malloc` in `std/c/mem.os` does not clash with that.

---

## 5. Graphics

### 5.1 What the showcases confirm works today

Both the Vulkan and OpenGL showcases run, so these are **[works today]**:

- `fn main() => i32` as an entry point (top-level script code works too).
- `let app: App;` zero-initializes a struct, including its default field values.
- Struct literals, nested ones included: `VkExtent2D{ width: 800, height: 600 }`.
- `&this.field` as an out-parameter.
- Prefix arrays (`[4]float`, `[9]float`) with array literals.
- `sizeof(vertices)` and `sizeof(void*)`.
- Raw strings (`r"..."`) for shader sources.
- Both extern forms: `extern "C" fn printf(...)` (single declaration) and `extern "path-or-header" { ... }` (block).
- Header import: `extern "standard/1/vulkan_min.h", "vulkan-1.dll" { }` generates structs, functions and macros.

### 5.2 Design

- One window owns exactly one graphics API. Vulkan and OpenGL cannot share a swapchain, so a window is created for one or the other (`Window.create` vs `Window.createOpenGL`).
- Public modules: `std/window` (section 3.4), `std/vulkan` (the generated header import), `std/opengl` (GL 3.3 core declarations and constants such as `GL_VERTEX_SHADER`). Replace the magic numbers in the showcases (`0x8B31`, `0x8892`) with named constants in `std/opengl`.
- A shared `Renderer` abstraction comes last, extracted from two working backends. It needs trait or interface dispatch **[needs compiler support]**.
- Never store `&local` in a struct that outlives the local, and do not give `void*` methods. Draw calls go through typed structs.

### 5.3 Showcase using `std/window`

OpenGL, shortened:

```os
include "std/window.os";
include "std/opengl.os";
include "std/console.os";

let win = Window.createOpenGL("OpenGL Showcase", 800, 600);
if (win == null) { console.error("could not create window"); return 1; }

// ... create shaders, VAO, VBO as in the showcase ...

while (!win.shouldClose()) {
    win.poll();
    glViewport(0, 0, 800, 600);
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    renderer.draw();
    win.swapBuffers();
}
win.close();
```

Vulkan, the window and surface part:

```os
include "std/window.os";
include "std/vulkan.os";

let win = Window.create("Vulkan Showcase", 800, 600);
if (win == null) return 1;

// ... create instance, then:
glfwCreateWindowSurface(instance, win.handle, nullptr, &surface);
```

### 5.4 Fixes before these become reference examples

**Vulkan showcase.** These are required by the Vulkan spec. If it runs on your machine anyway, the driver may be lenient, or an early return may be hiding a failure. Run it with the validation layers on.

| Issue | Fix |
|---|---|
| No `GLFW_NO_API` hint before the window is created, so GLFW makes an OpenGL context | Use `Window.create`, which sets it |
| Instance created without the surface extensions | Pass the result of `glfwGetRequiredInstanceExtensions` as `ppEnabledExtensionNames` |
| Device created without `VK_KHR_swapchain` | Enable it in `VkDeviceCreateInfo` |
| `vkAcquireNextImageKHR` passes null for both semaphore and fence | Pass at least one; the spec disallows both being null |
| `queueFamilyIndex` hardcoded to 0, and the first physical device is taken after a call with `count = 1` | Call `vkEnumeratePhysicalDevices` twice (count, then list) and pick a family with `vkGetPhysicalDeviceSurfaceSupportKHR` |
| No render pass or framebuffer, so nothing is drawn | The "Clear color set to purple" line is only a `printf`; build the render pass and framebuffers, then record `vkCmdBeginRenderPass` |
| Swapchain extent and image count hardcoded | Query `vkGetPhysicalDeviceSurfaceCapabilitiesKHR` |
| Command pool and `images` buffer never freed, no `vkDeviceWaitIdle` before teardown | Destroy in reverse order of creation, after waiting for the device |

**OpenGL showcase.**

| Issue | Fix |
|---|---|
| `opengl32.dll` on Windows exports only OpenGL 1.1 directly. Functions like `glGenVertexArrays` and `glCreateShader` come from `wglGetProcAddress`, or `glfwGetProcAddress` | The showcase works, so your resolver is probably doing this. Make it explicit in `std/opengl`, or use the glad loader you already ship |
| No context version or profile hints | `Window.createOpenGL` sets 3.3 core |
| Shader compile and link results are never checked | Call `glGetShaderiv` and `glGetProgramiv`, print the info log on failure |
| `norm: uint` in `glVertexAttribPointer` | GL expects a boolean (`u8`); declare it that way |
| Magic numbers | Named constants in `std/opengl` |

---

## 6. Open decisions

1. **Memory model.** Garbage collection, reference counting, or ownership. This decides whether "closed at the end of scope" can ever be true for `File` and `Buffer`.
2. **Destructors or `defer`.** Needed for automatic cleanup. Until then every `open` has a `close` and every `alloc` has a `free`, and the docs must say so.
3. **Where unsafe begins.** Pointer arithmetic and raw `malloc` in `std/mem` should sit behind a visible marker (`unsafe`), so safe and unsafe code look different.
4. **`null` vs `nullptr`.** Keep both, and never let `nullptr` convert silently into `T?`.
5. **Extern pointer returns.** `fopen` is declared `=> void*`, which is not nullable. For null safety to catch an unchecked `fopen`, extern pointer returns must be implicitly nullable, or declared nullable.
6. **Null narrowing.** After `if (f == null) return`, `f` should be usable as `File`. Verify the compiler does this.
7. **Entry point.** Both top-level script code and `fn main() => i32` work today. Choose one as the style used in the docs, and mention the other once.
8. **Numeric casts.** Allow implicit widening only. Require `as` for narrowing.
9. **User-defined variadics.** Needed for a forwarding `console.printf`.

---

## 7. Instructions for the next developer

1. Write `std/c/` first. It contains declarations only.
2. Then `mem`, `io`, `console`, in that order, then `window`, then `vulkan` and `opengl`. Any shared renderer abstraction comes last.
3. Never return the address of a local, and never store `&local` in a struct that outlives it.
4. Every public function that can fail returns a nullable type, and every example checks for null.
5. Tutorials use the friendly modules. If a common example needs `std/c/` just to do something simple, the friendly module is missing a function.
6. Turn each example in section 1 into a test script, and confirm it runs before calling the module done.
