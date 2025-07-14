# Foreign Function Interface (FFI) in OS

The **OS programming language**, features a uniquely powerful and flexible Foreign Function Interface (FFI) that distinguishes it in systems programming. Designed for maximum interoperability, the FFI enables OS to seamlessly integrate with virtually any C-based library that provides static (`.a`) or dynamic (`.dll`, `.so`, `.dylib`, `.wa`, etc.) library files, as long as they are supported by LLVM. With a concise, platform-aware syntax, support for multiple library formats, and the ability to declare functions, constants, and static variables, OS's FFI empowers developers to leverage existing ecosystems for diverse applications, from networking to databases and graphics. The `--target-os` flag ensures the correct library is selected for cross-platform development, while shorthands like `extern "C"` and `extern "kernel32"` simplify access to common system libraries, with full paths as a fallback if shorthands fail. During AOT compilation, linker arguments are automatically generated from library paths, and the backend attempts to link using available linkers (`clang++`, `g++`, or `link` on Windows), falling back to alternatives if one fails, and throws an error if all fail. The backend also generates a symbol table to document globals, functions, and aliases, applies executable permissions on non-Windows platforms, and intelligently manages dynamic library dependencies with automatic copying and cleanup. Currently tested primarily on Windows, OS plans to expand support for other operating systems in the future, despite the constraints of a single-developer project.

## Why OS's FFI Stands Out

The FFI in OS is built for simplicity, flexibility, and power. Here are the key features that make it unique:

1. **Broad Library Format Support**: OS can interface with any C-based library providing static (`.a`) or dynamic (`.dll`, `.so`, `.dylib`, `.wa`, etc.) files, as long as LLVM supports the format. This includes system libraries (e.g., `msvcrt.dll` on Windows, `libc.so` on Linux, `libSystem.dylib` on macOS) and third-party libraries like libcurl, SQLite, or GLAD.

2. **Multiple Library Paths with JIT/AOT Flexibility**: The FFI allows specifying multiple library paths in a single `extern` declaration, ensuring cross-platform compatibility. For example:
   ```os
   extern "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dll", 
          "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/libglfw3.a", 
          "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.so", 
          "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dylib", 
          "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.wa" { ... }
   ```
   During **JIT compilation**, these paths verify symbol existence to prevent runtime errors. During **AOT compilation**, the library does not need to be present at the specified path, as the path is only used for verification during compilation. The dynamic libraries can be in the system path library directory or in the same folder as the generated executable.

3. **Intelligent Dynamic Library Management**: During AOT compilation, OS implements smart dynamic library copying and cleanup:
   - **Selective Copying**: Only copies non-system dynamic libraries that are actually needed for the application to work
   - **System Library Detection**: Automatically identifies OS-specific system libraries that don't need copying:
     - **Windows**: `kernel32.dll`, `user32.dll`, `msvcrt.dll`, `ntdll.dll`, etc.
     - **Unix-like**: Libraries in `/lib/`, `/usr/lib/`, `libc.so`, `libm.so`, `libpthread.so`, etc.
   - **Automatic Cleanup**: Removes copied dynamic libraries when they're not needed:
     - If static resolution succeeds after copying a dynamic library, the copy is deleted
     - If dynamic resolution fails after copying, the copied library is deleted
     - If resolver creation fails after copying, the copied library is deleted
   - **Lean Output Directory**: Ensures the executable's output directory contains only necessary runtime dependencies

4. **Automatic Linker Argument Generation and Fallback**: During AOT compilation, linker arguments are automatically generated from the library paths specified in `extern` declarations. For example, a path like `"dependencies/curl/lib/libcurl.a"` generates linker flags like `-lcurl` for `clang++`/`g++` or `libcurl.lib` for MSVC linkers. The backend supports multiple linkers:
   - **Windows**: `clang++`, `g++`, and `link` (MSVC linker), with default libraries like `user32.lib`, `gdi32.lib`, `shell32.lib`, `kernel32.lib`, and `ntdll.lib`.
   - **Non-Windows**: `clang++` and `g++`, with default libraries like `-lm`, `-ldl`, and `-lpthread`.
   
   The backend attempts to link using the first available linker, falling back to the next if it fails (e.g., from `clang++` to `g++`). If all linkers fail, it throws a `std::runtime_error` listing available linkers and the last error, e.g., "Linking failed. Available linkers: clang++ g++. Last error: Linker clang++ failed with exit code: 1". This simplifies the build process by eliminating manual linker configuration.

5. **Symbol Table Generation**: During AOT compilation, the backend generates a symbol table documenting global variables, functions, and aliases in the LLVM module. The symbol table is written to a specified file, including:
   - **Global Variables**: Names, initialization status (e.g., `(initialized)`), and types.
   - **Functions**: Names, declaration/definition status (e.g., `(declaration)`), types, and argument details (name and type for each argument).
   - **Aliases**: Names and aliasee information (e.g., the name of the aliased symbol).
   If the output file cannot be opened, a `std::runtime_error` is thrown with the error details, e.g., "Failed to open symbol table output file: No such file or directory". This aids debugging and verification of FFI declarations. An example symbol table might look like:
   ```
   # Symbol Table for Module: example_module

   ## Global Variables:
   GLOBAL: global_var1 (initialized) - Type: i32
   GLOBAL: global_var2 - Type: i64

   ## Functions:
   FUNCTION: main - Type: i32 ()*
     Arguments:
       arg1 - i32
       arg2 - i8*
   FUNCTION: helper (declaration) - Type: void (i32)*

   ## Aliases:
   ALIAS: alias1 -> global_var1
   ```

6. **Platform-Specific Permissions**: On non-Windows platforms, the backend adds executable permissions (`owner_exec`, `group_exec`, `others_exec`) to the generated executable using `fs::permissions`, ensuring it can be run without manual intervention. This step is skipped on Windows, as it is not required.

7. **Target OS Selection**: The `--target-os` flag specifies the target operating system, guiding the FFI to select the appropriate library and system library detection. Supported target OSes include:
   - `linux`
   - `windows` (or `win32`)
   - `macos` (or `darwin`)
   - `freebsd`
   - `android`
   - `ios`
   - `wasm` (or `webassembly`)
   - `auto` (detects the host OS)
   If an unknown OS is specified, OS falls back to `auto` and warns the user. For example:
   ```bash
   ./bin/Debug-windows-x86_64/Osengine.exe program.os --target-os linux --make
   ```
   JIT compilation requires the host OS to match the target OS, while AOT supports cross-compilation but may fail for platform-specific symbols (e.g., `CreateThread` on Linux).

8. **System Library Shorthands with Full Path Fallback**: The `extern "C"` syntax is a shorthand for the C standard library's full path (e.g., `system32/msvcrt.dll` on Windows, `libc.so` on Linux, `libSystem.dylib` on macOS). Similarly, `extern "kernel32"` simplifies access to `kernel32.dll` on Windows. Other common system libraries for Linux, macOS, and other OSes are supported and will be documented in the future. If shorthands fail, use full paths, e.g.:
   ```os
   extern "C:/Windows/System32/msvcrt.dll" fn printf(fmt: char*, ...) => int;
   ```

9. **Symbol, Constant, and Static Variable Support**: The FFI supports declaring functions (`fn`), constants (`const`), and static variables (`const` or `let`). For example, with GLAD:
   ```os
   extern "dependencies/glad/gl/bin/glad_gl.dll", 
          "dependencies/glad/gl/bin/libglad_gl.a" {
       fn gladLoadGL(loader: void*) => int;
       const glad_glGetError: fn() => uint;
   }
   ```
   Static variables like `glad_glGetError` are loaded after calling a loader function (e.g., `gladLoadGL`).

10. **Concise and Expressive Syntax**: The `extern "C"` and `extern` with paths syntax is clean and intuitive, minimizing boilerplate.

11. **Standard Library Abstraction**: To simplify usage and reduce errors, OS's upcoming standard library will abstract common symbols. For example, `printf` will be available as:
    ```os
    import { console } from "std";
    console.log("Hello, OS!");
    ```
    or
    ```os
    std.console.log("Hello, OS!");
    ```

12. **Type Safety Caveat**: Incorrect type declarations in the FFI can cause runtime crashes. The standard library will mitigate this by providing type-safe abstractions.

13. **Future Extensibility**: The FFI is designed to potentially support non-C libraries (e.g., Rust, C#, Python), as long as they have dynamic and static library files which expose C-compatible interfaces.

14. **Seamless LLVM Integration**: The FFI integrates with OS's LLVM backend, ensuring optimized calls with `-O3`. The backend handles symbol table emission, executable linking with multiple linker support, platform-specific permissions, and intelligent dynamic library management, making it robust for AOT compilation.

15. **Solo Developer Context**: OS is developed by a single individual, limiting the speed of implementing full support for all target OSes. Currently tested primarily on Windows, the FFI is robust for Windows environments, with plans for expanded cross-platform support in future updates.

These features make OS's FFI a powerful tool for systems programming, even as a work-in-progress by a solo developer.

## Examples: Using libcurl, SQLite, and GLAD with the FFI

The following examples demonstrate integrating with **libcurl** for networking, **SQLite** for database operations, and **GLAD** for OpenGL function loading, showcasing the FFI's versatility, automatic linker argument generation, linker fallback, symbol table generation, intelligent dynamic library management, and platform-specific permissions.

### Example 1: Networking with libcurl
This example uses libcurl to perform an HTTP GET request, targeting Linux and using `extern "C"`. Linker arguments are automatically generated, and the backend attempts multiple linkers if one fails. Dynamic libraries are intelligently managed during AOT compilation.

```os
extern "C" {
    fn printf(fmt: char*, ...) => int; // Shorthand for msvcrt.dll, libc.so, etc.
}

extern "dependencies/curl/lib/libcurl.dll", 
       "dependencies/curl/lib/libcurl.a", 
       "dependencies/curl/lib/libcurl.so", 
       "dependencies/curl/lib/libcurl.dylib" {
    fn curl_global_init(flags: i64) => int;
    fn curl_easy_init() => void*;
    fn curl_easy_setopt(curl: void*, option: int, value: void*) => int;
    fn curl_easy_perform(curl: void*) => int;
    fn curl_easy_cleanup(curl: void*) => void;
    fn curl_global_cleanup() => void;
}

const CURL_GLOBAL_DEFAULT: i64 = 3;
const CURLOPT_URL: int = 10002;
const CURLOPT_WRITEFUNCTION: int = 20011;

// Callback function for writing received data
fn write_callback(data: char*, size: uint, nmemb: uint, userp: void*) => uint {
    printf("%s", data);
    return size * nmemb;
}

function main() => i32 {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
        printf("curl_global_init failed\n");
        return 1;
    }

    let curl = curl_easy_init();
    if (curl == nullptr) {
        printf("curl_easy_init failed\n");
        curl_global_cleanup();
        return 1;
    }

    // Set URL and write callback
    curl_easy_setopt(curl, CURLOPT_URL, "https://example.com");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

    // Perform the request
    let res = curl_easy_perform(curl);
    if (res != 0) {
        printf("curl_easy_perform failed: %d\n", res);
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    printf("\nHTTP request completed\n");
    return 0;
}
```

Run with (on a Linux host):
```bash
./bin/Debug-linux-x86_64/Osengine.exe curl_example.os --target-os linux
```

Or compile to an executable for Linux:
```bash
./bin/Debug-windows-x86_64/Osengine.exe curl_example.os --target-os linux --make -o curl_example
```

**Note**: During AOT compilation, the linker flag `-lcurl` (or `libcurl.lib` for MSVC) is automatically generated from `"dependencies/curl/lib/libcurl.a"`. The backend tries `clang++`, then `g++`, and throws an error if both fail, listing available linkers (e.g., "Linking failed. Available linkers: clang++ g++. Last error: Linker clang++ failed with exit code: 1"). If `libcurl.dll` is needed, it will be copied to the output directory only if it's not a system library and is actually required for the application. A symbol table is generated to document the module's symbols, and executable permissions are added on Linux. JIT compilation with `--target-os linux` on a non-Linux host fails with:
```
ERROR: JIT mode is not supported for cross-compilation
```
As OS is primarily tested on Windows, Linux support may be limited until further development.

### Example 2: Database Operations with SQLite
This example uses SQLite to create and query a table, targeting macOS and using a full path as a fallback for `printf`. Error handling messages are updated to include `err_msg` for consistency. Dynamic library management ensures only necessary libraries are copied.

```os
extern "C:/Windows/System32/msvcrt.dll" { // Full path as fallback if extern "C" fails
    fn printf(fmt: char*, ...) => int;
}

extern "dependencies/sqlite/lib/sqlite3.dll", 
       "dependencies/sqlite/lib/libsqlite3.a", 
       "dependencies/sqlite/lib/sqlite3.so", 
       "dependencies/sqlite/lib/sqlite3.dylib" {
    fn sqlite3_open(filename: char*, db: void**) => int;
    fn sqlite3_exec(db: void*, sql: char*, callback: void*, data: void*, errmsg: char**) => int;
    fn sqlite3_close(db: void*) => int;
}

// Callback function for query results
fn query_callback(data: void*, argc: int, argv: char**, col_names: char**) => int {
    for (let i = 0; i < argc; i++) {
        printf("%s = %s\n", col_names[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

function main() => i32 {
    let db: void*;
    let rc = sqlite3_open(":memory:", &db);
    if (rc != 0) {
        printf("Cannot open database: %d\n", rc);
        return 1;
    }

    // Create table
    let create_sql = "CREATE TABLE users (id INT, name TEXT);";
    let err_msg: char*;
    rc = sqlite3_exec(db, create_sql, nullptr, nullptr, &err_msg);
    if (rc != 0) {
        printf("Create table failed: %s\n", err_msg);
        sqlite3_close(db);
        return 1;
    }

    // Insert data
    let insert_sql = "INSERT INTO users (id, name) VALUES (1, 'Alice'), (2, 'Bob');";
    rc = sqlite3_exec(db, insert_sql, nullptr, nullptr, &err_msg);
    if (rc != 0) {
        printf("Insert failed: %s\n", err_msg);
        sqlite3_close(db);
        return 1;
    }

    // Query data
    let select_sql = "SELECT * FROM users;";
    rc = sqlite3_exec(db, select_sql, query_callback, nullptr, &err_msg);
    if (rc != 0) {
        printf("Query failed: %s\n", err_msg);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_close(db);
    printf("Database operations completed\n");
    return 0;
}
```

Run with (on a macOS host):
```bash
./bin/Debug-macos-x86_64/Osengine.exe sqlite_example.os --target-os macos
```

Or compile to an executable for macOS:
```bash
./bin/Debug-windows-x86_64/Osengine.exe sqlite_example.os --target-os macos --make -o sqlite_example
```

**Note**: During AOT compilation, the linker flag `-lsqlite3` (or `libsqlite3.lib` for MSVC) is automatically generated from `"dependencies/sqlite/lib/libsqlite3.a"`. The backend tries `clang++`, then `g++`, and throws an error if both fail, listing available linkers. If `sqlite3.dll` is needed and is not a system library, it will be copied to the output directory only if required for the application. A symbol table is generated, and executable permissions are added on macOS. macOS support is limited as OS is primarily tested on Windows. Use full paths if shorthands fail.

### Example 3: OpenGL Function Loading with GLAD
This example uses GLAD to load OpenGL functions, targeting Windows and using `extern "kernel32"`. It incorporates error checking for `glad_glGetError` based on prior conversations. Dynamic library management ensures a clean output directory.

```os
extern "C" {
    fn printf(fmt: char*, ...) => int; // Shorthand for msvcrt.dll
}

extern "kernel32" {
    fn GetProcAddress(hModule: void*, lpProcName: char*) => void*; // Windows-specific
}

extern "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dll", 
       "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/libglfw3.a", 
       "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.so", 
       "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dylib", 
       "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.wa" {
    fn glfwInit() => int;
    fn glfwCreateWindow(width: int, height: int, title: char*, monitor: void*, share: void*) => void*;
    fn glfwMakeContextCurrent(window: void*) => void;
    fn glfwSwapBuffers(window: void*) => void;
    fn glfwPollEvents() => void;
    fn glfwWindowShouldClose(window: void*) => int;
    fn glfwTerminate() => void;
}

extern "dependencies/glad/gl/bin/glad_gl.dll", 
       "dependencies/glad/gl/bin/libglad_gl.a" {
    fn gladLoadGL(loader: void*) => int;
    const glad_glGetError: fn() => uint;
    const glad_glClear: fn(mask: uint) => void;
    const glad_glClearColor: fn(r: float, g: float, b: float, a: float) => void;
}

const GL_COLOR_BUFFER_BIT: uint = 0x00004000;

function main() => i32 {
    if (!glfwInit()) {
        printf("GLFW initialization failed\n");
        return 1;
    }

    let window = glfwCreateWindow(800, 600, "OS GLAD Example", nullptr, nullptr);
    if (window == nullptr) {
        printf("GLFW window creation failed\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    // Use kernel32's GetProcAddress for GLAD loader
    if (!gladLoadGL(GetProcAddress)) {
        printf("GLAD initialization failed\n");
        glfwTerminate();
        return 1;
    }

    // Clear screen with a blue background
    glad_glClearColor(0.0, 0.5, 1.0, 1.0);
    glad_glClear(GL_COLOR_BUFFER_BIT);

    // Check for OpenGL errors
    let gl_error = glad_glGetError();
    if (gl_error != 0) {
        printf("OpenGL error after clear: %u\n", gl_error);
        glfwTerminate();
        return 1;
    }

    glfwSwapBuffers(window);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwTerminate();
    printf("GLAD application closed\n");
    return 0;
}
```

Run with (on a Windows host):
```bash
./bin/Debug-windows-x86_64/Osengine.exe glad_example.os --target-os windows
```

Or compile to an executable for Windows:
```bash
./bin/Debug-windows-x86_64/Osengine.exe glad_example.os --target-os windows --make -o glad_example.exe
```

**Note**: During AOT compilation, linker flags like `-lglad_gl` and `-lglfw3` (or `libglad_gl.lib`, `libglfw3.lib` for MSVC) are automatically generated. The backend tries `clang++`, `g++`, and `link`, falling back as needed, and throws an error if all fail. Non-system dynamic libraries like `glfw3.dll` and `glad_gl.dll` will be copied to the output directory only if they're actually needed. System libraries like `kernel32.dll` won't be copied. A symbol table is generated, but no permissions are added on Windows. If `extern "kernel32"` fails, use `extern "C:/Windows/System32/kernel32.dll"`. Using platform-specific symbols (e.g., `GetProcAddress`) with a mismatched `--target-os` may result in errors like:
```
ERROR: Failed to resolve external function: GetProcAddress
```

## How the FFI Works

The FFI in OS is built around the `extern` keyword, with shorthands like `extern "C"` and `extern "kernel32"` for common system libraries. Here's a breakdown of its key components:

1. **Function Declarations**: External functions are declared with their signatures. For example:
   ```os
   extern "C" fn printf(fmt: char*, ...) => int;
   ```
   This is a shorthand for the C standard library (e.g., `system32/msvcrt.dll`).

2. **Constant and Static Variable Declarations**: The FFI supports constants and static variables using `const` or `let`. For example:
   ```os
   extern "dependencies/glad/gl/bin/glad_gl.dll" {
       const GL_COLOR_BUFFER_BIT: uint = 0x00004000;
       const glad_glGetError: fn() => uint;
   }
   ```
   Static variables are loaded after calling a loader function (e.g., `gladLoadGL`).

3. **Multiple Library Paths**: Developers can specify multiple library formats:
   ```os
   extern "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dll", 
          "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/libglfw3.a", 
          "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.so", 
          "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dylib", 
          "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.wa" {
       fn glfwInit() => int;
   }
   ```
   During JIT, paths verify symbol existence. During AOT, the library is not required at runtime, and dynamic library management handles copying only necessary libraries.

4. **Smart Dynamic Library Management**: During AOT compilation, the FFI implements intelligent dynamic library handling:
   - **System Library Detection**: Automatically identifies system libraries that don't need copying:
     - **Windows**: `kernel32.dll`, `user32.dll`, `msvcrt.dll`, `ntdll.dll`, `gdi32.dll`, `shell32.dll`, `advapi32.dll`, `ole32.dll`, `oleaut32.dll`, `ws2_32.dll`
     - **Unix-like**: Libraries in `/lib/`, `/usr/lib/`, `/lib64/`, `/usr/lib64/`, plus common system libraries like `libc.so`, `libm.so`, `libpthread.so`, `libdl.so`, `librt.so`
   - **Selective Copying**: Only copies non-system dynamic libraries that are actually needed for the application to work
   - **Cleanup Strategy**: Removes copied dynamic libraries when they're not needed:
     - If static library resolution succeeds after a dynamic library was copied, the copied library is deleted
     - If dynamic library resolution fails after copying, the copied library is deleted
     - If resolver creation fails after copying, the copied library is deleted
   - **OS-Aware Behavior**: Adapts to the target OS specified by `--target-os` for proper system library detection

5. **Automatic Linker Argument Generation**: During AOT compilation, linker arguments are generated from library paths. For example, `"dependencies/curl/lib/libcurl.a"` produces `-lcurl` (or `libcurl.lib` for MSVC). Default libraries (e.g., `-lm`, `-ldl`, `-lpthread` on non-Windows; `user32.lib`, `kernel32.lib` on Windows) are automatically included. The backend constructs arguments using:
   - `buildLinkerArgs` for `clang++`/`g++`, combining output file, object file, additional libraries, and default libraries (e.g., `["-o", "output.exe", "input.o", "-lcurl", "-lm"]`).
   - `buildMSVCLinkerArgs` for MSVC `link`, converting `-l` flags to `.lib` (e.g., `["/OUT:output.exe", "input.o", "libcurl.lib", "user32.lib"]`).

6. **Linker Fallback Mechanism**: The backend attempts linking with available linkers in order (`clang++`, `g++`, `link` on Windows). If a linker fails (non-zero exit code), it logs the error (e.g., "Linker clang++ failed with exit code: 1") and tries the next. If all fail, it throws an error with available linkers and the last error message.

7. **Symbol Table Generation**: The backend generates a symbol table during AOT compilation, documenting all module symbols (globals, functions, aliases) in a human-readable format. It uses `llvm::raw_fd_ostream` to write to a file, throwing an error if the file cannot be opened. The table includes detailed type information and is logged upon completion (e.g., "Symbol table emitted to: symbols.txt").

8. **Platform-Specific Permissions**: On non-Windows platforms, the backend adds executable permissions to the output file, ensuring it can be run immediately. This is implemented using `fs::permissions` with `fs::perms::owner_exec`, `group_exec`, and `others_exec`.

9. **System Library Shorthands with Full Path Fallback**: `extern "C"` abstracts the C standard library path, and `extern "kernel32"` simplifies access to `kernel32.dll`. If shorthands fail, use full paths, e.g.:
   ```os
   extern "C:/Windows/System32/kernel32.dll" fn GetProcAddress(...);
   ```

10. **Target OS Selection**: The `--target-os` flag specifies the target platform (`linux`, `windows`, `macos`, `freebsd`, `android`, `ios`, `wasm`, `auto`). JIT requires host-target OS matching, while AOT supports cross-compilation. The target OS also affects system library detection for dynamic library management. Windows is the primary tested platform.

11. **Type Safety and Standard Library Abstraction**: Incorrect type declarations can cause crashes. The standard library will provide type-safe abstractions, e.g.:
    ```os
    import { console } from "std";
    console.log("Hello, OS!");
    ```

12. **Performance Optimization**: The FFI integrates with OS's LLVM backend, ensuring optimized calls with `-O3`.

13. **Future-Proof Design**: The FFI supports any LLVM-compatible library format and future non-C libraries.

14. **Solo Developer Context**: As a solo developer project, OS is primarily tested on Windows, with limited support for other OSes. Future updates will expand cross-platform compatibility.

## Why OS's FFI is Powerful

The FFI's design makes OS a standout choice, despite being a solo developer effort:
- **Universal Compatibility**: Supports any C-based library with static or dynamic files across domains.
- **Simplified Workflow**: Shorthands, full path fallbacks, automatic linker argument generation, linker fallback, symbol table generation, and intelligent dynamic library management streamline development.
- **Cross-Platform Potential**: The `--target-os` flag enables portability, with Windows as the primary tested platform.
- **Clean Deployment**: Smart dynamic library management ensures only necessary runtime dependencies are included.
- **Debugging Support**: Symbol table generation aids in verifying FFI declarations.
- **Safety and Abstraction**: The standard library will reduce manual FFI usage and errors.
- **Performance**: LLVM integration ensures fast FFI calls.
- **Extensibility**: Designed for future non-C libraries and expanded OS support.

## Next Steps
To explore OS's FFI further:
- **Experiment with System Libraries**: Try full paths if shorthands fail.
- **Cross-Platform Testing**: Use `--target-os` for supported platforms, noting Windows focus.
- **Advanced FFI Usage**: Declare complex data structures or static variables.
- **Debugging with Symbol Tables**: Use generated symbol tables to verify FFI declarations.
- **Linker Configuration**: Rely on automatic linker argument generation and fallback, but check logs for linker errors.
- **Library Management**: Benefit from automatic dynamic library copying and cleanup for clean deployments.

Stay tuned for more documentation, including system library shorthands, as OS evolves!