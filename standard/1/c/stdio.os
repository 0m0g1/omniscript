// standard/1/c/stdio.os  (C runtime file I/O, used to load .spv shader files)
extern "C" fn fopen(path: char*, mode: char*) => void*;
extern "C" fn fclose(f: void*) => int;
extern "C" fn fread(buf: void*, size: u64, count: u64, f: void*) => u64;
extern "C" fn fseek(f: void*, offset: int, whence: int) => int;
extern "C" fn ftell(f: void*) => int;
