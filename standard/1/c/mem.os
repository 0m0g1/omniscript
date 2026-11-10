// standard/1/c/mem.os
// Tier 0: raw C memory bindings. 1:1 with C, no safety, no logic.
// Import as:  module CMem = import "standard/1/c/mem.os";   then CMem.malloc(...)
// ASSUMPTION: extern "C" functions in an imported module keep their C symbol names and are called as Alias.name(...).

extern "C" fn malloc(size: u64) => void*;
extern "C" fn calloc(count: u64, size: u64) => void*;
extern "C" fn realloc(p: void*, size: u64) => void*;
extern "C" fn free(p: void*) => void;
extern "C" fn memcpy(dst: void*, src: void*, n: u64) => void*;
extern "C" fn memmove(dst: void*, src: void*, n: u64) => void*;
extern "C" fn memset(dst: void*, value: int, n: u64) => void*;
extern "C" fn memcmp(a: void*, b: void*, n: u64) => int;
extern "C" fn memchr(p: void*, value: int, n: u64) => void*;
