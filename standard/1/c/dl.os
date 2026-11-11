// standard/1/c/dl.os  (PLATFORM: Win32; POSIX: dlopen/dlsym/dlclose/dlerror)
extern "C" fn LoadLibraryA(path: char*) => void*;
extern "C" fn GetProcAddress(lib: void*, name: char*) => void*;
extern "C" fn FreeLibrary(lib: void*) => int;
extern "C" fn GetLastError() => u32;
