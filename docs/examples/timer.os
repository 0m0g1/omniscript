extern "kernel32" {
    fn QueryPerformanceCounter(counter: int64*) => int;
    fn QueryPerformanceFrequency(freq: int64*) => int;
}

extern "C" fn printf(...fmt: char*) => int;

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


// extern "C:/Windows/System32/kernel32.dll" {
//     fn QueryPerformanceCounter(counter: int64*) => int;
//     fn QueryPerformanceFrequency(freq: int64*) => int;
// }

// extern "C:/Windows/System32/msvcrt.dll" fn printf(...fmt: char*) => int;

// let freq: int64 = 0;
// QueryPerformanceFrequency(&freq);

// // Warmup
// let warmup: int64 = 0;
// let warmupNoise: int64 = 0;

// for (let i: int64 = 0; i < 1000000; i++) {
//     if (i % 1000000001 == 0) {
//         let temp: int64 = 0;
//         QueryPerformanceCounter(&temp);
//         warmupNoise ^= temp;
//     }
//     warmup += i;
// }

// // Fixed-point constants (scaled by 1e6)
// const a: int64 = 500000;    // 0.5
// const b: int64 = 3141593;   // π
// const c: int64 = 2718282;   // e

// let x: int64 = warmup ^ warmupNoise;
// let noise: int64 = 0;
// let start: int64 = 0;
// let end: int64 = 0;

// QueryPerformanceCounter(&start);

// for (let i: int64 = 1; i < 1000000000; i++) {
//     if (i % 1000000001 == 0) {
//         let temp: int64 = 0;
//         QueryPerformanceCounter(&temp);
//         noise ^= temp;
//     }

//     let mul1 = i * a;
//     let div1 = b / i;
//     let left = mul1 + div1;

//     let div2 = i / c;
//     let mul2 = (a * b) / 1000000;
//     let right = div2 + mul2;

//     let result = left - right;
//     x += result;
// }

// QueryPerformanceCounter(&end);

// let elapsedMs: double = (end - start) as double * 1000.0 / freq as double;

// printf("Result: %lld\n", x ^ noise);
// printf("Elapsed: %.4f ms\n", elapsedMs);
// printf("Ops/ms: %.1f\n", 1000000000.0 / elapsedMs);
