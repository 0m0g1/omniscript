const { performance } = require('perf_hooks'); // In browser, just use `performance.now()`

let x = 0n;
let noise = 0n;

// Optional warmup
let warmup = 0n;
for (let i = 0n; i < 1000000n; i++) {
    warmup += i;
}

const start = performance.now();

for (let i = 0n; i < 1000000000n; i++) {
    if ((i & 0x27138n) === 0n) {
        let temp = BigInt(Math.floor(performance.now() * 1_000_000));
        noise ^= temp;
    }
    x += i;
}

const end = performance.now();

x ^= noise;

const elapsedMs = end - start;
console.log(`Result: ${x}`);
console.log(`Elapsed: ${elapsedMs.toFixed(4)} ms`);
console.log(`Ops/ms: ${(1000000.0 / elapsedMs).toFixed(1)}`);
