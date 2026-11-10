// standard/1/c/fs.os
// Tier 0: raw Win32 file-system bindings (attributes, directory enumeration, copy).
// WIN32_FIND_DATAA is 320 bytes; dwFileAttributes is at offset 0 (uint), cFileName at offset 44.

extern "C" fn GetFileAttributesA(path: char*) => uint;                  // 0xFFFFFFFF = does not exist
extern "C" fn FindFirstFileA(pattern: char*, data: void*) => void*;     // (void*)-1 = failure
extern "C" fn FindNextFileA(h: void*, data: void*) => int;
extern "C" fn FindClose(h: void*) => int;
extern "C" fn CopyFileA(src: char*, dst: char*, failIfExists: int) => int;

const INVALID_FILE_ATTRIBUTES: uint = 0xFFFFFFFF;
const FILE_ATTRIBUTE_DIRECTORY: uint = 0x10;
