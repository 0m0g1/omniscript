extern "C:/Windows/System32/kernel32.dll" {
    fn QueryPerformanceCounter(counter: int64*) => int;
    fn QueryPerformanceFrequency(freq: int64*) => int;
}

extern "C:/Windows/System32/msvcrt.dll" fn printf(...fmt: char*) => int;

let freq: int64 = 0;
QueryPerformanceFrequency(&freq);

// Optional warmup
let warmup: int64 = 0;
let warmupNoise: int64 = 0;

for (let i = 0; i < 1000000; i++) {
    if (i % 1000000001 == 0) { // A rare but irregular pattern
        let temp: int64 = 0;
        QueryPerformanceCounter(&temp);
        warmupNoise ^= temp;
    }
    warmup += i;
}

let noise: int64 = 0;
// Benchmark
let start: int64 = 0;
let end: int64 = 0;
let x: int64 = warmup ^ warmupNoise;

QueryPerformanceCounter(&start);

for (let i: int64 = 0; i < 1000000000; i++) {
    // Every 10000 iterations, add a small unpredictable value
    if (i % 1000000001 == 0) { // A rare but irregular pattern
        let temp: int64 = 0;
        QueryPerformanceCounter(&temp);
        noise ^= temp;
    }

    x += i;
}

QueryPerformanceCounter(&end);

// Combine noise into final result to prevent dead-code removal
x ^= noise;

let elapsedMs = (end - start) as double * 1000.0 / freq;
printf("Result: %lld\n", x);
printf("Elapsed: %.4f ms\n", elapsedMs);
printf("Ops/ms: %.1f\n", 1000000.0 / elapsedMs);