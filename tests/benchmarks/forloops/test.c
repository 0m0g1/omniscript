// Benchmarking loop with noise and performance timing
#include <windows.h>
#include <stdio.h>
#include <stdint.h>

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

    printf("Result: %lld\n", x);
    printf("Elapsed: %.4f ms\n", elapsedMs);
    printf("Ops/ms: %.1f\n", 1000000.0 / elapsedMs);

    return 0;
}
