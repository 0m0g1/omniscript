extern "C:/Windows/System32/kernel32.dll" {
    fn QueryPerformanceCounter(counter: int64*) => int;
    fn QueryPerformanceFrequency(freq: int64*) => int;
}

extern "C:/Windows/System32/msvcrt.dll"
fn printf(...fmt: char*) => int;

function main(argc: int, argv: char**) => i32 {
    let freq: int64 = 0;
    QueryPerformanceFrequency(&freq);

    // Warmup
    let warmup: double = 0.0;
    let warmupNoise: double = 0.0;
    for (let i: int64 = 0; i < 1000000; i++) {
        if (i % 1000000001 == 0) {
            let temp: int64 = 0;
            QueryPerformanceCounter(&temp);
            warmupNoise = warmupNoise + (temp as double);
        }
        warmup = warmup + (i as double);
    }

    // Floating-point constants
    const a: double = 0.5;
    const b: double = 3.141593;   // π
    const c: double = 2.718282;   // e

    let x: double = warmup + warmupNoise;
    let noise: double = 0.0;

    let start: int64 = 0;
    let end: int64 = 0;

    QueryPerformanceCounter(&start);

    for (let i: int64 = 1; i < 1000000000; i++) {
        if (i % 1000000001 == 0) {
            let temp: int64 = 0;
            QueryPerformanceCounter(&temp);
            noise = noise + (temp as double);
        }

        let fi: double = i as double;

        let mul1: double = fi * a;
        let div1: double = b / fi;
        let left: double = mul1 + div1;

        let div2: double = fi / c;
        let mul2: double = a * b;
        let right: double = div2 + mul2;

        let result: double = left - right;
        x = x + result;
    }

    QueryPerformanceCounter(&end);

    let elapsedMs: double = (end - start) as double * 1000.0 / freq as double;

    printf("Result: %.0f\n", x + noise);  // Rounded float result
    printf("Elapsed: %.4f ms\n", elapsedMs);
    printf("Ops/ms: %.1f\n", 1000000000.0 / elapsedMs);

    return 0;
}

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
