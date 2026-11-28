/// grep recursive directory walk (-r). Uses Fs.scan one directory at a time and searches each file as it goes,
/// so no big path list is held in memory. Symlinked directories are not followed (avoids cycles).
/// --include / --exclude filter files by base name, --exclude-dir skips folders by base name.
/// `bare` = the walk started from the implicit "." (-r with no FILE): names are printed without a "./" prefix.
/// NEXT STEPS: sorted output, skip .git, -R to follow links, depth limit.
/// ASSUMPTION: Fs.Entry.name is either a bare name or a full path; both are handled below.

module Str = import "standard/1/String.os";
module Fs = import "standard/1/Fs.os";
module L = import "standard/1/List.os";
module Path = import "standard/1/Path.os";
module S = import "grep/search.os";

function isDots(name: char*) => bool {
    if ((name[0] as int) != 46) return false;                       // '.'
    if ((name[1] as int) == 0) return true;                         // "."
    if ((name[1] as int) == 46 && (name[2] as int) == 0) return true;   // ".."
    return false;
}

function walk(s: S.Searcher*, dir: char*, bare: bool) => void {
    let es: L.List<Fs.Entry>;
    if (!Fs.scan(dir, &es)) {
        S.reportError(s, dir, "cannot read directory");
        return;
    }
    let i: u64 = 0;
    while (i < es.len) {
        let e: Fs.Entry* = es.ptr(i);
        let name: char* = e.name.cstr();
        if (!isDots(name)) {
            let full: Str.String;
            if (bare && (name[0] as int) == 46 && ((name[1] as int) == 47 || (name[1] as int) == 92)) {
                let q: u64 = 2;                                      // "./x" -> "x"
                while ((name[q] as int) != 0) { full.appendChar(name[q]); q = q + 1; }
            } else if (Path.lastSep(name) >= 0) {
                full.set(name);                                      // already a path
            } else if (bare) {
                full.set(name);
            } else {
                Path.join(dir, name, &full);
            }
            if (e.isDir) {
                if (!e.isLink && S.dirAllowed(s, name)) walk(s, full.cstr(), false);
            } else if (S.fileAllowed(s, full.cstr())) {
                S.searchFile(s, full.cstr());
            }
            full.free();
        }
        i = i + 1;
    }
    Fs.freeEntries(&es);
}
