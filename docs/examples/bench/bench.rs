// docs/examples/bench/langs/bench.rs
// Rust reference for the OmniScript benchmark suite. Same algorithms, sizes and checks as bench.c.
// Idiomatic safe Rust (bounds checks on). Build: rustc -C opt-level=3 -C target-cpu=native -o bench_rs.exe bench.rs
// Run: bench_rs fib|sieve|nbody|spectral|fannkuch|trees [n]
use std::env;

fn fib(n: i32) -> i64 { if n < 2 { n as i64 } else { fib(n - 1) + fib(n - 2) } }

fn sieve(n: usize) -> i64 {
    let mut s = vec![0u8; n + 1];
    let mut count = 0i64;
    let mut i = 2usize;
    while i <= n {
        if s[i] == 0 {
            count += 1;
            let mut j = i * i;
            while j <= n { s[j] = 1; j += i; }
        }
        i += 1;
    }
    count
}

const PI: f64 = 3.141592653589793;
const SOLAR_MASS: f64 = 4.0 * PI * PI;
const DAYS: f64 = 365.24;

struct Bodies { x: [f64; 5], y: [f64; 5], z: [f64; 5], vx: [f64; 5], vy: [f64; 5], vz: [f64; 5], m: [f64; 5] }

fn nb_init() -> Bodies {
    let xs = [0.0, 4.84143144246472090e+00, 8.34336671824457987e+00, 1.28943695621391310e+01, 1.53796971148509165e+01];
    let ys = [0.0, -1.16032004402742839e+00, 4.12479856412430479e+00, -1.51111514016986312e+01, -2.59193146099879641e+01];
    let zs = [0.0, -1.03622044471123109e-01, -4.03523417114321381e-01, -2.23307578892655734e-01, 1.79258772950371181e-01];
    let vxs = [0.0, 1.66007664274403694e-03, -2.76742510726862411e-03, 2.96460137564761618e-03, 2.68067772490389322e-03];
    let vys = [0.0, 7.69901118419740425e-03, 4.99852801234917238e-03, 2.37847173959480950e-03, 1.62824170038242295e-03];
    let vzs = [0.0, -6.90460016972063023e-05, 2.30417297573763929e-05, -2.96589568540237556e-05, -9.51592254519715870e-05];
    let ms = [1.0, 9.54791938424326609e-04, 2.85885980666130812e-04, 4.36624404335156298e-05, 5.15138902046611451e-05];
    let mut b = Bodies { x: xs, y: ys, z: zs, vx: [0.0; 5], vy: [0.0; 5], vz: [0.0; 5], m: [0.0; 5] };
    for i in 0..5 {
        b.vx[i] = vxs[i] * DAYS; b.vy[i] = vys[i] * DAYS; b.vz[i] = vzs[i] * DAYS;
        b.m[i] = ms[i] * SOLAR_MASS;
    }
    let (mut px, mut py, mut pz) = (0.0f64, 0.0f64, 0.0f64);
    for i in 0..5 { px += b.vx[i] * b.m[i]; py += b.vy[i] * b.m[i]; pz += b.vz[i] * b.m[i]; }
    b.vx[0] = -px / SOLAR_MASS; b.vy[0] = -py / SOLAR_MASS; b.vz[0] = -pz / SOLAR_MASS;
    b
}

fn nb_energy(b: &Bodies) -> f64 {
    let mut e = 0.0f64;
    for i in 0..5 {
        e += 0.5 * b.m[i] * (b.vx[i] * b.vx[i] + b.vy[i] * b.vy[i] + b.vz[i] * b.vz[i]);
        for j in (i + 1)..5 {
            let dx = b.x[i] - b.x[j]; let dy = b.y[i] - b.y[j]; let dz = b.z[i] - b.z[j];
            e -= (b.m[i] * b.m[j]) / (dx * dx + dy * dy + dz * dz).sqrt();
        }
    }
    e
}

fn nb_advance(b: &mut Bodies, dt: f64) {
    for i in 0..5 {
        for j in (i + 1)..5 {
            let dx = b.x[i] - b.x[j]; let dy = b.y[i] - b.y[j]; let dz = b.z[i] - b.z[j];
            let d2 = dx * dx + dy * dy + dz * dz;
            let mag = dt / (d2 * d2.sqrt());
            b.vx[i] -= dx * b.m[j] * mag; b.vy[i] -= dy * b.m[j] * mag; b.vz[i] -= dz * b.m[j] * mag;
            b.vx[j] += dx * b.m[i] * mag; b.vy[j] += dy * b.m[i] * mag; b.vz[j] += dz * b.m[i] * mag;
        }
    }
    for i in 0..5 { b.x[i] += dt * b.vx[i]; b.y[i] += dt * b.vy[i]; b.z[i] += dt * b.vz[i]; }
}

fn sp_a(i: usize, j: usize) -> f64 { 1.0 / (((i + j) * (i + j + 1) / 2 + i + 1) as f64) }
fn sp_mul_av(n: usize, v: &[f64], av: &mut [f64]) {
    for i in 0..n { let mut s = 0.0; for j in 0..n { s += sp_a(i, j) * v[j]; } av[i] = s; }
}
fn sp_mul_atv(n: usize, v: &[f64], atv: &mut [f64]) {
    for i in 0..n { let mut s = 0.0; for j in 0..n { s += sp_a(j, i) * v[j]; } atv[i] = s; }
}
fn spectral(n: usize) -> f64 {
    let mut u = vec![1.0f64; n]; let mut v = vec![0.0f64; n]; let mut t = vec![0.0f64; n];
    for _ in 0..10 {
        sp_mul_av(n, &u, &mut t); sp_mul_atv(n, &t, &mut v);
        sp_mul_av(n, &v, &mut t); sp_mul_atv(n, &t, &mut u);
    }
    let (mut vbv, mut vv) = (0.0f64, 0.0f64);
    for i in 0..n { vbv += u[i] * v[i]; vv += v[i] * v[i]; }
    (vbv / vv).sqrt()
}

fn fannkuch(n: usize) -> (i64, i64) {
    let mut p = [0usize; 32]; let mut q = [0usize; 32]; let mut s = [0usize; 32];
    let mut sign: i64 = 1; let mut sum: i64 = 0; let mut maxf: i64 = 0;
    for i in 0..n { p[i] = i; q[i] = i; s[i] = i; }
    loop {
        let mut q0 = p[0];
        if q0 != 0 {
            for i in 1..n { q[i] = p[i]; }
            let mut flips: i64 = 1;
            loop {
                let qq = q[q0];
                if qq == 0 { sum += sign * flips; if flips > maxf { maxf = flips; } break; }
                q[q0] = q0;
                if q0 >= 3 {
                    let mut i = 1usize; let mut j = q0 - 1;
                    loop { q.swap(i, j); i += 1; j -= 1; if !(i < j) { break; } }
                }
                q0 = qq; flips += 1;
            }
        }
        if sign == 1 { p.swap(0, 1); sign = -1; }
        else {
            p.swap(1, 2); sign = 1;
            let mut i = 2usize;
            while i < n {
                let sx = s[i];
                if sx != 0 { s[i] = sx - 1; break; }
                if i == n - 1 { return (sum, maxf); }
                s[i] = i;
                let t = p[0];
                for j in 0..=i { p[j] = p[j + 1]; }
                p[i + 1] = t;
                i += 1;
            }
        }
    }
}

struct Node { l: Option<Box<Node>>, r: Option<Box<Node>> }
fn mk(d: i32) -> Box<Node> {
    if d > 0 { Box::new(Node { l: Some(mk(d - 1)), r: Some(mk(d - 1)) }) } else { Box::new(Node { l: None, r: None }) }
}
fn chk(n: &Node) -> i64 {
    match (&n.l, &n.r) { (Some(l), Some(r)) => 1 + chk(l) + chk(r), _ => 1 }
}
fn trees(maxd: i32) -> i64 {
    let mut total = 0i64;
    let mut d = 4;
    while d <= maxd {
        let iters = 1i64 << (maxd - d + 4);
        for _ in 0..iters { let t = mk(d); total += chk(&t); }
        d += 4;
    }
    total
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let b = args.get(1).map(|s| s.as_str()).unwrap_or("");
    let n: i64 = args.get(2).map(|s| s.parse().unwrap_or(-1)).unwrap_or(-1);
    let pick = |d: i64| if n < 0 { d } else { n };
    match b {
        "fib" => println!("{}", fib(pick(40) as i32)),
        "sieve" => println!("{}", sieve(pick(100_000_000) as usize)),
        "nbody" => {
            let mut bodies = nb_init();
            println!("{:.9}", nb_energy(&bodies));
            for _ in 0..pick(50_000_000) { nb_advance(&mut bodies, 0.01); }
            println!("{:.9}", nb_energy(&bodies));
        }
        "spectral" => println!("{:.9}", spectral(pick(5500) as usize)),
        "fannkuch" => { let nn = pick(11); let (c, f) = fannkuch(nn as usize); println!("{}\nPfannkuchen({}) = {}", c, nn, f); }
        "trees" => println!("{}", trees(pick(20) as i32)),
        _ => { eprintln!("usage: bench_rs fib|sieve|nbody|spectral|fannkuch|trees [n]"); std::process::exit(1); }
    }
}
