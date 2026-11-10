// standard/1/c/file.os
// Tier 0: raw C file bindings.
// PLATFORM: 64-bit seek uses the Windows names (_fseeki64/_ftelli64). On POSIX use fseeko/ftello.

extern "C" fn fopen(path: char*, mode: char*) => void*;
extern "C" fn fclose(f: void*) => int;
extern "C" fn fread(buf: void*, size: u64, count: u64, f: void*) => u64;
extern "C" fn fwrite(buf: void*, size: u64, count: u64, f: void*) => u64;
extern "C" fn fgets(buf: char*, size: int, f: void*) => char*;
extern "C" fn fgetc(f: void*) => int;
extern "C" fn fputc(c: int, f: void*) => int;
extern "C" fn fputs(s: char*, f: void*) => int;
extern "C" fn ungetc(c: int, f: void*) => int;
extern "C" fn feof(f: void*) => int;
extern "C" fn ferror(f: void*) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn fseek(f: void*, offset: int, origin: int) => int;     // 32-bit offset
extern "C" fn ftell(f: void*) => int;
extern "C" fn _fseeki64(f: void*, offset: i64, origin: int) => int;
extern "C" fn _ftelli64(f: void*) => i64;
extern "C" fn remove(path: char*) => int;
extern "C" fn rename(src: char*, to: char*) => int;
extern "C" fn tmpfile() => void*;

const SEEK_SET: int = 0;
const SEEK_CUR: int = 1;
const SEEK_END: int = 2;
const EOF: int = -1;
