// standard/1/Ffi.os
// Tier 8: load shared libraries at runtime, resolve symbols, C string helpers.
module D = import "standard/1/c/dl.os";
module Str = import "standard/1/String.os";
module CMem = import "standard/1/c/mem.os";
module CStr = import "standard/1/c/string.os";

struct Library {
    h: void* = nullptr;
    open(path: char*) => bool { this.h = D.LoadLibraryA(path); return this.h != nullptr; }
    close() => void { if (this.h != nullptr) { D.FreeLibrary(this.h); this.h = nullptr; } }
    symbol(name: char*) => void* { if (this.h == nullptr) return nullptr; return D.GetProcAddress(this.h, name); }
    // let f = lib.symbolAs<fn(int) => int>("name");  (or: lib.symbol("name") as fn(int) => int)
    symbolAs<F>(name: char*) => F { return this.symbol(name) as F; }
}

function loadLibrary(path: char*, out: Library*) => bool { return out.open(path); }
function getSymbol(lib: Library*, name: char*) => void* { return lib.symbol(name); }

// Copies s into a fresh malloc'd NUL-terminated buffer (caller frees with CMem.free).
function toCString(s: Str.String*) => char* {
    let p = CMem.malloc(s.len + 1) as char*;
    if (p == nullptr) return nullptr;
    CMem.memcpy(p as void*, s.cstr() as void*, s.len + 1);
    return p;
}
function fromCString(p: char*, out: Str.String*) => bool { return out.set(p); }
function fromCStringN(p: char*, n: u64, out: Str.String*) => bool { return out.appendN(p, n); }
// Callbacks: pass an Omniscript function as `f as void*` to C APIs that take function pointers.
