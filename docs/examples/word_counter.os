extern "C" {
    fn printf(fmt: char*, ...) => int;
    fn fopen(path: char*, mode: char*) => void*;
    fn fclose(f: void*) => int;
    fn fgetc(f: void*) => int;
}

struct Stats {
    lines: i32 = 0;
    words: i32 = 0;
    chars: i32 = 0;
    longestLine: i32 = 0;

    report(label: char* = "file") => void {
        printf("%s: %d lines, %d words, %d chars, longest line %d\n",
               label, this.lines, this.words, this.chars, this.longestLine);
    }
}

function maxOf<T extends i32>(a: T, b: T) => T {
    return (a > b) ? a : b;
}

function analyze(path: char*) => Stats? {
    let f = fopen(path, "r");
    if (f == nullptr) {
        return null;
    }

    let s = Stats{};
    let inWord = false;
    let lineLen = 0;

    let c = fgetc(f);
    while (c != -1) {
        s.chars++;
        lineLen++;
        if (c == 10) {
            s.lines++;
            s.longestLine = maxOf<i32>(s.longestLine, lineLen - 1);
            lineLen = 0;
        }
        let isSpace = c == 32 || c == 10 || c == 9 || c == 13;
        if (isSpace) {
            inWord = false;
        } else if (!inWord) {
            inWord = true;
            s.words++;
        }
        c = fgetc(f);
    }

    fclose(f);
    return s;
}

function main(argc: int, argv: char**) => i32 {
    let path: char* = argc > 1 ? argv[1] : "README.md";
    let result = analyze(path);
    if (result == null) {
        printf("cannot open %s\n", path);
        return 1;
    }
    result.report(path);
    return 0;
}