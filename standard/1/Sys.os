// standard/1/Sys.os
// Tier 4 (early): environment, working directory, process exit, platform info.
// Strings come back through a String* you own (see String.os).
// args() is not here yet: it needs the engine to hand argc/argv to the script.

module Str = import "standard/1/String.os";
module COs = import "standard/1/c/os.os";

function platform() => char* { return "windows"; }
function arch() => char* { return "x86_64"; }
function cpuCount() => i64 { return COs.GetActiveProcessorCount(65535) as i64; }

function exit(code: int) => void { COs.exit(code); }

// Replaces out with the variable's value. false if it is not set.
function env(name: char*, out: Str.String*) => bool {
    let v = COs.getenv(name);
    if (v == nullptr) return false;
    return out.set(v);
}

function hasEnv(name: char*) => bool { return COs.getenv(name) != nullptr; }
function setEnv(name: char*, value: char*) => bool { return COs._putenv_s(name, value) == 0; }

function cwd(out: Str.String*) => bool {
    let buf: [1024]char;
    let p = COs._getcwd(&buf as char*, 1024);
    if (p == nullptr) return false;
    return out.set(&buf as char*);
}

function chdir(path: char*) => bool { return COs._chdir(path) == 0; }
