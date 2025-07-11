#include <windows.h>
#include <iostream>

int main() {
    HMODULE handle = LoadLibraryA("dependencies/glad/gl/bin/glad_gl.dll");
    if (!handle) {
        std::cerr << "Failed to load DLL: " << GetLastError() << std::endl;
        return 1;
    }
    FARPROC symbol = GetProcAddress(handle, "glad_glGetError");
    if (symbol) {
        std::cout << "glad_glGetError found in DLL\n";
    } else {
        std::cerr << "glad_glGetError not found: " << GetLastError() << std::endl;
    }
    FreeLibrary(handle);
    return 0;
}