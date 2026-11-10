// standard/1/Console.os
// stdout: print, println, log, info (+ String versions).  stderr: warn, error.
// Formatting: variadic forwarding is not possible yet, so call the extern printf directly,
// or build text with String.append / appendInt and print it with printlnString.
//
// ASSUMPTION: `Str.String` is how a type from an imported module is named.

include "standard/1/String.os";

extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fputs(s: char*, f: void*) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn fgetc(f: void*) => int;
extern "C" fn __acrt_iob_func(index: uint) => void*;   // Windows UCRT; swap per platform

let useColor: bool = true;

function setColor(on: bool) => void { useColor = on; }

function out() => void* { return __acrt_iob_func(1); }
function err() => void* { return __acrt_iob_func(2); }

// ---- char* ----
function print(s: char*) => void { fputs(s, out()); }
function println(s: char*) => void { fputs(s, out()); fputs("\n", out()); }
function log(s: char*) => void { println(s); }

function info(s: char*) => void {
    if (useColor) fputs("\x1b[36m[info]\x1b[0m ", out()); else fputs("[info] ", out());
    println(s);
}

function warn(s: char*) => void {
    fflush(out());   // keep stdout/stderr ordering sane
    if (useColor) fputs("\x1b[33m[warn]\x1b[0m ", err()); else fputs("[warn] ", err());
    fputs(s, err());
    fputs("\n", err());
}

function error(s: char*) => void {
    fflush(out());
    if (useColor) fputs("\x1b[31m[error]\x1b[0m ", err()); else fputs("[error] ", err());
    fputs(s, err());
    fputs("\n", err());
}

// ---- String* ----
function printString(s: Str.String*) => void { print(s.cstr()); }
function printlnString(s: Str.String*) => void { println(s.cstr()); }
function infoString(s: Str.String*) => void { info(s.cstr()); }
function warnString(s: Str.String*) => void { warn(s.cstr()); }
function errorString(s: Str.String*) => void { error(s.cstr()); }

function flush() => void { fflush(out()); }

// Reads a line from stdin into out (no \n / \r\n). false at EOF.
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

// ANSI clear + cursor home (Windows 10+ needs virtual terminal mode enabled for this).
function clear() => void { fputs("\x1b[2J\x1b[H", out()); }
