// standard/1/c/os.os
// Tier 0: raw process/environment/directory bindings.
// PLATFORM: Windows (UCRT underscore names). On POSIX: setenv, getcwd, chdir, mkdir(path, mode), rmdir, access, sysconf.

extern "C" fn getenv(name: char*) => char*;
extern "C" fn _putenv_s(name: char*, value: char*) => int;
extern "C" fn exit(code: int) => void;
extern "C" fn abort() => void;
extern "C" fn system(cmd: char*) => int;
extern "C" fn _getcwd(buf: char*, size: int) => char*;
extern "C" fn _chdir(path: char*) => int;
extern "C" fn _mkdir(path: char*) => int;
extern "C" fn _rmdir(path: char*) => int;
extern "C" fn _access(path: char*, mode: int) => int;
extern "C" fn GetActiveProcessorCount(group: uint) => uint;             // 0xFFFF = all groups
