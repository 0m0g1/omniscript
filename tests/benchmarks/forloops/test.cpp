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
