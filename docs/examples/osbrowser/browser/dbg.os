// browser/dbg.os  -- log buffer shared by the engine, loader and inspector (prints to terminal AND keeps history).
module U = import "browser/util.os";
module Console = import "standard/1/Console.os";

let lines: char** = nullptr;
let count: int = 0;

function push(s: char*) => void {
    if (lines == nullptr) lines = U.alloc(8 * 256) as char**;
    if (count < 256) { lines[count] = U.dup(s); count = count + 1; }
    else {
        let i: int = 1;
        while (i < 256) { lines[i - 1] = lines[i]; i = i + 1; }
        lines[255] = U.dup(s);
    }
}

function log(s: char*) => void { Console.log(s); push(s); }
function error(s: char*) => void { Console.error(s); push(U.cat("[error] ", s)); }
function length() => int { return count; }
function get(i: int) => char* { return lines[i]; }
