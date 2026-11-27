/// grep search: reads one file (or stdin) line by line and reports matches through output.os.
/// Binary files (a NUL byte in the first chunk) are skipped silently.
/// Memory: one line String and one 8 KB chunk are reused; files are never read whole.
/// NEXT STEPS: -A/-B/-C context, -m max count, "Binary file x matches" message, mmap/large-block scanning
/// (search the chunk for the pattern first, then split lines), multithreaded -r.

module IO = import "standard/1/IO.os";
module Str = import "standard/1/String.os";
module M = import "grep/matcher.os";
module O = import "grep/output.os";

let CHUNK: u64 = 8192;

struct Searcher {
    m: M.Matcher* = nullptr;
    o: O.Out* = nullptr;
    invert: bool = false;            // -v
    count: bool = false;             // -c
    filesOnly: bool = false;         // -l
    lineNumbers: bool = false;       // -n
    withName: bool = false;          // prefix lines with the file name
    anyMatch: bool = false;          // at least one line selected somewhere (exit code 0)
    hadError: bool = false;          // some file could not be read (exit code 2)
    chunk: Str.String;               // scratch buffer for the binary check
}

function init(s: Searcher*) => void { s.chunk.reserve(CHUNK); }
function destroy(s: Searcher*) => void { s.chunk.free(); }

function reportError(s: Searcher*, path: char*, msg: char*) => void {
    O.error(s.o, path, msg);
    s.hadError = true;
}

function hasNul(p: char*, n: u64) => bool {
    let i: u64 = 0;
    while (i < n) {
        if ((p[i] as int) == 0) return true;
        i = i + 1;
    }
    return false;
}

// Prints one selected line, highlighting every match when color is on.
function emitLine(s: Searcher*, name: char*, lineNo: i64, line: Str.String*) => void {
    O.prefix(s.o, name, s.withName, lineNo, s.lineNumbers);
    if (s.o.color && !s.invert) {
        let pos: u64 = 0;
        let st: u64 = 0;
        let ln: u64 = 0;
        let go: bool = true;
        let piece: Str.String;
        while (go && M.findFrom(s.m, line, pos, &st, &ln)) {
            if (ln == 0) {
                go = false;                              // empty match: stop, avoid looping forever
            } else {
                if (st > pos) {
                    piece.clear();
                    line.substring(pos, st - pos, &piece);
                    s.o.buf.appendString(&piece);
                }
                O.colorStart(s.o, "01;31");
                piece.clear();
                line.substring(st, ln, &piece);
                s.o.buf.appendString(&piece);
                O.colorEnd(s.o);
                pos = st + ln;
            }
        }
        if (pos < line.len) {
            piece.clear();
            line.substring(pos, line.len - pos, &piece);
            s.o.buf.appendString(&piece);
        }
        piece.free();
    } else {
        s.o.buf.appendString(line);
    }
    O.endLine(s.o);
}

// Scans an open stream line by line.
function scanStream(s: Searcher*, f: IO.File*, path: char*) => void {
    let line: Str.String;
    let lineNo: i64 = 0;
    let matched: i64 = 0;
    let done: bool = false;
    while (!done && f.readLine(&line)) {
        lineNo = lineNo + 1;
        let hit: bool = M.matches(s.m, &line);
        if (s.invert) hit = !hit;
        if (hit) {
            matched = matched + 1;
            s.anyMatch = true;
            if (s.filesOnly) {
                done = true;                             // -l: one hit is enough
            } else if (!s.count) {
                emitLine(s, path, lineNo, &line);
            }
        }
    }
    line.free();
    if (s.filesOnly) {
        if (matched > 0) { s.o.buf.append(path); O.endLine(s.o); }
    } else if (s.count) {
        O.prefix(s.o, path, s.withName, 0, false);
        s.o.buf.appendInt(matched);
        O.endLine(s.o);
    }
}

function searchFile(s: Searcher*, path: char*) => void {
    let f: IO.File;
    if (!f.open(path, IO.MODE_READ)) {
        reportError(s, path, "cannot open file");
        return;
    }
    let n: u64 = f.readBytes(s.chunk.data as void*, CHUNK);
    if (hasNul(s.chunk.data, n)) {                       // binary file: skip
        f.close();
        return;
    }
    f.seek(0, IO.SEEK_SET);
    scanStream(s, &f, path);
    f.close();
}

function searchStdin(s: Searcher*) => void {
    let f: IO.File;
    f.attach(0);                                         // stdin: never close()
    scanStream(s, &f, "(standard input)");
}
