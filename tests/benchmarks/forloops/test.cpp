// bench_cpp.cpp
// Compiled with
// g++ -O3 -o bench_cpp.exe test.cpp
#include <windows.h>
#include <cstdint>
#include <iostream>

using PerfCounterFn = int(__stdcall*)(int64_t*);

int main() {
    HMODULE kernel32 = LoadLibraryA("kernel32.dll");
    if (!kernel32) {
        std::cerr << "Failed to load kernel32.dll\n";
        return 1;
    }

    PerfCounterFn QueryPerformanceFrequency = (PerfCounterFn)GetProcAddress(kernel32, "QueryPerformanceFrequency");
    PerfCounterFn QueryPerformanceCounter = (PerfCounterFn)GetProcAddress(kernel32, "QueryPerformanceCounter");

    if (!QueryPerformanceFrequency || !QueryPerformanceCounter) {
        std::cerr << "Failed to get function pointers\n";
        return 1;
    }

    int64_t freq = 0;
    int64_t start = 0, end = 0;
    int64_t warmup = 0, warmupNoise = 0;

    QueryPerformanceFrequency(&freq);

    for (int64_t i = 0; i < 1000000; ++i) {
        if (i % 1000000001 == 0) {
            int64_t temp = 0;
            QueryPerformanceCounter(&temp);
            warmupNoise ^= temp;
        }
        warmup += i;
    }

    int64_t noise = 0;
    int64_t x = warmup ^ warmupNoise;

    QueryPerformanceCounter(&start);

    for (int64_t i = 0; i < 1000000000; ++i) {
        if (i % 1000000001 == 0) {
            int64_t temp = 0;
            QueryPerformanceCounter(&temp);
            noise ^= temp;
        }
        x += i;
    }

    QueryPerformanceCounter(&end);
    x ^= noise;

    double elapsedMs = (double)(end - start) * 1000.0 / freq;

    std::cout << "Result: " << x << "\n";
    std::cout << "Elapsed: " << elapsedMs << " ms\n";
    std::cout << "Ops/ms: " << (1000000.0 / elapsedMs) << "\n";

    FreeLibrary(kernel32);
    return 0;
}

// // Freestanding C++
// // Compiled with 
// // g++ -O3 test.cpp -ffreestanding -nostdlib -lkernel32 -lmsvcrt -luser32 "-Wl,--entry=_start" -o bench_cpp.exe
// typedef long long LONGLONG;
// typedef unsigned long DWORD;
// typedef void* HANDLE;
// typedef int BOOL;
// typedef void VOID;
// typedef unsigned int UINT;
// typedef char* LPSTR;

// typedef union _LARGE_INTEGER {
//     struct {
//         DWORD LowPart;
//         long HighPart;
//     };
//     LONGLONG QuadPart;
// } LARGE_INTEGER;

// extern "C" {
//     __declspec(dllimport) BOOL __stdcall QueryPerformanceCounter(LARGE_INTEGER*);
//     __declspec(dllimport) BOOL __stdcall QueryPerformanceFrequency(LARGE_INTEGER*);
//     __declspec(dllimport) VOID __stdcall ExitProcess(UINT);
//     __declspec(dllimport) BOOL __stdcall WriteConsoleA(HANDLE, const void*, DWORD, DWORD*, void*);
//     __declspec(dllimport) HANDLE __stdcall GetStdHandle(DWORD);
//     __declspec(dllimport) int __stdcall wsprintfA(LPSTR, const char*, ...);
// }

// #define STD_OUTPUT_HANDLE ((DWORD)-11)

// using int64_t = long long;

// extern "C" void _start() {
//     LARGE_INTEGER freq, start, end, temp;
//     QueryPerformanceFrequency(&freq);
    
//     // Warmup with same logic as your language
//     volatile int64_t warmup = 0;
//     volatile int64_t warmupNoise = 0;
    
//     for (int64_t i = 0; i < 1000000; ++i) {
//         if (i % 1000000001 == 0) { // Same rare pattern
//             QueryPerformanceCounter(&temp);
//             warmupNoise ^= temp.QuadPart;
//         }
//         warmup += i;
//     }
    
//     // Main benchmark
//     volatile int64_t noise = 0;
//     volatile int64_t x = warmup ^ warmupNoise;
    
//     QueryPerformanceCounter(&start);
    
//     for (int64_t i = 0; i < 1000000000; ++i) { // Same 1 billion iterations
//         if (i % 1000000001 == 0) { // Same rare pattern
//             QueryPerformanceCounter(&temp);
//             noise ^= temp.QuadPart;
//         }
//         x += i;
//     }
    
//     QueryPerformanceCounter(&end);
    
//     // Combine noise to prevent dead-code removal
//     x ^= noise;
    
//     double elapsed = (double)(end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
    
//     // Compute ops/ms - corrected calculation
//     double ops_per_ms = 1000000000.0 / elapsed; // 1B iterations divided by elapsed time in ms
    
//     // Format and write output using integer arithmetic (wsprintfA doesn't do floats)
//     char buffer[256];
//     DWORD written;
    
//     // Convert elapsed to integer with 4 decimal places
//     int elapsed_int = (int)elapsed;
//     int elapsed_frac = (int)((elapsed - elapsed_int) * 10000);
    
//     // Convert ops/ms to integer with 1 decimal place
//     int ops_int = (int)ops_per_ms;
//     int ops_frac = (int)((ops_per_ms - ops_int) * 10);
    
//     int len = wsprintfA(buffer,
//         "Result: %I64d\nElapsed: %d.%04d ms\nOps/ms: %d.%d\n",
//         x,
//         elapsed_int, elapsed_frac,
//         ops_int, ops_frac);
    
//     HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
//     WriteConsoleA(stdoutHandle, buffer, len, &written, nullptr);
    
//     ExitProcess(0);
// }