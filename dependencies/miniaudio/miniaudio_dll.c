// dependencies/miniaudio/miniaudio_dll.c
// Builds miniaudio.h (same folder) into miniaudio.dll with every ma_* function exported.
//
//   MSVC (x64 Native Tools prompt):   cl /nologo /O2 /LD miniaudio_dll.c /Fe:miniaudio.dll
//   MinGW-w64:                        gcc -O2 -shared -o miniaudio.dll miniaudio_dll.c
//
// miniaudio loads the Windows audio APIs at runtime, so no extra libraries are needed on Windows.

#define MA_API __declspec(dllexport)
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
