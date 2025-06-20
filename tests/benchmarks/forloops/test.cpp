// --- C++ Version ---
#include <cstdint>
#include <windows.h>
#include <iostream>

int main() {
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);

    int64_t warmup = 0;
    int64_t warmupNoise = 0;

    for (int64_t i = 0; i < 1000000; ++i) {
        if (i % 1000000001 == 0) {
            LARGE_INTEGER temp;
            QueryPerformanceCounter(&temp);
            warmupNoise ^= temp.QuadPart;
        }
        warmup += i;
    }

    int64_t noise = 0;
    int64_t x = warmup ^ warmupNoise;

    QueryPerformanceCounter(&start);

    for (int64_t i = 0; i < 1000000000; ++i) {
        if (i % 1000000001 == 0) {
            LARGE_INTEGER temp;
            QueryPerformanceCounter(&temp);
            noise ^= temp.QuadPart;
        }
        x += i;
    }

    QueryPerformanceCounter(&end);

    x ^= noise;
    double elapsedMs = (double)(end.QuadPart - start.QuadPart) * 1000.0 / (double)freq.QuadPart;

    std::cout << "Result: " << x << "\n";
    std::cout << "Elapsed: " << elapsedMs << " ms\n";
    std::cout << "Ops/ms: " << (1000000.0 / elapsedMs) << "\n";

    return 0;
}
