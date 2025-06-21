// bench_c.c
// gcc -O3 -o bench_c.exe test.c
#include <windows.h>
#include <stdint.h>
#include <stdio.h>

typedef int (__stdcall *PerfCounterFn)(int64_t*);

int main() {
    HMODULE kernel32 = LoadLibraryA("kernel32.dll");
    if (!kernel32) {
        fprintf(stderr, "Failed to load kernel32.dll\n");
        return 1;
    }

    PerfCounterFn QueryPerformanceFrequency = (PerfCounterFn)GetProcAddress(kernel32, "QueryPerformanceFrequency");
    PerfCounterFn QueryPerformanceCounter = (PerfCounterFn)GetProcAddress(kernel32, "QueryPerformanceCounter");

    if (!QueryPerformanceFrequency || !QueryPerformanceCounter) {
        fprintf(stderr, "Failed to get function pointers\n");
        return 1;
    }

    int64_t freq = 0;
    int64_t start = 0, end = 0;
    int64_t warmup = 0, warmupNoise = 0;

    QueryPerformanceFrequency(&freq);

    // Warmup loop with noise
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

    printf("Result: %lld\n", x);
    printf("Elapsed: %.4f ms\n", elapsedMs);
    printf("Ops/ms: %.1f\n", 1000000.0 / elapsedMs);

    FreeLibrary(kernel32);
    return 0;
}

// // Freestanding C
// // Compiled with 
// // gcc -O3 test.c -ffreestanding -nostdlib -lkernel32 -lmsvcrt -luser32 "-Wl,--entry=_start" -o bench_c.exe
// #include <windows.h>
// #include <stdint.h>
// #include <stdio.h>

// typedef int (__stdcall *PerfCounterFn)(int64_t*);

// int main() {
//     HMODULE kernel32 = LoadLibraryA("kernel32.dll");
//     if (!kernel32) {
//         fprintf(stderr, "Failed to load kernel32.dll\n");
//         return 1;
//     }

//     PerfCounterFn QueryPerformanceFrequency = (PerfCounterFn)GetProcAddress(kernel32, "QueryPerformanceFrequency");
//     PerfCounterFn QueryPerformanceCounter = (PerfCounterFn)GetProcAddress(kernel32, "QueryPerformanceCounter");

//     if (!QueryPerformanceFrequency || !QueryPerformanceCounter) {
//         fprintf(stderr, "Failed to get function pointers\n");
//         return 1;
//     }

//     int64_t freq = 0;
//     int64_t start = 0, end = 0;
//     int64_t warmup = 0, warmupNoise = 0;

//     QueryPerformanceFrequency(&freq);

//     // Warmup loop with noise
//     for (int64_t i = 0; i < 1000000; ++i) {
//         if (i % 1000000001 == 0) {
//             int64_t temp = 0;
//             QueryPerformanceCounter(&temp);
//             warmupNoise ^= temp;
//         }
//         warmup += i;
//     }

//     int64_t noise = 0;
//     int64_t x = warmup ^ warmupNoise;

//     QueryPerformanceCounter(&start);

//     for (int64_t i = 0; i < 1000000000; ++i) {
//         if (i % 1000000001 == 0) {
//             int64_t temp = 0;
//             QueryPerformanceCounter(&temp);
//             noise ^= temp;
//         }
//         x += i;
//     }

//     QueryPerformanceCounter(&end);
//     x ^= noise;

//     double elapsedMs = (double)(end - start) * 1000.0 / freq;

//     printf("Result: %lld\n", x);
//     printf("Elapsed: %.4f ms\n", elapsedMs);
//     printf("Ops/ms: %.1f\n", 1000000.0 / elapsedMs);

//     FreeLibrary(kernel32);
//     return 0;
// }