// standard/1/c/fs.os
// Tier 0: raw Win32 file-system bindings (kernel32). (PLATFORM: Win32)
// This replaces the old top-level standard/1/fs.os. Windows file systems are case-insensitive, so a file called
// fs.os next to Fs.os would shadow the real Fs module: delete standard/1/fs.os when you add this file.
// WIN32_FIND_DATAA is 320 bytes: dwFileAttributes at offset 0 (u32), cFileName at offset 44.
// WIN32_FILE_ATTRIBUTE_DATA is 36 bytes: attrs @0, creation @4, access @12, last write @20 (FILETIME, u64), sizeHigh @28, sizeLow @32.

extern "C" fn GetFileAttributesA(path: char*) => uint;                       // 0xFFFFFFFF = does not exist
extern "C" fn GetFileAttributesExA(path: char*, level: int, data: void*) => int;   // level 0 = GetFileExInfoStandard
extern "C" fn FindFirstFileA(pattern: char*, data: void*) => void*;          // (void*)-1 = failure
extern "C" fn FindNextFileA(h: void*, data: void*) => int;
extern "C" fn FindClose(h: void*) => int;
extern "C" fn CopyFileA(src: char*, dst: char*, failIfExists: int) => int;
extern "C" fn MoveFileExA(src: char*, dst: char*, flags: uint) => int;       // 1 = MOVEFILE_REPLACE_EXISTING
extern "C" fn DeleteFileA(path: char*) => int;
extern "C" fn CreateDirectoryA(path: char*, security: void*) => int;
extern "C" fn RemoveDirectoryA(path: char*) => int;
extern "C" fn GetTempPathA(size: uint, buf: char*) => uint;
extern "C" fn GetTempFileNameA(dir: char*, prefix: char*, unique: uint, out: char*) => uint;   // creates the file

const INVALID_FILE_ATTRIBUTES: uint = 0xFFFFFFFF;
const FILE_ATTRIBUTE_DIRECTORY: uint = 0x10;
const FILE_ATTRIBUTE_REPARSE_POINT: uint = 0x400;
