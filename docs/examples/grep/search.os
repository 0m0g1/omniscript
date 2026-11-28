/// grep search: reads one file (or stdin) line by line and reports matches through output.os.
/// Binary files (a NUL byte in the first chunk) are skipped silently unless -a is given.
/// Memory: one line String and one 8 KB chunk are reused; files are never read whole.
/// Features here: -v -c -l -L -n -o -q -m, context lines (-A/-B/-C), --include / --exclude / --exclude-dir globs.
/// NEXT STEPS: "Binary file x matches" message, mmap/large-block scanning (search the chunk for the pattern first,
/// then split lines), multithreaded -r.

module IO = import "standard/1/IO.os";
module Str = import "standard/1/String.os";
module Sys = import "standard/1/Sys.os";
module M = import "grep/matcher.os";
module O = import "grep/output.os";

let CHUNK: u64 = 8192;

struct Searcher {
    m: M.Matcher* = nullptr;
    o: O.Out* = nullptr;
    invert: bool = false;            // -v
    count: bool = false;             // -c
    filesOnly: bool = false;         // -l
    listMissing: bool = false;       // -L: list files WITHOUT a selected line
    lineNumbers: bool = false;       // -n
    withName: bool = false;          // prefix lines with the file name
    onlyMatching: bool = false;      // -o
    quiet: bool = false;             // -q: exit 0 at the first selected line, print nothing
    silent: bool = false;            // -s: no messages about unreadable files
    textMode: bool = false;          // -a: search binary files as text
    hasMax: bool = false;            // -m NUM given
    maxCount: i64 = 0;
    after: i64 = 0;                  // -A
    before: i64 = 0;                 // -B
    ctxOn: bool = false;             // print context lines / "--" separators
    anyMatch: bool = false;          // at least one line selected somewhere (exit code 0)
    hadError: bool = false;          // some file could not be read (exit code 2)
    newFile: bool = true;            // nothing printed yet from the current file
    printedAny: bool = false;        // something was printed already (needed for the "--" separator)
    lastPrinted: i64 = 0;            // line number of the last printed line in the current file
    bcnt: i64 = 0;                   // lines waiting in bbuf
    inc: Str.String;                 // --include globs, '\n' separated
    exc: Str.String;                 // --exclude globs
    exd: Str.String;                 // --exclude-dir globs
    bbuf: Str.String;                // the last -B lines that were not printed, '\n' terminated
    chunk: Str.String;               // scratch buffer for the binary check
}

function init(s: Searcher*) => void { s.chunk.reserve(CHUNK); }

function destroy(s: Searcher*) => void {
    s.chunk.free();
    s.bbuf.free();
    s.inc.free();
    s.exc.free();
    s.exd.free();
}

function reportError(s: Searcher*, path: char*, msg: char*) => void {
    if (!s.silent) O.error(s.o, path, msg);
    s.hadError = true;
}

// ------------------------------------------------------------------ globs (--include / --exclude)

// [set] / [!set] / [^set] with ranges. p[start] is '['. Sets *next to the index after the set.
// An unterminated '[' is an ordinary character.
function classMatch(p: char*, start: u64, c: int, next: u64*) => bool {
    let i: u64 = start + 1;
    let neg: bool = false;
    if ((p[i] as int) == 33 || (p[i] as int) == 94) { neg = true; i = i + 1; }
    let matched: bool = false;
    let first: bool = true;
    while ((p[i] as int) != 0 && ((p[i] as int) != 93 || first)) {
        let lo: int = p[i] as int;
        let hi: int = lo;
        if ((p[i + 1] as int) == 45 && (p[i + 2] as int) != 0 && (p[i + 2] as int) != 93) {
            hi = p[i + 2] as int;
            i = i + 3;
        } else {
            i = i + 1;
        }
        if (c >= lo && c <= hi) matched = true;
        first = false;
    }
    if ((p[i] as int) == 0) {
        next[0] = start + 1;
        return c == 91;
    }
    next[0] = i + 1;
    if (neg) return !matched;
    return matched;
}

// Shell-style wildcard match: * ? [set] and \x escapes.
function globMatch(p: char*, t: char*) => bool {
    let pi: u64 = 0;
    let ti: u64 = 0;
    let starP: u64 = 0;
    let starT: u64 = 0;
    let haveStar: bool = false;
    let failed: bool = false;
    while (!failed && (t[ti] as int) != 0) {
        let pc: int = p[pi] as int;
        if (pc == 42) {                                   // '*': remember where to resume
            haveStar = true;
            starP = pi + 1;
            starT = ti;
            pi = pi + 1;
        } else {
            let ok: bool = false;
            let adv: u64 = 1;
            if (pc == 63) {
                ok = true;
            } else if (pc == 91) {
                let nx: u64 = 0;
                ok = classMatch(p, pi, t[ti] as int, &nx);
                adv = nx - pi;
            } else if (pc == 92 && (p[pi + 1] as int) != 0) {
                ok = (p[pi + 1] as int) == (t[ti] as int);
                adv = 2;
            } else if (pc != 0) {
                ok = pc == (t[ti] as int);
            }
            if (ok) {
                pi = pi + adv;
                ti = ti + 1;
            } else if (haveStar) {
                starT = starT + 1;
                ti = starT;
                pi = starP;
            } else {
                failed = true;
            }
        }
    }
    if (failed) return false;
    while ((p[pi] as int) == 42) { pi = pi + 1; }
    return (p[pi] as int) == 0;
}

// The part of a path after the last / or \.
function baseName(p: char*, out: Str.String*) => void {
    let i: u64 = 0;
    let b: u64 = 0;
    while ((p[i] as int) != 0) {
        let c: int = p[i] as int;
        if (c == 47 || c == 92) b = i + 1;
        i = i + 1;
    }
    out.clear();
    while ((p[b] as int) != 0) { out.appendChar(p[b]); b = b + 1; }
}

// Does the base name of `path` match any glob in the '\n'-separated list?
function globAny(list: Str.String*, path: char*) => bool {
    if (list.len == 0) return false;
    let bn: Str.String;
    baseName(path, &bn);
    let seg: Str.String;
    let i: u64 = 0;
    let hit: bool = false;
    while (!hit && i <= list.len) {
        seg.clear();
        while (i < list.len && (list.data[i] as int) != 10) { seg.appendChar(list.data[i]); i = i + 1; }
        if (seg.len > 0 && bn.len > 0 && globMatch(seg.cstr(), bn.cstr())) hit = true;
        i = i + 1;
    }
    bn.free();
    seg.free();
    return hit;
}

function addGlob(list: Str.String*, g: char*) => void {
    if (list.len > 0) list.appendChar(10 as char);
    list.append(g);
}

// May this file be searched (--include / --exclude)?
function fileAllowed(s: Searcher*, path: char*) => bool {
    if (globAny(&s.exc, path)) return false;
    if (s.inc.len > 0 && !globAny(&s.inc, path)) return false;
    return true;
}

// May the recursive walk enter this directory (--exclude-dir)?
function dirAllowed(s: Searcher*, path: char*) => bool {
    return !globAny(&s.exd, path);
}

// ------------------------------------------------------------------ output helpers

function hasNul(p: char*, n: u64) => bool {
    let i: u64 = 0;
    while (i < n) {
        if ((p[i] as int) == 0) return true;
        i = i + 1;
    }
    return false;
}

// Call once before every printed line: prints "--" between groups that are not adjacent (context mode only).
function startLine(s: Searcher*, lineNo: i64) => void {
    if (s.ctxOn && s.printedAny && (s.newFile || lineNo != s.lastPrinted + 1)) O.groupSep(s.o);
    s.newFile = false;
    s.lastPrinted = lineNo;
    s.printedAny = true;
}

// A context line (not selected): same as a selected line but with '-' separators and no highlighting.
function emitContext(s: Searcher*, name: char*, lineNo: i64, text: Str.String*) => void {
    startLine(s, lineNo);
    O.prefixSep(s.o, name, s.withName, lineNo, s.lineNumbers, '-');
    s.o.buf.appendString(text);
    O.endLine(s.o);
}

// Prints one selected line, highlighting every match when color is on. -o prints only the matches.
function emitLine(s: Searcher*, name: char*, lineNo: i64, line: Str.String*) => void {
    let piece: Str.String;
    let pos: u64 = 0;
    let st: u64 = 0;
    let ln: u64 = 0;
    let go: bool = true;
    if (s.onlyMatching) {
        if (!s.invert) {                                 // -o -v prints nothing, like real grep
            while (go && M.findFrom(s.m, line, pos, &st, &ln)) {
                if (ln == 0) {
                    go = false;                          // empty match: stop, avoid looping forever
                } else {
                    O.prefix(s.o, name, s.withName, lineNo, s.lineNumbers);
                    piece.clear();
                    line.substring(st, ln, &piece);
                    O.colorStart(s.o, "01;31");
                    s.o.buf.appendString(&piece);
                    O.colorEnd(s.o);
                    O.endLine(s.o);
                    pos = st + ln;
                }
            }
        }
        piece.free();
        return;
    }
    startLine(s, lineNo);
    O.prefix(s.o, name, s.withName, lineNo, s.lineNumbers);
    if (s.o.color && !s.invert) {
        while (go && M.findFrom(s.m, line, pos, &st, &ln)) {
            if (ln == 0) {
                go = false;
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
    } else {
        s.o.buf.appendString(line);
    }
    piece.free();
    O.endLine(s.o);
}

// -B: remember an unprinted line (keeps only the last `before` of them).
function pushBefore(s: Searcher*, line: Str.String*) => void {
    if (s.before == 0) return;
    s.bbuf.appendString(line);
    s.bbuf.appendChar(10 as char);
    s.bcnt = s.bcnt + 1;
    if (s.bcnt > s.before) {                             // drop the oldest one
        let k: u64 = 0;
        while (k < s.bbuf.len && (s.bbuf.data[k] as int) != 10) { k = k + 1; }
        let rest: Str.String;
        s.bbuf.substring(k + 1, s.bbuf.len - (k + 1), &rest);
        s.bbuf.set(rest.cstr());
        rest.free();
        s.bcnt = s.bcnt - 1;
    }
}

// -B: print the remembered lines just before the selected line `lineNo`.
function flushBefore(s: Searcher*, name: char*, lineNo: i64) => void {
    if (s.bcnt == 0) return;
    let n: i64 = lineNo - s.bcnt;
    let piece: Str.String;
    let i: u64 = 0;
    while (i < s.bbuf.len) {
        piece.clear();
        while (i < s.bbuf.len && (s.bbuf.data[i] as int) != 10) { piece.appendChar(s.bbuf.data[i]); i = i + 1; }
        emitContext(s, name, n, &piece);
        n = n + 1;
        i = i + 1;
    }
    piece.free();
    s.bbuf.clear();
    s.bcnt = 0;
}

// ------------------------------------------------------------------ scanning

// Scans an open stream line by line.
function scanStream(s: Searcher*, f: IO.File*, path: char*) => void {
    let line: Str.String;
    let lineNo: i64 = 0;
    let matched: i64 = 0;
    let done: bool = false;
    let capped: bool = false;            // -m reached: only trailing context is still printed
    let afterLeft: i64 = 0;
    s.newFile = true;
    s.bbuf.clear();
    s.bcnt = 0;
    if (s.hasMax && s.maxCount == 0) done = true;
    while (!done && f.readLine(&line)) {
        lineNo = lineNo + 1;
        let hit: bool = M.matches(s.m, &line);
        if (s.invert) hit = !hit;
        if (capped) {
            if (afterLeft > 0) {
                emitContext(s, path, lineNo, &line);
                afterLeft = afterLeft - 1;
            }
            if (afterLeft == 0) done = true;
        } else if (hit) {
            matched = matched + 1;
            if (!s.listMissing) s.anyMatch = true;
            if (s.quiet) {                               // -q: the first selected line decides everything
                O.flush(s.o);
                Sys.exit(0);
            }
            if (s.filesOnly || s.listMissing) {
                done = true;                             // -l / -L: one hit is enough
            } else if (!s.count) {
                if (s.ctxOn) flushBefore(s, path, lineNo);
                emitLine(s, path, lineNo, &line);
                afterLeft = s.after;
            }
            if (s.hasMax && matched >= s.maxCount) {
                capped = true;
                if (!s.ctxOn || afterLeft == 0) done = true;
            }
        } else if (s.ctxOn) {
            if (afterLeft > 0) {
                emitContext(s, path, lineNo, &line);
                afterLeft = afterLeft - 1;
            } else {
                pushBefore(s, &line);
            }
        }
    }
    line.free();
    if (!s.quiet) {
        if (s.filesOnly) {
            if (matched > 0) { s.o.buf.append(path); O.endLine(s.o); }
        } else if (s.listMissing) {
            if (matched == 0) {
                s.anyMatch = true;                       // -L succeeds when it lists a file
                s.o.buf.append(path);
                O.endLine(s.o);
            }
        } else if (s.count) {
            O.prefix(s.o, path, s.withName, 0, false);
            s.o.buf.appendInt(matched);
            O.endLine(s.o);
        }
    }
}

function searchFile(s: Searcher*, path: char*) => void {
    let f: IO.File;
    if (!f.open(path, IO.MODE_READ)) {
        reportError(s, path, "No such file or directory");
        return;
    }
    if (!s.textMode) {
        let n: u64 = f.readBytes(s.chunk.data as void*, CHUNK);
        if (hasNul(s.chunk.data, n)) {                   // binary file: skip
            f.close();
            return;
        }
        f.seek(0, IO.SEEK_SET);
    }
    scanStream(s, &f, path);
    f.close();
}

function searchStdin(s: Searcher*) => void {
    let f: IO.File;
    f.attach(0);                                         // stdin: never close()
    scanStream(s, &f, "(standard input)");
}
