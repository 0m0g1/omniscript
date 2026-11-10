// standard/1/c/console.os
// Tier 0: raw C console/stdio-stream bindings.
// PLATFORM: MSVC/MinGW — stdout, stderr, stdin are data symbols in msvcrt.dll,
//           declared here as extern let so the linker resolves them directly.

extern "C" let stdout: void*;
extern "C" let stderr: void*;
extern "C" let stdin: void*;

extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fprintf(f: void*, fmt: char*, ...) => int;
extern "C" fn snprintf(buf: char*, size: u64, fmt: char*, ...) => int;
extern "C" fn puts(s: char*) => int;
extern "C" fn putchar(c: int) => int;
extern "C" fn fputs(s: char*, f: void*) => int;
extern "C" fn fputc(c: int, f: void*) => int;
extern "C" fn fgetc(f: void*) => int;
extern "C" fn fgets(buf: char*, size: int, f: void*) => char*;
extern "C" fn fflush(f: void*) => int;