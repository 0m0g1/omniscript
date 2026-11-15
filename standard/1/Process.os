// standard/1/Process.os
// Tier 4: run commands, env vars, cwd, pid, and child processes with pipes (spawn / wait / kill).
// (PLATFORM: MSVCRT _popen/_pclose + Win32 CreateProcess; POSIX: popen/pclose, fork/exec/pipe)
// No `return` inside loop bodies (compiler quirk noted in Canvas.os).
module Str = import "standard/1/String.os";
module CMem = import "standard/1/c/mem.os";
module CStr = import "standard/1/c/string.os";

extern "C" fn _popen(cmd: char*, mode: char*) => void*;
extern "C" fn _pclose(f: void*) => int;
extern "C" fn system(cmd: char*) => int;
extern "C" fn fgets(buf: char*, n: int, f: void*) => char*;
extern "C" fn getenv(name: char*) => char*;
extern "C" fn _putenv_s(name: char*, val: char*) => int;
extern "C" fn _getcwd(buf: char*, n: int) => char*;
extern "C" fn _chdir(path: char*) => int;
extern "C" fn _getpid() => int;
extern "C" fn exit(code: int) => void;

// Runs cmd through the shell, returns exit code (-1 on failure to start).
function run(cmd: char*) => int { return system(cmd); }

// Runs cmd and captures stdout into out. Returns exit code or -1.
function capture(cmd: char*, out: Str.String*) => int {
    let f = _popen(cmd, "r");
    if (f == nullptr) return -1;
    let buf: [512]char;
    while (fgets(&buf as char*, 512, f) != nullptr) out.append(&buf as char*);
    return _pclose(f);
}

function getEnv(name: char*) => char* { return getenv(name); }       // nullptr if unset
function setEnv(name: char*, val: char*) => bool { return _putenv_s(name, val) == 0; }
function pid() => int { return _getpid(); }
function quit(code: int) => void { exit(code); }

function cwd(out: Str.String*) => bool {
    let buf: [1024]char;
    if (_getcwd(&buf as char*, 1024) == nullptr) return false;
    return out.set(&buf as char*);
}
function chdir(path: char*) => bool { return _chdir(path) == 0; }

// ---------------------------------------------------------------------------------------------------------------
// Child processes (Win32)
//
//   let ch: Process.Child;
//   if (Process.spawn("git status", Process.SPAWN_STDOUT, &ch)) {     // stdout + stderr come back through the pipe
//       let out: Str.String;  ch.readAll(&out);  let code: int = ch.wait();  ch.close();  out.free();
//   }
//   let args: [3]char*;  args[0] = "-c"; args[1] = "print(1)"; args[2] = nullptr;
//   Process.spawnArgs("python", &args as char**, Process.SPAWN_STDIN | Process.SPAWN_STDOUT, &ch);   // quotes each arg
//   Process.runCapture("dir", &out) => exit code    (stdout and stderr merged)
//
// Deadlock warning: if you pipe both stdin and stdout, write all input (then closeStdin()) before the child fills the
// output pipe, or read from another thread. There is no timeout on reads; use waitMs + kill for runaway children.
// ---------------------------------------------------------------------------------------------------------------
extern "C" fn CreateProcessA(app: char*, cmd: char*, pa: void*, ta: void*, inherit: int, flags: uint, env: void*, cwd: char*, si: void*, pi: void*) => int;
extern "C" fn CreatePipe(rd: void**, wr: void**, sa: void*, size: uint) => int;
extern "C" fn SetHandleInformation(h: void*, mask: uint, flags: uint) => int;
extern "C" fn ReadFile(h: void*, buf: void*, n: uint, got: uint*, ov: void*) => int;
extern "C" fn WriteFile(h: void*, buf: void*, n: uint, written: uint*, ov: void*) => int;
extern "C" fn WaitForSingleObject(h: void*, ms: uint) => uint;
extern "C" fn GetExitCodeProcess(h: void*, code: uint*) => int;
extern "C" fn TerminateProcess(h: void*, code: uint) => int;
extern "C" fn CloseHandle(h: void*) => int;
extern "C" fn GetStdHandle(which: uint) => void*;

let SPAWN_STDIN: int = 1;         // parent gets a pipe to write the child's stdin
let SPAWN_STDOUT: int = 2;        // parent gets a pipe to read the child's stdout AND stderr
let SPAWN_NO_WINDOW: int = 4;     // do not create a console window (CREATE_NO_WINDOW)

struct Child {
    proc: void* = nullptr;
    stdinW: void* = nullptr;
    stdoutR: void* = nullptr;
    pid: u32 = 0;

    valid() => bool { return this.proc != nullptr; }

    // Bytes written, -1 on error (needs SPAWN_STDIN).
    write(p: u8*, n: u64) => i64 {
        if (this.stdinW == nullptr) return -1;
        let w: uint = 0;
        if (WriteFile(this.stdinW, p as void*, n as uint, &w, nullptr) == 0) return -1;
        return w as i64;
    }

    writeAll(p: u8*, n: u64) => bool {
        let off: u64 = 0;
        let ok: bool = true;
        while (ok && off < n) {
            let r: i64 = this.write(&p[off], n - off);
            if (r <= 0) ok = false;
            else off = off + (r as u64);
        }
        return ok;
    }

    writeStr(s: char*) => bool { return this.writeAll(s as u8*, CStr.strlen(s)); }

    // Signals end of input to the child.
    closeStdin() => void {
        if (this.stdinW != nullptr) { CloseHandle(this.stdinW); this.stdinW = nullptr; }
    }

    // Bytes read; 0 = the child closed its output (EOF), -1 = error / not piped.
    read(p: u8*, n: u64) => i64 {
        if (this.stdoutR == nullptr) return -1;
        let got: uint = 0;
        if (ReadFile(this.stdoutR, p as void*, n as uint, &got, nullptr) == 0) return 0;    // broken pipe = EOF
        return got as i64;
    }

    // Reads until EOF, appending to out.
    readAll(out: Str.String*) => bool {
        let buf: [4096]u8;
        let more: bool = true;
        let ok: bool = true;
        while (more) {
            let n: i64 = this.read(&buf as u8*, 4096);
            if (n > 0) ok = out.appendN(&buf as char*, n as u64);
            else more = false;
            if (!ok) more = false;
        }
        return ok;
    }

    // Blocks until the child exits, returns its exit code (-1 on error).
    wait() => int {
        if (this.proc == nullptr) return -1;
        WaitForSingleObject(this.proc, 0xFFFFFFFF);
        let code: uint = 0;
        if (GetExitCodeProcess(this.proc, &code) == 0) return -1;
        return code as int;
    }

    // True (and *code set) if the child exited within ms milliseconds.
    waitMs(ms: u32, code: int*) => bool {
        if (this.proc == nullptr) return false;
        if (WaitForSingleObject(this.proc, ms) != 0) return false;
        let c: uint = 0;
        GetExitCodeProcess(this.proc, &c);
        code[0] = c as int;
        return true;
    }

    kill() => bool {
        if (this.proc == nullptr) return false;
        return TerminateProcess(this.proc, 1) != 0;
    }

    // Releases the handles (does not kill the child).
    close() => void {
        this.closeStdin();
        if (this.stdoutR != nullptr) { CloseHandle(this.stdoutR); this.stdoutR = nullptr; }
        if (this.proc != nullptr) { CloseHandle(this.proc); this.proc = nullptr; }
    }
}

// Starts cmdline (e.g. "git status", or a full quoted command line). flags: SPAWN_* bits (0 = inherit everything).
function spawn(cmdline: char*, flags: int, out: Child*) => bool {
    let n: u64 = CStr.strlen(cmdline);
    let cmd = CMem.malloc(n + 1) as char*;                       // CreateProcess may modify the command line
    if (cmd == nullptr) return false;
    CMem.memcpy(cmd as void*, cmdline as void*, n + 1);

    let sa: [24]u8;                                             // SECURITY_ATTRIBUTES: nLength u32 @0, descriptor @8, bInheritHandle @16
    CMem.memset(&sa as void*, 0, 24);
    let nl: u32 = 24;
    let inh: u32 = 1;
    CMem.memcpy(&sa[0] as void*, &nl as void*, 4);
    CMem.memcpy(&sa[16] as void*, &inh as void*, 4);

    let si: [104]u8;                                            // STARTUPINFOA: cb @0, dwFlags @60, hStdInput @80, hStdOutput @88, hStdError @96
    CMem.memset(&si as void*, 0, 104);
    let pi: [24]u8;                                             // PROCESS_INFORMATION: hProcess @0, hThread @8, dwProcessId @16
    CMem.memset(&pi as void*, 0, 24);

    let hIn: void* = GetStdHandle(0xFFFFFFF6);                  // STD_INPUT_HANDLE  (-10)
    let hOut: void* = GetStdHandle(0xFFFFFFF5);                 // STD_OUTPUT_HANDLE (-11)
    let hErr: void* = GetStdHandle(0xFFFFFFF4);                 // STD_ERROR_HANDLE  (-12)
    let inR: void* = nullptr;
    let inW: void* = nullptr;
    let outR: void* = nullptr;
    let outW: void* = nullptr;
    let ok: bool = true;

    if ((flags & 1) != 0) {
        ok = CreatePipe(&inR, &inW, &sa as void*, 0) != 0;
        if (ok) { SetHandleInformation(inW, 1, 0); hIn = inR; }
    }
    if (ok && (flags & 2) != 0) {
        ok = CreatePipe(&outR, &outW, &sa as void*, 0) != 0;
        if (ok) { SetHandleInformation(outR, 1, 0); hOut = outW; hErr = outW; }
    }

    let started: bool = false;
    if (ok) {
        let cb: u32 = 104;
        let sf: u32 = 0x100;                                    // STARTF_USESTDHANDLES
        CMem.memcpy(&si[0] as void*, &cb as void*, 4);
        CMem.memcpy(&si[60] as void*, &sf as void*, 4);
        CMem.memcpy(&si[80] as void*, &hIn as void*, 8);
        CMem.memcpy(&si[88] as void*, &hOut as void*, 8);
        CMem.memcpy(&si[96] as void*, &hErr as void*, 8);
        let cflags: uint = 0;
        if ((flags & 4) != 0) cflags = 0x08000000;              // CREATE_NO_WINDOW
        started = CreateProcessA(nullptr, cmd, nullptr, nullptr, 1, cflags, nullptr, nullptr, &si as void*, &pi as void*) != 0;
    }
    CMem.free(cmd as void*);

    // the child owns its ends of the pipes now; the parent closes them
    if (inR != nullptr) CloseHandle(inR);
    if (outW != nullptr) CloseHandle(outW);

    if (!started) {
        if (inW != nullptr) CloseHandle(inW);
        if (outR != nullptr) CloseHandle(outR);
        return false;
    }
    let hp: void* = nullptr;
    let ht: void* = nullptr;
    let pid: u32 = 0;
    CMem.memcpy(&hp as void*, &pi[0] as void*, 8);
    CMem.memcpy(&ht as void*, &pi[8] as void*, 8);
    CMem.memcpy(&pid as void*, &pi[16] as void*, 4);
    CloseHandle(ht);
    out.proc = hp;
    out.stdinW = inW;
    out.stdoutR = outR;
    out.pid = pid;
    return true;
}

// Appends one argument to out using the Windows command-line quoting rules (backslashes and quotes escaped).
function quoteArg(s: char*, out: Str.String*) => bool {
    let needs: bool = (s[0] as int) == 0;
    let i: u64 = 0;
    while ((s[i] as int) != 0) {
        let c: int = s[i] as int;
        if (c == 32 || c == 9 || c == 10 || c == 11 || c == 34) needs = true;
        i = i + 1;
    }
    if (!needs) return out.append(s);
    out.appendChar(34 as char);
    i = 0;
    let done: bool = false;
    while (!done) {
        let bs: u64 = 0;
        while ((s[i] as int) == 92) { i = i + 1; bs = bs + 1; }
        let c: int = s[i] as int;
        let k: u64 = 0;
        if (c == 0) {
            while (k < bs * 2) { out.appendChar(92 as char); k = k + 1; }
            done = true;
        } else if (c == 34) {
            while (k < bs * 2 + 1) { out.appendChar(92 as char); k = k + 1; }
            out.appendChar(34 as char);
            i = i + 1;
        } else {
            while (k < bs) { out.appendChar(92 as char); k = k + 1; }
            out.appendChar(c as char);
            i = i + 1;
        }
    }
    return out.appendChar(34 as char);
}

// Like spawn, but builds the command line from an executable and a nullptr-terminated argument array (no shell quoting
// mistakes): args = {"-c", "print(1)", nullptr}. args may be nullptr for no arguments.
function spawnArgs(exe: char*, args: char**, flags: int, out: Child*) => bool {
    let line: Str.String;
    quoteArg(exe, &line);
    if (args != nullptr) {
        let i: u64 = 0;
        while (args[i] != nullptr) {
            line.appendChar(32 as char);
            quoteArg(args[i], &line);
            i = i + 1;
        }
    }
    let ok: bool = spawn(line.cstr(), flags, out);
    line.free();
    return ok;
}

// Runs cmdline, captures stdout + stderr into out, returns the exit code (-1 if it could not be started).
function runCapture(cmdline: char*, out: Str.String*) => int {
    let ch: Child;
    if (!spawn(cmdline, SPAWN_STDOUT | SPAWN_NO_WINDOW, &ch)) return -1;
    ch.readAll(out);
    let code: int = ch.wait();
    ch.close();
    return code;
}
