// standard/1/IO.os
// Files and standard streams on top of C stdio.
// Text goes in as char* or String*; text comes out into a String* you own (see String.os).
// Failure: bool return (false = failed / EOF). There is no defer yet, so close() your files.
//
// ASSUMPTIONS (patch if the compiler rejects them):
//   - `Str.String` is how a type from an imported module is named.
//   - i64 exists; 64-bit seek is Windows (_fseeki64/_ftelli64). On POSIX swap in fseeko/ftello.
//   - `f.field` and `out.field` work through a struct pointer (as `props[i].queueFlags` does).

module Str = import "standard/1/String.os";

extern "C" fn fopen(path: char*, mode: char*) => void*;
extern "C" fn fclose(f: void*) => int;
extern "C" fn fread(buf: void*, size: u64, count: u64, f: void*) => u64;
extern "C" fn fwrite(buf: void*, size: u64, count: u64, f: void*) => u64;
extern "C" fn fgetc(f: void*) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn feof(f: void*) => int;
extern "C" fn ferror(f: void*) => int;
extern "C" fn remove(path: char*) => int;
extern "C" fn rename(src: char*, to: char*) => int;
extern "C" fn strlen(s: char*) => u64;
extern "C" fn _fseeki64(f: void*, offset: i64, origin: int) => int;
extern "C" fn _ftelli64(f: void*) => i64;
extern "C" fn __acrt_iob_func(index: uint) => void*;   // Windows UCRT: 0 stdin, 1 stdout, 2 stderr

// fopen mode strings. Always binary, so Windows never rewrites newlines.
let MODE_READ: char* = "rb";
let MODE_WRITE: char* = "wb";
let MODE_APPEND: char* = "ab";
let MODE_READWRITE: char* = "r+b";

let SEEK_SET: int = 0;
let SEEK_CUR: int = 1;
let SEEK_END: int = 2;

struct File {
    handle: void* = nullptr;
    isOpen: bool = false;

    open(path: char*, mode: char*) => bool {
        this.handle = fopen(path, mode);
        this.isOpen = this.handle != nullptr;
        return this.isOpen;
    }

    // Attach a process stream: 0 = stdin, 1 = stdout, 2 = stderr. Do not close() these.
    attach(which: uint) => void {
        this.handle = __acrt_iob_func(which);
        this.isOpen = true;
    }

    raw() => void* { return this.handle; }   // escape hatch: the C FILE*

    close() => void {
        if (this.isOpen) fclose(this.handle);
        this.handle = nullptr;
        this.isOpen = false;
    }

    flush() => void { fflush(this.handle); }
    eof() => bool { return feof(this.handle) != 0; }
    hasError() => bool { return ferror(this.handle) != 0; }

    tell() => i64 { return _ftelli64(this.handle); }
    seek(offset: i64, origin: int) => bool { return _fseeki64(this.handle, offset, origin) == 0; }

    size() => i64 {
        let here: i64 = _ftelli64(this.handle);
        _fseeki64(this.handle, 0, SEEK_END);
        let end: i64 = _ftelli64(this.handle);
        _fseeki64(this.handle, here, SEEK_SET);
        return end;
    }

    // Reads up to n bytes into buf; returns how many were read (0 at EOF).
    readBytes(buf: void*, n: u64) => u64 { return fread(buf, 1, n, this.handle); }

    // Replaces out with the whole file contents.
    readAll(out: Str.String*) => bool {
        let total: u64 = this.size() as u64;
        this.seek(0, SEEK_SET);
        out.clear();
        if (!out.reserve(total)) return false;
        let got: u64 = fread(out.data as void*, 1, total, this.handle);
        out.len = got;
        out.data[got] = 0 as char;
        return true;
    }

    // Replaces out with the next line (no \n / \r\n). false at EOF.
    readLine(out: Str.String*) => bool {
        out.clear();
        let gotAny: bool = false;
        let done: bool = false;
        while (!done) {
            let c: int = fgetc(this.handle);
            if (c < 0) {
                done = true;
            } else {
                gotAny = true;
                if (c == 10) done = true; else out.appendChar(c as char);
            }
        }
        if (!gotAny) return false;
        if (out.len > 0 && (out.data[out.len - 1] as int) == 13) out.truncate(out.len - 1);
        return true;
    }

    writeBytes(buf: void*, n: u64) => bool { return fwrite(buf, 1, n, this.handle) == n; }

    write(text: char*) => bool {
        let n = strlen(text);
        return fwrite(text as void*, 1, n, this.handle) == n;
    }

    writeString(s: Str.String*) => bool { return this.writeBytes(s.cstr() as void*, s.len); }

    writeLine(text: char*) => bool {
        let ok = this.write(text);
        if (ok) ok = this.write("\n");
        return ok;
    }

    writeLineString(s: Str.String*) => bool {
        let ok = this.writeString(s);
        if (ok) ok = this.write("\n");
        return ok;
    }
}

// ---- one-liners ----
function readFile(path: char*, out: Str.String*) => bool {
    let f: File;
    if (!f.open(path, MODE_READ)) return false;
    let ok = f.readAll(out);
    f.close();
    return ok;
}

function writeFile(path: char*, text: char*) => bool {
    let f: File;
    if (!f.open(path, MODE_WRITE)) return false;
    let ok = f.write(text);
    f.close();
    return ok;
}

function writeFileString(path: char*, s: Str.String*) => bool {
    let f: File;
    if (!f.open(path, MODE_WRITE)) return false;
    let ok = f.writeString(s);
    f.close();
    return ok;
}

function appendFile(path: char*, text: char*) => bool {
    let f: File;
    if (!f.open(path, MODE_APPEND)) return false;
    let ok = f.write(text);
    f.close();
    return ok;
}

function deleteFile(path: char*) => bool { return remove(path) == 0; }
function renameFile(src: char*, to: char*) => bool { return rename(src, to) == 0; }
