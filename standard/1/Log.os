// standard/1/Log.os
// Tier 4: leveled logger with timestamps, optional file sink. Levels: 0 debug 1 info 2 warn 3 error.
module Str = import "standard/1/String.os";
module Time = import "standard/1/Time.os";

extern "C" fn fopen(path: char*, mode: char*) => void*;
extern "C" fn fclose(f: void*) => int;
extern "C" fn fputs(s: char*, f: void*) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn time(t: i64*) => i64;
extern "C" fn localtime(t: i64*) => void*;
extern "C" fn strftime(buf: char*, n: u64, fmt: char*, tm: void*) => u64;
extern "C" fn __acrt_iob_func(index: uint) => void*;   // PLATFORM

let minLevel: int = 1;
let sink: void* = nullptr;       // file sink (nullptr = none)
let toConsole: bool = true;

function setLevel(l: int) => void { minLevel = l; }
function setConsole(on: bool) => void { toConsole = on; }
function openFile(path: char*) => bool { sink = fopen(path, "a"); return sink != nullptr; }
function closeFile() => void { if (sink != nullptr) { fclose(sink); sink = nullptr; } }

function emit(level: int, tag: char*, msg: char*) => void {
    if (level < minLevel) return;
    let t: i64 = time(nullptr);
    let stamp: [32]char;
    strftime(&stamp as char*, 32, "%Y-%m-%d %H:%M:%S", localtime(&t));
    let line: Str.String;
    line.append("["); line.append(&stamp as char*); line.append("] [");
    line.append(tag); line.append("] "); line.append(msg); line.append("\n");
    if (toConsole) {
        let f = __acrt_iob_func(1);
        if (level >= 2) f = __acrt_iob_func(2);
        fputs(line.cstr(), f);
    }
    if (sink != nullptr) { fputs(line.cstr(), sink); fflush(sink); }
    line.free();
}

function debug(msg: char*) => void { emit(0, "debug", msg); }
function info(msg: char*) => void { emit(1, "info", msg); }
function warn(msg: char*) => void { emit(2, "warn", msg); }
function error(msg: char*) => void { emit(3, "error", msg); }
