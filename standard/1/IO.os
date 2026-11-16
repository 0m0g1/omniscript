//! Files and standard streams on top of C stdio.
//!
//! Text goes in as `char*` or `String*`; text comes out into a `String*` you own (see `String.os`).
//! Failure is reported with a `bool` return (`false` means failed, or end of file).
//! There is no `defer` yet, so `close()` your files.
//!
//! ## Example
//!
//! ```
//! let f: File;
//! if (f.open("notes.txt", MODE_READ)) {
//!     let line: Str.String;
//!     while (f.readLine(&line)) { printf("%s\n", line.cstr()); }
//!     line.free();
//!     f.close();
//! }
//! ```

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

/// Open an existing file for reading.
let MODE_READ: char* = "rb";
/// Create the file, or empty it if it exists, and open it for writing.
let MODE_WRITE: char* = "wb";
/// Open the file for writing at the end, creating it if needed.
let MODE_APPEND: char* = "ab";
/// Open an existing file for reading and writing.
let MODE_READWRITE: char* = "r+b";

/// `seek` origin: the start of the file.
let SEEK_SET: int = 0;
/// `seek` origin: the current position.
let SEEK_CUR: int = 1;
/// `seek` origin: the end of the file.
let SEEK_END: int = 2;

/// An open file or standard stream. Call `open` (or `attach`), use it, then `close`.
struct File {
    /// The underlying C `FILE*`.
    handle: void* = nullptr;
    /// `true` while the file is open.
    isOpen: bool = false;

    /// Opens `path` with one of the `MODE_` strings. Returns `false` if it could not be opened.
    open(path: char*, mode: char*) => bool {
        this.handle = fopen(path, mode);
        this.isOpen = this.handle != nullptr;
        return this.isOpen;
    }

    /// Attaches a process stream: 0 is stdin, 1 is stdout, 2 is stderr. Do not `close()` these.
    attach(which: uint) => void {
        this.handle = __acrt_iob_func(which);
        this.isOpen = true;
    }

    /// Escape hatch: returns the C `FILE*`.
    raw() => void* { return this.handle; }

    /// Closes the file. Safe to call on a file that is not open.
    close() => void {
        if (this.isOpen) fclose(this.handle);
        this.handle = nullptr;
        this.isOpen = false;
    }

    /// Writes any buffered output to the file.
    flush() => void { fflush(this.handle); }

    /// `true` once a read has hit the end of the file.
    eof() => bool { return feof(this.handle) != 0; }

    /// `true` if an I/O error has occurred on this file.
    hasError() => bool { return ferror(this.handle) != 0; }

    /// Returns the current position, in bytes from the start.
    tell() => i64 { return _ftelli64(this.handle); }

    /// Moves to `offset` bytes from `origin` (`SEEK_SET`, `SEEK_CUR` or `SEEK_END`).
    /// Returns `false` on failure.
    seek(offset: i64, origin: int) => bool { return _fseeki64(this.handle, offset, origin) == 0; }

    /// Returns the file size in bytes. The current position is left unchanged.
    size() => i64 {
        let here: i64 = _ftelli64(this.handle);
        _fseeki64(this.handle, 0, SEEK_END);
        let end: i64 = _ftelli64(this.handle);
        _fseeki64(this.handle, here, SEEK_SET);
        return end;
    }

    /// Reads up to `n` bytes into `buf`. Returns how many were read (0 at end of file).
    readBytes(buf: void*, n: u64) => u64 { return fread(buf, 1, n, this.handle); }

    /// Replaces `out` with the whole file contents, read from the start.
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

    /// Replaces `out` with the next line, without the trailing `\n` or `\r\n`.
    /// Returns `false` at end of file.
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

    /// Writes `n` bytes from `buf`. Returns `false` if fewer were written.
    writeBytes(buf: void*, n: u64) => bool { return fwrite(buf, 1, n, this.handle) == n; }

    /// Writes the C string `text`.
    write(text: char*) => bool {
        let n = strlen(text);
        return fwrite(text as void*, 1, n, this.handle) == n;
    }

    /// Writes the contents of a `String`.
    writeString(s: Str.String*) => bool { return this.writeBytes(s.cstr() as void*, s.len); }

    /// Writes the C string `text` followed by a newline.
    writeLine(text: char*) => bool {
        let ok = this.write(text);
        if (ok) ok = this.write("\n");
        return ok;
    }

    /// Writes a `String` followed by a newline.
    writeLineString(s: Str.String*) => bool {
        let ok = this.writeString(s);
        if (ok) ok = this.write("\n");
        return ok;
    }
}

// ---- one-liners ----

/// Reads the whole file at `path` into `out`. Returns `false` if it could not be opened or read.
function readFile(path: char*, out: Str.String*) => bool {
    let f: File;
    if (!f.open(path, MODE_READ)) return false;
    let ok = f.readAll(out);
    f.close();
    return ok;
}

/// Creates or overwrites the file at `path` with the C string `text`.
function writeFile(path: char*, text: char*) => bool {
    let f: File;
    if (!f.open(path, MODE_WRITE)) return false;
    let ok = f.write(text);
    f.close();
    return ok;
}

/// Creates or overwrites the file at `path` with the contents of a `String`.
function writeFileString(path: char*, s: Str.String*) => bool {
    let f: File;
    if (!f.open(path, MODE_WRITE)) return false;
    let ok = f.writeString(s);
    f.close();
    return ok;
}

/// Appends the C string `text` to the file at `path`, creating it if needed.
function appendFile(path: char*, text: char*) => bool {
    let f: File;
    if (!f.open(path, MODE_APPEND)) return false;
    let ok = f.write(text);
    f.close();
    return ok;
}

/// Deletes the file at `path`. Returns `false` on failure.
function deleteFile(path: char*) => bool { return remove(path) == 0; }

/// Renames or moves the file `src` to `to`. Returns `false` on failure.
function renameFile(src: char*, to: char*) => bool { return rename(src, to) == 0; }
