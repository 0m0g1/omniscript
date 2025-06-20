use std::hint::black_box;
use std::time::Instant;

fn main() {
    let mut x: u64 = 0;
    let mut noise: u64 = 0;

    // Warmup (irregular)
    for i in 0..1_000_000 {
        if i % 1_000_001 == 0 {
            noise ^= black_box(i as u64);
        }
        x = black_box(x + i);
    }

    // Benchmark
    let start = Instant::now();

    for i in 0..1_000_000_000 {
        if i % 1000000001 == 0 {
            noise ^= black_box(i as u64);
        }
        x = black_box(x + i);
    }

    let elapsed = start.elapsed().as_secs_f64() * 1000.0;
    x ^= noise;

    println!("Result: {}", x);
    println!("Elapsed: {:.4} ms", elapsed);
    println!("Ops/ms: {:.1}", 1_000_000.0 / elapsed);
}
