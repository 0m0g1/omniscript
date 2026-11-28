/// grep clone in OmniScript.
///   Build:  Osengine.exe grep\main.os --make -o grep.exe
///   Run:    grep.exe [OPTIONS] PATTERN [FILE...]
/// Run/build from the project root: import paths are relative to the launch folder.
/// Exit codes: 0 = a line was selected, 1 = none, 2 = error.
/// Modules: main (args) -> walk (-r) -> search (read + decide) -> matcher (engine) / output (printing).
/// Script arguments reach main() through Osengine's `Config::scriptArgs` (everything after the script path).

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
    o.e.write("       grep [OPTIONS] -e PATTERN ... [-f FILE ...] [FILE...]\n");
    o.e.write("Patterns:\n");
    o.e.write("  -e PATTERN   use PATTERN (may start with '-'; may be given several times)\n");
    o.e.write("  -f FILE      read patterns from FILE, one per line ('-' = standard input)\n");
    o.e.write("  -E           PATTERNs are regular expressions (default: literal text)\n");
    o.e.write("  -F           PATTERNs are literal text (default)\n");
    o.e.write("  -i, -y       ignore case (ASCII)\n");
    o.e.write("  -w           match whole words only\n");
    o.e.write("  -x           match whole lines only\n");
    o.e.write("  -v           select non-matching lines\n");
    o.e.write("Output:\n");
    o.e.write("  -c           print only a count of selected lines per file\n");
    o.e.write("  -l / -L      print only names of files with / without a match\n");
    o.e.write("  -o           print only the matching part of a line\n");
    o.e.write("  -q           quiet: print nothing, exit 0 at the first match\n");
    o.e.write("  -s           hide messages about unreadable files\n");
    o.e.write("  -m NUM       stop after NUM selected lines per file\n");
    o.e.write("  -n           prefix lines with line number\n");
    o.e.write("  -H / -h      always / never prefix lines with the file name\n");
    o.e.write("  -A NUM, -B NUM, -C NUM   print NUM lines of context after / before / around matches\n");
    o.e.write("  --color      highlight matches (ANSI)\n");
    o.e.write("Files:\n");
    o.e.write("  -r, -R       search directories recursively (no FILE = current directory)\n");
    o.e.write("  --include=GLOB, --exclude=GLOB, --exclude-dir=GLOB   filter files / folders by base name\n");
    o.e.write("  -a           treat binary files as text\n");
    o.e.write("Long forms: --ignore-case --invert-match --line-number --count --files-with-matches\n");
    o.e.write("  --files-without-match --regexp= --file= --extended-regexp --fixed-strings --with-filename\n");
    o.e.write("  --no-filename --recursive --word-regexp --line-regexp --only-matching --quiet --silent\n");
    o.e.write("  --no-messages --max-count= --after-context= --before-context= --context= --text\n");
    o.e.write("  --version  --help\n");
    o.e.write("Binary files (NUL in the first 8 KB) are skipped. No FILE = read standard input.\n");
}

function die(o: O.Out*, msg: char*) => void {
    O.error(o, nullptr, msg);
    Sys.exit(2);
}

// Non-negative decimal number; false if the text is empty or has anything else in it.
function parseNum(s: char*, out: i64*) => bool {
    if ((s[0] as int) == 0) return false;
    let i: u64 = 0;
    let v: i64 = 0;
    let ok: bool = true;
    while (ok && (s[i] as int) != 0) {
        let c: int = s[i] as int;
        if (c < 48 || c > 57) {
            ok = false;
        } else {
            v = v * 10 + ((c - 48) as i64);
        }
        i = i + 1;
    }
    if (ok) out[0] = v;
    return ok;
}

// Every command-line setting that is not a file name.
struct Opts {
    icase: bool = false;
    invert: bool = false;
    lineNums: bool = false;
    count: bool = false;
    filesOnly: bool = false;
    listMissing: bool = false;
    recursive: bool = false;
    forceName: bool = false;
    noName: bool = false;
    color: bool = false;
    regex: bool = false;
    word: bool = false;
    whole: bool = false;
    onlyMatching: bool = false;
    quiet: bool = false;
    silent: bool = false;
    textMode: bool = false;
    hasMax: bool = false;
    maxCount: i64 = 0;
    after: i64 = 0;
    before: i64 = 0;
    ctxGiven: bool = false;
    patFromE: bool = false;          // patterns came from -e / -f (so the first plain argument is a FILE)
    npats: u64 = 0;
    pats: Str.String;                // all patterns, '\n' separated
    inc: Str.String;                 // --include globs
    exc: Str.String;                 // --exclude globs
    exd: Str.String;                 // --exclude-dir globs
}

function addPattern(o: Opts*, text: char*) => void {
    if (o.npats > 0) o.pats.appendChar(10 as char);
    let i: u64 = 0;
    while ((text[i] as int) != 0) { o.pats.appendChar(text[i]); i = i + 1; }
    o.npats = o.npats + 1;
}

function addPatternStr(o: Opts*, text: Str.String*, n: u64) => void {
    if (o.npats > 0) o.pats.appendChar(10 as char);
    let i: u64 = 0;
    while (i < n) { o.pats.appendChar(text.data[i]); i = i + 1; }
    o.npats = o.npats + 1;
}

// -f FILE: one pattern per line ("-" = standard input).
function readPatterns(o: Opts*, path: char*, out: O.Out*) => void {
    let f: IO.File;
    let fromStdin: bool = eq(path, "-");
    if (fromStdin) {
        f.attach(0);
    } else if (!f.open(path, IO.MODE_READ)) {
        O.error(out, path, "No such file or directory");
        Sys.exit(2);
    }
    let line: Str.String;
    while (f.readLine(&line)) {
        let n: u64 = line.len;
        if (n > 0 && (line.data[n - 1] as int) == 13) n = n - 1;       // CRLF pattern files
        addPatternStr(o, &line, n);
    }
    line.free();
    if (!fromStdin) f.close();
}

function takesValue(ch: char) => bool {
    return ch == 'e' || ch == 'f' || ch == 'A' || ch == 'B' || ch == 'C' || ch == 'm';
}

// The value of an option that takes one: the rest of the cluster ("-A2") or the next argument ("-A 2").
// false = there was no next argument.
function takeValue(argc: int, argv: char**, i: int*, a: char*, k: u64, v: Str.String*) => bool {
    v.clear();
    if ((a[k + 1] as int) != 0) {
        let j: u64 = k + 1;
        while ((a[j] as int) != 0) { v.appendChar(a[j]); j = j + 1; }
        return true;
    }
    i[0] = i[0] + 1;
    if (i[0] >= argc) return false;
    v.append(argv[i[0]]);
    return true;
}

// Flags without a value. false = not a known option letter.
function setFlag(o: Opts*, ch: char) => bool {
    if (ch == 'i' || ch == 'y') o.icase = true;
    else if (ch == 'v') o.invert = true;
    else if (ch == 'n') o.lineNums = true;
    else if (ch == 'c') o.count = true;
    else if (ch == 'l') o.filesOnly = true;
    else if (ch == 'L') o.listMissing = true;
    else if (ch == 'r' || ch == 'R') o.recursive = true;
    else if (ch == 'E') o.regex = true;
    else if (ch == 'F') o.regex = false;
    else if (ch == 'w') o.word = true;
    else if (ch == 'x') o.whole = true;
    else if (ch == 'o') o.onlyMatching = true;
    else if (ch == 'q') o.quiet = true;
    else if (ch == 's') o.silent = true;
    else if (ch == 'a') o.textMode = true;
    else if (ch == 'I') o.textMode = false;
    else if (ch == 'H') { o.forceName = true; o.noName = false; }
    else if (ch == 'h') { o.noName = true; o.forceName = false; }
    else return false;
    return true;
}

// Options with a value. 'N' = --include, 'X' = --exclude, 'D' = --exclude-dir (long options only).
function applyValue(o: Opts*, ch: char, v: Str.String*, out: O.Out*) => void {
    let n: i64 = 0;
    if (ch == 'e') {
        addPatternStr(o, v, v.len);
        o.patFromE = true;
    } else if (ch == 'f') {
        o.patFromE = true;
        if (v.len == 0) die(out, "option requires a file name -- 'f'");
        readPatterns(o, v.cstr(), out);
    } else if (ch == 'N' || ch == 'X' || ch == 'D') {
        if (v.len > 0) {
            if (ch == 'N') S.addGlob(&o.inc, v.cstr());
            else if (ch == 'X') S.addGlob(&o.exc, v.cstr());
            else S.addGlob(&o.exd, v.cstr());
        }
    } else {
        if (v.len == 0) die(out, "invalid argument");
        if (ch == 'm') {
            if (!parseNum(v.cstr(), &n)) die(out, "invalid max count");
            o.hasMax = true;
            o.maxCount = n;
        } else {
            if (!parseNum(v.cstr(), &n)) die(out, "invalid context length argument");
            o.ctxGiven = true;
            if (ch == 'A' || ch == 'C') o.after = n;
            if (ch == 'B' || ch == 'C') o.before = n;
        }
    }
}

// Long option name (without "--") -> the equivalent short letter; '?' = not a flag.
function longFlag(nm: char*) => char {
    if (eq(nm, "ignore-case")) return 'i';
    if (eq(nm, "invert-match")) return 'v';
    if (eq(nm, "line-number")) return 'n';
    if (eq(nm, "count")) return 'c';
    if (eq(nm, "files-with-matches")) return 'l';
    if (eq(nm, "files-without-match")) return 'L';
    if (eq(nm, "extended-regexp")) return 'E';
    if (eq(nm, "fixed-strings")) return 'F';
    if (eq(nm, "with-filename")) return 'H';
    if (eq(nm, "no-filename")) return 'h';
    if (eq(nm, "recursive")) return 'r';
    if (eq(nm, "dereference-recursive")) return 'R';
    if (eq(nm, "word-regexp")) return 'w';
    if (eq(nm, "line-regexp")) return 'x';
    if (eq(nm, "only-matching")) return 'o';
    if (eq(nm, "quiet")) return 'q';
    if (eq(nm, "silent")) return 'q';
    if (eq(nm, "no-messages")) return 's';
    if (eq(nm, "text")) return 'a';
    return '?';
}

// Long option name -> short letter of an option that takes a value; '?' = not one.
function longValue(nm: char*) => char {
    if (eq(nm, "regexp")) return 'e';
    if (eq(nm, "file")) return 'f';
    if (eq(nm, "max-count")) return 'm';
    if (eq(nm, "after-context")) return 'A';
    if (eq(nm, "before-context")) return 'B';
    if (eq(nm, "context")) return 'C';
    if (eq(nm, "include")) return 'N';
    if (eq(nm, "exclude")) return 'X';
    if (eq(nm, "exclude-dir")) return 'D';
    return '?';
}

function main(argc: int, argv: char**) => int {
    let out: O.Out;
    O.init(&out, false);

    let op: Opts;
    let files: L.List<char*>;
    let endOpts: bool = false;
    let argName: Str.String;
    let argVal: Str.String;

    let i: int = 1;
    while (i < argc) {
        let a: char* = argv[i];
        let isOpt: bool = !endOpts && (a[0] as int) == 45 && (a[1] as int) != 0;
        if (!isOpt) {
            files.push(a);                                   // the first one is the pattern unless -e / -f gave some
        } else if (eq(a, "--")) {
            endOpts = true;
        } else if ((a[1] as int) == 45) {
            // ---- long option: --name or --name=value
            argName.clear();
            argVal.clear();
            let hasVal: bool = false;
            let j: u64 = 2;
            while ((a[j] as int) != 0 && (a[j] as int) != 61) { argName.appendChar(a[j]); j = j + 1; }
            if ((a[j] as int) == 61) {
                hasVal = true;
                j = j + 1;
                while ((a[j] as int) != 0) { argVal.appendChar(a[j]); j = j + 1; }
            }
            let known: bool = false;
            if (argName.len > 0) {
                let nm: char* = argName.cstr();
                let fc: char = longFlag(nm);
                let vc: char = longValue(nm);
                if (eq(nm, "help")) {
                    usage(&out);
                    Sys.exit(0);
                } else if (eq(nm, "version")) {
                    out.f.write("grep (OmniScript) 1.1\n");
                    out.f.flush();
                    Sys.exit(0);
                } else if (eq(nm, "color") || eq(nm, "colour")) {
                    known = true;
                    op.color = true;
                    if (hasVal && argVal.len > 0 && eq(argVal.cstr(), "never")) op.color = false;
                } else if (fc != '?') {
                    known = true;
                    if (hasVal) die(&out, "option doesn't allow an argument");
                    setFlag(&op, fc);
                } else if (vc != '?') {
                    known = true;
                    if (!hasVal) {
                        i = i + 1;
                        if (i >= argc) die(&out, "option requires an argument");
                        argVal.append(argv[i]);
                    }
                    applyValue(&op, vc, &argVal, &out);
                }
            }
            if (!known) {
                let msg: Str.String;
                msg.append("unrecognized option '");
                msg.append(a);
                msg.append("'");
                die(&out, msg.cstr());
            }
        } else {
            // ---- short options, may be clustered: -inr, -A2, -ne PATTERN
            let k: u64 = 1;
            let stop: bool = false;
            while (!stop && (a[k] as int) != 0) {
                let ch: char = a[k];
                if (takesValue(ch)) {
                    if (!takeValue(argc, argv, &i, a, k, &argVal)) {
                        let msg3: Str.String;
                        msg3.append("option requires an argument -- '");
                        msg3.appendChar(ch);
                        msg3.append("'");
                        die(&out, msg3.cstr());
                    }
                    applyValue(&op, ch, &argVal, &out);
                    stop = true;                             // the rest of the cluster was the value
                } else if (ch == 'V') {
                    out.f.write("grep (OmniScript) 1.1\n");
                    out.f.flush();
                    Sys.exit(0);
                } else if (!setFlag(&op, ch)) {
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

    let fileStart: u64 = 0;
    if (!op.patFromE) {
        if (files.len == 0) { usage(&out); Sys.exit(2); }
        addPattern(&op, files.at(0));
        fileStart = 1;
    }
    out.color = op.color;

    let m: M.Matcher;
    if (!M.init(&m, op.pats.cstr(), op.npats, op.regex, op.icase, op.word, op.whole)) die(&out, "invalid regular expression");

    let nfiles: u64 = files.len - fileStart;

    let s: S.Searcher;
    S.init(&s);
    s.m = &m;
    s.o = &out;
    s.invert = op.invert;
    s.count = op.count;
    s.filesOnly = op.filesOnly;
    s.listMissing = op.listMissing;
    s.lineNumbers = op.lineNums;
    s.onlyMatching = op.onlyMatching;
    s.quiet = op.quiet;
    s.silent = op.silent;
    s.textMode = op.textMode;
    s.hasMax = op.hasMax;
    s.maxCount = op.maxCount;
    s.after = op.after;
    s.before = op.before;
    s.ctxOn = op.ctxGiven && !op.count && !op.filesOnly && !op.listMissing && !op.onlyMatching && !op.quiet;
    if (op.inc.len > 0) s.inc.set(op.inc.cstr());
    if (op.exc.len > 0) s.exc.set(op.exc.cstr());
    if (op.exd.len > 0) s.exd.set(op.exd.cstr());

    let anyDir: bool = false;
    if (op.recursive && nfiles == 1) anyDir = Fs.isDirectory(files.at(fileStart));
    s.withName = !op.noName && (op.forceName || nfiles > 1 || (op.recursive && (nfiles == 0 || anyDir)));

    if (nfiles == 0) {
        if (op.recursive) W.walk(&s, ".", true);             // -r with no FILE: search "." (names without "./")
        else S.searchStdin(&s);
    } else {
        let idx: u64 = fileStart;
        while (idx < files.len) {
            let p: char* = files.at(idx);
            if (eq(p, "-")) {
                S.searchStdin(&s);
            } else if (Fs.isDirectory(p)) {
                if (op.recursive) W.walk(&s, p, false);
                else S.reportError(&s, p, "Is a directory");
            } else if (S.fileAllowed(&s, p)) {
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
    op.pats.free();
    op.inc.free();
    op.exc.free();
    op.exd.free();
    argName.free();
    argVal.free();
    Sys.exit(code);                                          // also sets the exit code when run under JIT
    return code;
}
