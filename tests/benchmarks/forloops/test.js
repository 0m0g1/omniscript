const { performance } = require('perf_hooks');

let warmup = 0n, warmupNoise = 0n;
for (let i = 0n; i < 1000000n; i++) {
    if (i % 1000000001n === 0n) {
        warmupNoise ^= BigInt(Math.floor(performance.now() * 1_000_000));
    }
    warmup += i;
}

let x = warmup ^ warmupNoise;
let noise = 0n;

const start = performance.now();
for (let i = 0n; i < 1000000000n; i++) {
    if (i % 1000000001n === 0n) {
        noise ^= BigInt(Math.floor(performance.now() * 1_000_000));
    }
    x += i;
}
const end = performance.now();

x ^= noise;
const elapsedMs = end - start;

console.log(`Result: ${x}`);
console.log(`Elapsed: ${elapsedMs.toFixed(4)} ms`);
console.log(`Ops/ms: ${(1_000_000.0 / elapsedMs).toFixed(1)}`);
