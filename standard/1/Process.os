// standard/1/Process.os
// Tier 4: run commands, env vars, cwd, pid. (PLATFORM: MSVCRT _popen/_pclose; POSIX: popen/pclose)
module Str = import "standard/1/String.os";

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
// TODO(spawn/wait/kill/pipes): CreateProcessA + CreatePipe + WaitForSingleObject + TerminateProcess (needs STARTUPINFO/PROCESS_INFORMATION structs).
