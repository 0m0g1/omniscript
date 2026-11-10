// standard/1/c/console.os
// Tier 0: raw C console/stdio-stream bindings.
// stdout / stderr / stdin are macros for __acrt_iob_func(N) on modern Windows.
// The wrapper functions below make them callable from Omniscript.

extern "C" fn __acrt_iob_func(index: uint) => void*;

function stdin()  => void* { return __acrt_iob_func(0); }
function stdout() => void* { return __acrt_iob_func(1); }
function stderr() => void* { return __acrt_iob_func(2); }

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