# External Bindings

OS provides powerful integration with C libraries and external functions through the `extern` keyword, allowing seamless interoperability with existing codebases and system libraries.

## Basic External Function Declaration

### C Library Functions
```os
extern "C" {
    fn printf(fmt: char*, ...) => int;
    fn malloc(size: uint) => void*;
    fn free(ptr: void*) => void;
    fn strlen(str: char*) => uint;
    fn strcpy(dest: char*, src: char*) => char*;
}
```

### Usage Example
```os
extern "C" {
    fn printf(fmt: char*, ...) => int;
}

function main() => i32 {
    printf("Hello from C library!\n");
    return 0;
}
```

## External Libraries with Linking

OS supports linking with both dynamic libraries (DLLs) and static libraries:

### Dynamic Library Linking
```os
extern "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dll", 
       "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/libglfw3.a" {
    fn glfwInit() => int;
    fn glfwCreateWindow(width: int, height: int, title: char*, 
                        monitor: void*, share: void*) => void*;
    fn glfwMakeContextCurrent(window: void*) => void;
    fn glfwWindowShouldClose(window: void*) => int;
    fn glfwPollEvents() => void;
    fn glfwSwapBuffers(window: void*) => void;
    fn glfwTerminate() => void;
}
```

### OpenGL Library Example
```os
extern "C:/Windows/System32/opengl32.dll", 
       "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64/OpenGL32.lib" {
    fn glClearColor(r: float, g: float, b: float, a: float) => void;
    fn glClear(mask: uint) => void;
    fn glBegin(mode: uint) => void;
    fn glEnd() => void;
    fn glVertex2f(x: float, y: float) => void;
    fn glColor3f(r: float, g: float, b: float) => void;
    fn glLoadIdentity() => void;
    fn glViewport(x: int, y: int, width: int, height: int) => void;
    fn glMatrixMode(mode: uint) => void;
    fn glOrtho(left: double, right: double, bottom: double, 
               top: double, near: double, far: double) => void;
}
```

## Function Declaration Syntax

### Basic Syntax
```os
extern "calling_convention" {
    fn function_name(parameters) => return_type;
}
```

### With Library Paths
```os
extern "dll_path", "lib_path" {
    fn function_name(parameters) => return_type;
}
```

## Calling Conventions

### Standard Calling Conventions
```os
extern "C" {
    fn c_function(x: int) => int;
}

extern "stdcall" {
    fn windows_function(x: int) => int;
}

extern "cdecl" {
    fn cdecl_function(x: int) => int;
}

extern "fastcall" {
    fn fast_function(x: int, y: int) => int;
}
```

## Parameter Types in External Functions

### Basic Types
```os
extern "C" {
    fn process_int(value: int) => void;
    fn process_float(value: float) => void;
    fn process_double(value: double) => void;
    fn process_char(value: char) => void;
    fn process_bool(value: bool) => void;
}
```

### Pointer Types
```os
extern "C" {
    fn process_string(str: char*) => void;
    fn process_buffer(buffer: void*, size: uint) => void;
    fn process_int_array(arr: int*, count: uint) => void;
    fn get_pointer() => void*;
}
```

### Array Parameters
```os
extern "C" {
    fn process_fixed_array(arr: [10]int) => void;
    fn sort_array(arr: int*, size: uint) => void;
}
```

## Variadic Functions

External variadic functions are supported:

```os
extern "C" {
    fn printf(fmt: char*, ...) => int;
    fn sprintf(buffer: char*, fmt: char*, ...) => int;
    fn scanf(fmt: char*, ...) => int;
}

// Usage
printf("Number: %d, String: %s\n", 42, "Hello");
```

## Constants from External Libraries

Define constants that correspond to external library values:

```os
// OpenGL constants
const GL_COLOR_BUFFER_BIT = 0x00004000;
const GL_PROJECTION = 0x1701;
const GL_MODELVIEW = 0x1700;
const GL_TRIANGLES = 0x0004;

// Usage with external functions
extern "opengl32.dll" {
    fn glClear(mask: uint) => void;
    fn glMatrixMode(mode: uint) => void;
}

glClear(GL_COLOR_BUFFER_BIT);
glMatrixMode(GL_PROJECTION);
```

## Complete External Library Example

### GLFW and OpenGL Integration
```os
// External library declarations
extern "C" {
    fn printf(fmt: char*, ...) => int;
}

extern "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dll", 
       "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/libglfw3.a" {
    fn glfwInit() => int;
    fn glfwCreateWindow(width: int, height: int, title: char*, 
                        monitor: void*, share: void*) => void*;
    fn glfwMakeContextCurrent(window: void*) => void;
    fn glfwWindowShouldClose(window: void*) => int;
    fn glfwPollEvents() => void;
    fn glfwSwapBuffers(window: void*) => void;
    fn glfwTerminate() => void;
}

extern "C:/Windows/System32/opengl32.dll", 
       "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64/OpenGL32.lib" {
    fn glClearColor(r: float, g: float, b: float, a: float) => void;
    fn glClear(mask: uint) => void;
    fn glBegin(mode: uint) => void;
    fn glEnd() => void;
    fn glVertex2f(x: float, y: float) => void;
    fn glColor3f(r: float, g: float, b: float) => void;
    fn glLoadIdentity() => void;
    fn glViewport(x: int, y: int, width: int, height: int) => void;
    fn glMatrixMode(mode: uint) => void;
    fn glOrtho(left: double, right: double, bottom: double, 
               top: double, near: double, far: double) => void;
}

// Constants
const GL_COLOR_BUFFER_BIT = 0x00004000;
const GL_PROJECTION = 0x1701;
const GL_MODELVIEW = 0x1700;
const GL_TRIANGLES = 0x0004;

// Application code
function main() => i32 {
    if (glfwInit() == 0) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }

    let window = glfwCreateWindow(800, 600, "OS OpenGL Window", nullptr, nullptr);
    if (window == nullptr) {
        printf("Failed to create window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    
    // Set up OpenGL viewport and projection
    glViewport(0, 0, 800, 600);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    // Main render loop
    while (glfwWindowShouldClose(window) == 0) {
        glClearColor(0.1, 0.1, 0.1, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glLoadIdentity();

        // Draw a triangle
        glBegin(GL_TRIANGLES);
        glColor3f(1.0, 0.0, 0.0);  // Red
        glVertex2f(0.0, 0.5);
        glColor3f(0.0, 1.0, 0.0);  // Green  
        glVertex2f(-0.5, -0.5);
        glColor3f(0.0, 0.0, 1.0);  // Blue
        glVertex2f(0.5, -0.5);
        glEnd();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
```

## System Library Integration

### Windows API Example
```os
extern "kernel32.dll" {
    fn GetCurrentProcessId() => uint;
    fn Sleep(milliseconds: uint) => void;
    fn GetTickCount() => uint;
}

extern "user32.dll" {
    fn MessageBoxA(hwnd: void*, text: char*, caption: char*, type: uint) => int;
}

// Usage
let process_id = GetCurrentProcessId();
MessageBoxA(nullptr, "Hello from OS!", "Message", 0);
Sleep(1000);  // Sleep for 1 second
```

### POSIX/Unix Example
```os
extern "C" {
    fn getpid() => int;
    fn sleep(seconds: uint) => uint;
    fn write(fd: int, buffer: void*, count: uint) => int;
    fn read(fd: int, buffer: void*, count: uint) => int;
}

// Usage
let pid = getpid();
write(1, "Hello from POSIX!\n", 18);  // Write to stdout
sleep(1);
```

## Memory Management with External Libraries

```os
extern "C" {
    fn malloc(size: uint) => void*;
    fn calloc(count: uint, size: uint) => void*;
    fn realloc(ptr: void*, size: uint) => void*;
    fn free(ptr: void*) => void;
}

function allocate_buffer(size: uint) => char* {
    let buffer = malloc(size) as char*;
    if (buffer == nullptr) {
        printf("Memory allocation failed!\n");
        return nullptr;
    }
    return buffer;
}

function deallocate_buffer(buffer: char*) => void {
    if (buffer != nullptr) {
        free(buffer as void*);
    }
}
```

## Error Handling with External Functions

```os
extern "C" {
    fn fopen(filename: char*, mode: char*) => void*;
    fn fclose(file: void*) => int;
    fn fprintf(file: void*, format: char*, ...) => int;
    fn ferror(file: void*) => int;
}

function write_to_file(filename: char*, content: char*) => bool {
    let file = fopen(filename, "w");
    if (file == nullptr) {
        printf("Failed to open file: %s\n", filename);
        return false;
    }
    
    let result = fprintf(file, "%s", content);
    if (result < 0 || ferror(file) != 0) {
        printf("Failed to write to file\n");
        fclose(file);
        return false;
    }
    
    fclose(file);
    return true;
}
```

## Best Practices for External Bindings

1. **Always check return values** from external functions for errors
2. **Use appropriate calling conventions** for the target platform
3. **Match parameter types exactly** with the external function signatures
4. **Handle null pointers** and error conditions gracefully
5. **Free allocated memory** when using malloc/free from C libraries
6. **Use const correctness** for string literals and read-only data
7. **Group related functions** in the same extern block
8. **Define constants** for magic numbers used by external libraries

## Platform-Specific Considerations

### Windows
- Use `"stdcall"` for Windows API functions
- Link with `.dll` and `.lib` files
- Handle Unicode strings appropriately

### Linux/Unix
- Use `"C"` calling convention for system calls
- Link with `.so` shared libraries
- Handle POSIX-specific error codes

### Cross-Platform Code
```os
#ifdef WINDOWS
extern "kernel32.dll" {
    fn Sleep(ms: uint) => void;
}
#else
extern "C" {
    fn usleep(microseconds: uint) => int;
}
#endif

function platform_sleep(milliseconds: uint) => void {
    #ifdef WINDOWS
        Sleep(milliseconds);
    #else
        usleep(milliseconds * 1000);
    #endif
}
```

---

*External binding features are actively being developed and may change.*