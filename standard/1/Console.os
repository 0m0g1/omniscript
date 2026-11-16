// standard/1/Console.os

//! Printing to the terminal and reading a line from it.
//!
//! Output goes to stdout (`print`, `println`, `log`, `info`) or stderr (`warn`, `error`).
//! Every function also has a `String` version (`printString`, `printlnString`, ...)
//! that takes a `Str.String*` instead of a `char*`.
//!
//! ```
//! import "std/Console";
//!
//! Console.println("hello");
//! Console.warn("disk is almost full");
//! ```
//!
//! Formatted output: variadic forwarding is not possible yet. Either call the extern `printf`
//! directly, or build the text with `String.append` / `appendInt` and print it with `printlnString`.

// ASSUMPTION: `Str.String` is how a type from an imported module is named.

module Str = import "standard/1/String.os";

extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fputs(s: char*, f: void*) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn fgetc(f: void*) => int;
extern "C" fn __acrt_iob_func(index: uint) => void*;   // Windows UCRT; swap per platform

/// Whether `info`, `warn` and `error` color their `[info]`, `[warn]` and `[error]` tags
/// with ANSI escape codes. On by default; change it with `setColor`.
let useColor: bool = true;

/// Turns the colored tags of `info`, `warn` and `error` on or off.
/// Turn them off when the output goes to a file or a terminal without ANSI support.
function setColor(on: bool) => void { useColor = on; }

/// The C `stdout` stream, for use with the raw C functions.
/// Windows only for now, because it calls `__acrt_iob_func`.
function out() => void* { return __acrt_iob_func(1); }

/// The C `stderr` stream, for use with the raw C functions.
/// Windows only for now, because it calls `__acrt_iob_func`.
function err() => void* { return __acrt_iob_func(2); }

// ---- char* ----

/// Writes `s` to stdout without a trailing newline.
function print(s: char*) => void { fputs(s, out()); }

/// Writes `s` to stdout, followed by a newline.
///
/// ```
/// Console.println("hello");   // hello
/// ```
function println(s: char*) => void { fputs(s, out()); fputs("\n", out()); }

/// Prints an integer followed by a newline.
function println(v: i64) => void {
    let s: Str.String;
    s.appendInt(v);
    printlnString(&s);
    s.free();
}

/// Prints a floating-point number followed by a newline.
function println(v: f64) => void { printf("%g\n", v); }

/// Prints `true` or `false` followed by a newline.
function println(v: bool) => void {
    if (v) println("true"); else println("false");
}

/// Prints a `String` followed by a newline.
function println(v: Str.String*) => void { printlnString(v); }

function log(v: i64) => void { println(v); }
function log(v: f64) => void { println(v); }
function log(v: bool) => void { println(v); }
function log(v: Str.String*) => void { println(v); }
/// Prints a C string followed by a newline.
function log(s: char*) => void { println(s); }

/// 32-bit and unsigned integers, so `Console.log(someInt)` and `Console.log(someU32)` need no cast.
function log(v: i32) => void { println(v as i64); }
function log(v: u32) => void { println(v as i64); }
function log(v: u64) => void { println(v as i64); }

/// 32-bit float.
function log(v: f32) => void { println(v as f64); }

/// Writes `s` to stdout as an informational line, starting with an `[info]` tag.
/// The tag is colored cyan unless `setColor(false)` was called.
function info(s: char*) => void {
    if (useColor) fputs("\x1b[36m[info]\x1b[0m ", out()); else fputs("[info] ", out());
    println(s);
}

/// Writes `s` to **stderr**, starting with a `[warn]` tag (yellow unless `setColor(false)`).
/// stdout is flushed first, so output from `println` and `warn` appears in the right order.
function warn(s: char*) => void {
    fflush(out());   // keep stdout/stderr ordering sane
    if (useColor) fputs("\x1b[33m[warn]\x1b[0m ", err()); else fputs("[warn] ", err());
    fputs(s, err());
    fputs("\n", err());
}

/// Writes `s` to **stderr**, starting with an `[error]` tag (red unless `setColor(false)`).
/// stdout is flushed first, so output from `println` and `error` appears in the right order.
function error(s: char*) => void {
    fflush(out());
    if (useColor) fputs("\x1b[31m[error]\x1b[0m ", err()); else fputs("[error] ", err());
    fputs(s, err());
    fputs("\n", err());
}

// ---- String* ----

/// Like `print`, but takes a `String`.
function printString(s: Str.String*) => void { print(s.cstr()); }

/// Like `println`, but takes a `String`.
function printlnString(s: Str.String*) => void { println(s.cstr()); }

/// Like `info`, but takes a `String`.
function infoString(s: Str.String*) => void { info(s.cstr()); }

/// Like `warn`, but takes a `String`.
function warnString(s: Str.String*) => void { warn(s.cstr()); }

/// Like `error`, but takes a `String`.
function errorString(s: Str.String*) => void { error(s.cstr()); }

/// Forces everything written to stdout so far to appear now.
/// Useful after `print` when you want a prompt to show before waiting for input.
function flush() => void { fflush(out()); }

/// Reads one line from stdin into `out`, without the trailing `\n` or `\r\n`.
/// Anything already in `out` is cleared first.
/// Returns `false` when stdin is at end of file and nothing was read.
///
/// ```
/// let name: Str.String;
/// Console.print("name? ");
/// Console.flush();
/// if (Console.readLine(&name)) Console.println(name.cstr());
/// name.free();
/// ```
function readLine(out: Str.String*) => bool {
    out.clear();
    let gotAny: bool = false;
    let done: bool = false;
    while (!done) {
        let c: int = fgetc(__acrt_iob_func(0));
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

/// Clears the terminal and moves the cursor to the top-left corner.
/// Uses ANSI escape codes, so Windows 10 or later needs virtual terminal mode enabled.
function clear() => void { fputs("\x1b[2J\x1b[H", out()); }
