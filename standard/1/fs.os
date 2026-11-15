// standard/1/Fs.os
// Tier 4: filesystem. exists/isFile/isDirectory, stat, directory listing and walking, create/remove/rename/copy,
// temp files. (PLATFORM: Win32 via c/fs.os; accepts / and \ in paths.)
//
//   if (Fs.isFile("a.txt")) { ... }
//   let st: Fs.Stat;  if (Fs.stat("a.txt", &st)) printf("%llu bytes\n", st.size);
//   let names: L.List<Str.String>;                   // listDir / walk give you newly allocated Strings:
//   Fs.listDir(".", &names);  ...  Text.freeParts(&names);   // free each, then the list
//
// Recursive helpers (walk, removeAll) do not follow symlinks/junctions. Functions return bool (false = failed).
// No `return` inside loop bodies (compiler quirk noted in Canvas.os).
module CF = import "standard/1/c/fs.os";
module CMem = import "standard/1/c/mem.os";
module CStr = import "standard/1/c/string.os";
module Str = import "standard/1/String.os";
module L = import "standard/1/List.os";
module P = import "standard/1/Path.os";

let INVALID_ATTR: uint = 0xFFFFFFFF;
let FILETIME_UNIX_EPOCH: u64 = 116444736000000000;

struct Stat {
    size: u64 = 0;
    modified: i64 = 0;        // seconds since 1970-01-01 UTC
    isDir: bool = false;
    isFile: bool = false;
}

function exists(path: char*) => bool { return CF.GetFileAttributesA(path) != INVALID_ATTR; }

function isDirectory(path: char*) => bool {
    let a: uint = CF.GetFileAttributesA(path);
    return a != INVALID_ATTR && (a & 0x10) != 0;
}

function isFile(path: char*) => bool {
    let a: uint = CF.GetFileAttributesA(path);
    return a != INVALID_ATTR && (a & 0x10) == 0;
}

function stat(path: char*, out: Stat*) => bool {
    let d: [40]u8;
    CMem.memset(&d as void*, 0, 40);
    if (CF.GetFileAttributesExA(path, 0, &d as void*) == 0) return false;
    let a: u32 = 0;
    let w: u64 = 0;
    let hi: u32 = 0;
    let lo: u32 = 0;
    CMem.memcpy(&a as void*, &d[0] as void*, 4);
    CMem.memcpy(&w as void*, &d[20] as void*, 8);
    CMem.memcpy(&hi as void*, &d[28] as void*, 4);
    CMem.memcpy(&lo as void*, &d[32] as void*, 4);
    out.isDir = (a & 0x10) != 0;
    out.isFile = !out.isDir;
    out.size = ((hi as u64) << 32) | (lo as u64);
    out.modified = 0;
    if (w >= FILETIME_UNIX_EPOCH) out.modified = ((w - FILETIME_UNIX_EPOCH) / 10000000) as i64;
    return true;
}

// File size in bytes (false if the path does not exist or is a directory).
function fileSize(path: char*, out: u64*) => bool {
    let st: Stat;
    if (!stat(path, &st)) return false;
    if (st.isDir) return false;
    out[0] = st.size;
    return true;
}

// ---- create / remove / move / copy ----
function createDir(path: char*) => bool { return CF.CreateDirectoryA(path, nullptr) != 0; }

// Creates every missing parent too. True if the directory exists afterwards.
function createDirAll(path: char*) => bool {
    let s: Str.String;
    s.set(path);
    let i: u64 = 1;
    while (i < s.len) {
        let c: int = s.data[i] as int;
        if (c == 47 || c == 92) {
            s.data[i] = 0 as char;
            if (!exists(s.data)) CF.CreateDirectoryA(s.data, nullptr);
            s.data[i] = c as char;
        }
        i = i + 1;
    }
    s.free();
    if (!isDirectory(path)) CF.CreateDirectoryA(path, nullptr);
    return isDirectory(path);
}

function removeFile(path: char*) => bool { return CF.DeleteFileA(path) != 0; }
function removeDir(path: char*) => bool { return CF.RemoveDirectoryA(path) != 0; }      // must be empty

// Replaces dst if it exists.
function rename(src: char*, dst: char*) => bool { return CF.MoveFileExA(src, dst, 1) != 0; }

function copy(src: char*, dst: char*, overwrite: bool) => bool {
    let failIfExists: int = 1;
    if (overwrite) failIfExists = 0;
    return CF.CopyFileA(src, dst, failIfExists) != 0;
}

// ---- directory listing ----
struct Entry {
    name: Str.String;
    isDir: bool = false;
    isLink: bool = false;     // reparse point (symlink / junction)
}

function freeEntries(es: L.List<Entry>*) => void {
    let i: u64 = 0;
    while (i < es.len) { es.data[i].name.free(); i = i + 1; }
    es.free();
}

// Entries of one directory (without "." and ".."). Free with freeEntries.
function scan(path: char*, out: L.List<Entry>*) => bool {
    let pat: Str.String;
    pat.set(path);
    if (pat.len > 0 && !P.isSep(pat.data[pat.len - 1] as int)) pat.appendChar(47 as char);
    pat.append("*");
    let fd: [320]u8;
    CMem.memset(&fd as void*, 0, 320);
    let h: void* = CF.FindFirstFileA(pat.cstr(), &fd as void*);
    pat.free();
    if ((h as u64) == 0xFFFFFFFFFFFFFFFF) return false;
    let more: bool = true;
    while (more) {
        let name: char* = (&fd[44]) as char*;
        if (CStr.strcmp(name, ".") != 0 && CStr.strcmp(name, "..") != 0) {
            let e: Entry;
            let a: u32 = 0;
            CMem.memcpy(&a as void*, &fd[0] as void*, 4);
            e.name.set(name);
            e.isDir = (a & 0x10) != 0;
            e.isLink = (a & 0x400) != 0;
            out.push(e);
        }
        more = CF.FindNextFileA(h, &fd as void*) != 0;
    }
    CF.FindClose(h);
    return true;
}

// Names (not paths) of everything in a directory. Free: Text.freeParts(&out).
function listDir(path: char*, out: L.List<Str.String>*) => bool {
    let es: L.List<Entry>;
    if (!scan(path, &es)) return false;
    let i: u64 = 0;
    while (i < es.len) { out.push(es.data[i].name); i = i + 1; }     // ownership of each name moves to `out`
    es.free();                                                        // buffer only
    return true;
}

// Every file under path (recursively), as "path/sub/file" Strings. Directories themselves are not listed.
// Free: Text.freeParts(&out).
function walk(path: char*, out: L.List<Str.String>*) => bool {
    let es: L.List<Entry>;
    if (!scan(path, &es)) return false;
    let i: u64 = 0;
    while (i < es.len) {
        let full: Str.String;
        P.join(path, es.data[i].name.cstr(), &full);
        if (es.data[i].isDir) {
            if (!es.data[i].isLink) walk(full.cstr(), out);
            full.free();
        } else {
            out.push(full);                                           // ownership moves to `out`
        }
        i = i + 1;
    }
    freeEntries(&es);
    return true;
}

// Deletes a file, or a directory with everything inside it. True if nothing is left (or it never existed).
function removeAll(path: char*) => bool {
    if (!exists(path)) return true;
    if (!isDirectory(path)) return removeFile(path);
    let es: L.List<Entry>;
    scan(path, &es);
    let i: u64 = 0;
    while (i < es.len) {
        let full: Str.String;
        P.join(path, es.data[i].name.cstr(), &full);
        if (es.data[i].isDir) {
            if (es.data[i].isLink) CF.RemoveDirectoryA(full.cstr());
            else removeAll(full.cstr());
        } else {
            CF.DeleteFileA(full.cstr());
        }
        full.free();
        i = i + 1;
    }
    freeEntries(&es);
    return removeDir(path);
}

// Creates an empty uniquely named file in the temp directory and stores its path in out.
function tempFile(out: Str.String*) => bool {
    let dir: [264]char;
    let name: [264]char;
    if (CF.GetTempPathA(260, &dir as char*) == 0) return false;
    if (CF.GetTempFileNameA(&dir as char*, "os", 0, &name as char*) == 0) return false;
    return out.set(&name as char*);
}
