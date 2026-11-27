/// grep clone in OmniScript.
///   Build:  Osengine.exe grep\main.os --make -o grep.exe
///   Run:    grep.exe [OPTIONS] PATTERN [FILE...]
/// Run/build from the project root: import paths are relative to the launch folder.
/// Exit codes: 0 = a line was selected, 1 = none, 2 = error.
/// Modules: main (args) -> walk (-r) -> search (read + decide) -> matcher (engine) / output (printing).

module IO = import "standard/1/IO.os";
module Str = import "standard/1/String.os";
module Sys = import "standard/1/Sys.os";
module Fs = import "standard/1/Fs.os";
module L = import "standard/1/List.os";
module M = import "grep/matcher.os";
module O = import "grep/output.os";
module S = import "grep/search.os";
module W = import "grep/walk.os";

function eq(a: char*, b: char*) => bool {
    let i: u64 = 0;
    let ca: int = a[0] as int;
    let cb: int = b[0] as int;
    while (ca == cb && ca != 0) {
        i = i + 1;
        ca = a[i] as int;
        cb = b[i] as int;
    }
    return ca == cb;
}

function usage(o: O.Out*) => void {
    o.e.write("Usage: grep [OPTIONS] PATTERN [FILE...]\n");
    o.e.write("  -e PATTERN   use PATTERN (may start with '-')\n");
    o.e.write("  -i           ignore case (ASCII)\n");
    o.e.write("  -v           select non-matching lines\n");
    o.e.write("  -n           prefix lines with line number\n");
    o.e.write("  -c           print only a count of selected lines per file\n");
    o.e.write("  -l           print only names of files with a match\n");
    o.e.write("  -r, -R       search directories recursively (no FILE = current directory)\n");
    o.e.write("  -E           PATTERN is a regular expression (default: literal text)\n");
    o.e.write("  -F           PATTERN is literal text (default)\n");
    o.e.write("  -H / -h      always / never prefix lines with the file name\n");
    o.e.write("  --color      highlight matches (ANSI)\n");
    o.e.write("  --help       this text\n");
    o.e.write("Binary files (NUL in the first 8 KB) are skipped. No FILE = read standard input.\n");
}

function die(o: O.Out*, msg: char*) => void {
    O.error(o, nullptr, msg);
    Sys.exit(2);
}

function main(argc: int, argv: char**) => int {
    let out: O.Out;
    O.init(&out, false);

    let pattern: char* = nullptr;
    let havePattern: bool = false;
    let patFromE: bool = false;
    let icase: bool = false;
    let regex: bool = false;
    let invert: bool = false;
    let lineNums: bool = false;
    let count: bool = false;
    let filesOnly: bool = false;
    let recursive: bool = false;
    let forceName: bool = false;
    let noName: bool = false;
    let color: bool = false;
    let endOpts: bool = false;
    let files: L.List<char*>;

    let i: int = 1;
    while (i < argc) {
        let a: char* = argv[i];
        let isOpt: bool = !endOpts && (a[0] as int) == 45 && (a[1] as int) != 0;
        if (!isOpt) {
            if (!havePattern) { pattern = a; havePattern = true; }
            else { files.push(a); }
        } else if (eq(a, "--")) {
            endOpts = true;
        } else if (eq(a, "--color")) {
            color = true;
        } else if (eq(a, "--help")) {
            usage(&out);
            Sys.exit(0);
        } else if ((a[1] as int) == 45) {
            let msg: Str.String;
            msg.append("unrecognized option '");
            msg.append(a);
            msg.append("'");
            die(&out, msg.cstr());
        } else {
            let k: u64 = 1;                                  // short options, may be clustered: -inr
            while ((a[k] as int) != 0) {
                let ch: char = a[k];
                if (ch == 'i') icase = true;
                else if (ch == 'v') invert = true;
                else if (ch == 'n') lineNums = true;
                else if (ch == 'c') count = true;
                else if (ch == 'l') filesOnly = true;
                else if (ch == 'r' || ch == 'R') recursive = true;
                else if (ch == 'E') regex = true;
                else if (ch == 'F') regex = false;
                else if (ch == 'H') { forceName = true; noName = false; }
                else if (ch == 'h') { noName = true; forceName = false; }
                else if (ch == 'e') {
                    i = i + 1;
                    if (i >= argc) die(&out, "option requires an argument -- 'e'");
                    if (havePattern && !patFromE) files.push(pattern);   // an earlier positional was really a file
                    pattern = argv[i];
                    havePattern = true;
                    patFromE = true;
                } else {
                    let msg2: Str.String;
                    msg2.append("invalid option -- '");
                    msg2.appendChar(ch);
                    msg2.append("'");
                    die(&out, msg2.cstr());
                }
                k = k + 1;
            }
        }
        i = i + 1;
    }

    if (!havePattern) { usage(&out); Sys.exit(2); }
    out.color = color;

    let m: M.Matcher;
    if (!M.init(&m, pattern, regex, icase)) die(&out, "invalid regular expression");

    if (recursive && files.len == 0) files.push(".");

    let s: S.Searcher;
    S.init(&s);
    s.m = &m;
    s.o = &out;
    s.invert = invert;
    s.count = count;
    s.filesOnly = filesOnly;
    s.lineNumbers = lineNums;
    s.withName = !noName && (forceName || recursive || files.len > 1);

    if (files.len == 0) {
        S.searchStdin(&s);
    } else {
        let idx: u64 = 0;
        while (idx < files.len) {
            let p: char* = files.at(idx);
            if (Fs.isDirectory(p)) {
                if (recursive) W.walk(&s, p);
                else S.reportError(&s, p, "is a directory");
            } else {
                S.searchFile(&s, p);
            }
            idx = idx + 1;
        }
    }

    O.flush(&out);
    let code: int = 1;
    if (s.hadError) code = 2;
    else if (s.anyMatch) code = 0;

    S.destroy(&s);
    M.destroy(&m);
    O.destroy(&out);
    files.free();
    Sys.exit(code);                                          // also sets the exit code when run under JIT
    return code;
}
