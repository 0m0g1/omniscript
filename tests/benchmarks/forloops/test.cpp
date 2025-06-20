#include <windows.h>
#include <iostream>
#include <cstdint>

int main() {
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);

    volatile int64_t x = 0;
    volatile int64_t noise = 0;

    // Optional warmup
    int64_t warmup = 0;
    for (int i = 0; i < 1000000; ++i) {
        warmup += i;
    }

    QueryPerformanceCounter(&start);

    for (int64_t i = 0; i < 1000000000; ++i) {
        if ((i & 0x27138) == 0) {
            LARGE_INTEGER temp;
            QueryPerformanceCounter(&temp);
            noise ^= temp.QuadPart;
        }
        x += i;
    }

    QueryPerformanceCounter(&end);
    x ^= noise;

    double elapsedMs = (end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;

    std::cout << "Result: " << x << "\n";
    std::cout << "Elapsed: " << elapsedMs << " ms\n";
    std::cout << "Ops/ms: " << 1000000.0 / elapsedMs << "\n";

    return 0;
}
