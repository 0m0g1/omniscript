// standard/1/Cli.os
// Tier 4: argv parsing. Supports --flag, --key=value, --key value, -k, -abc (short flags), positionals, "--".
// Option specs are registered first (so --key value knows to take a value).
module Str = import "standard/1/String.os";
module L = import "standard/1/List.os";
module CStr = import "standard/1/c/string.os";

struct Opt {
    name: char* = nullptr;      // long name without dashes
    short: char = 0 as char;    // 0 = none
    takesValue: bool = false;
    help: char* = nullptr;
    present: bool = false;
    value: Str.String;
}

struct Cli {
    prog: char* = nullptr;
    opts: L.List<Opt>;
    positionals: L.List<Str.String>;

    free() => void {
        let i: u64 = 0;
        while (i < this.opts.len) { this.opts.data[i].value.free(); i = i + 1; }
        this.opts.free();
        i = 0;
        while (i < this.positionals.len) { this.positionals.data[i].free(); i = i + 1; }
        this.positionals.free();
    }

    flag(name: char*, short: char, help: char*) => bool {
        let o: Opt; o.name = name; o.short = short; o.help = help; o.takesValue = false;
        return this.opts.push(o);
    }
    option(name: char*, short: char, help: char*) => bool {
        let o: Opt; o.name = name; o.short = short; o.help = help; o.takesValue = true;
        return this.opts.push(o);
    }

    findLong(n: char*) => Opt* {
        let i: u64 = 0;
        while (i < this.opts.len) { if (CStr.strcmp(this.opts.data[i].name, n) == 0) return &this.opts.data[i]; i = i + 1; }
        return nullptr;
    }
    findShort(c: char) => Opt* {
        let i: u64 = 0;
        while (i < this.opts.len) { if (this.opts.data[i].short == c) return &this.opts.data[i]; i = i + 1; }
        return nullptr;
    }

    // false on unknown option or missing value (message printed to stderr by caller via errorMsg)
    parse(argc: int, argv: char**) => bool {
        this.prog = argv[0];
        let i: int = 1;
        let rest: bool = false;
        while (i < argc) {
            let a: char* = argv[i];
            i = i + 1;
            if (rest || (a[0] as int) != 45 || (a[1] as int) == 0) { let s: Str.String; s.set(a); this.positionals.push(s); continue; }
            if ((a[1] as int) == 45 && (a[2] as int) == 0) { rest = true; continue; }
            if ((a[1] as int) == 45) {
                let body: char* = (a as u64 + 2) as char*;
                let eq: char* = CStr.strchr(body, 61);
                let name: Str.String;
                if (eq == nullptr) name.set(body); else name.appendN(body, (eq as u64) - (body as u64));
                let o = this.findLong(name.cstr());
                name.free();
                if (o == nullptr) return false;
                o.present = true;
                if (o.takesValue) {
                    if (eq != nullptr) o.value.set((eq as u64 + 1) as char*);
                    else if (i < argc) { o.value.set(argv[i]); i = i + 1; }
                    else return false;
                }
            } else {
                let k: u64 = 1;
                while ((a[k] as int) != 0) {
                    let o = this.findShort(a[k]);
                    if (o == nullptr) return false;
                    o.present = true;
                    if (o.takesValue) {
                        if ((a[k + 1] as int) != 0) o.value.set((a as u64 + k + 1) as char*);
                        else if (i < argc) { o.value.set(argv[i]); i = i + 1; }
                        else return false;
                        break;
                    }
                    k = k + 1;
                }
            }
        }
        return true;
    }

    has(name: char*) => bool { let o = this.findLong(name); return o != nullptr && o.present; }
    get(name: char*, def: char*) => char* {
        let o = this.findLong(name);
        if (o == nullptr || !o.present) return def;
        return o.value.cstr();
    }

    usage(out: Str.String*) => void {
        out.append("Usage: "); out.append(this.prog); out.append(" [options]\n");
        let i: u64 = 0;
        while (i < this.opts.len) {
            let o: Opt* = &this.opts.data[i];
            out.append("  --"); out.append(o.name);
            if (o.takesValue) out.append(" <value>");
            out.append("  "); if (o.help != nullptr) out.append(o.help);
            out.append("\n");
            i = i + 1;
        }
    }
}
