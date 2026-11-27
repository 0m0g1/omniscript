// bench_spectral: spectral norm (CLBG); floating point division in nested loops.
// Run:  .\bin\Release-windows-x86_64\Osengine.exe bench\bench_spectral.os
// Known (CLBG): N=100 -> 1.274219991 ; N=5500 -> 1.274224153.

module CMem = import "standard/1/c/mem.os";
module Console = import "standard/1/Console.os";

extern "C" fn sqrt(x: f64) => f64;

const N: int = 5500;
const TOL: f64 = 0.000000001;

function absf(v: f64) => f64 {
    if (v < 0.0) { return 0.0 - v; }
    return v;
}

function a(i: int, j: int) => f64 {
    let d: int = (i + j) * (i + j + 1) / 2 + i + 1;
    return 1.0 / (d as f64);
}

function mulAv(n: int, v: f64*, av: f64*) => void {
    for (let i: int = 0; i < n; i = i + 1) {
        let s: f64 = 0.0;
        for (let j: int = 0; j < n; j = j + 1) { s = s + a(i, j) * v[j]; }
        av[i] = s;
    }
}

function mulAtv(n: int, v: f64*, atv: f64*) => void {
    for (let i: int = 0; i < n; i = i + 1) {
        let s: f64 = 0.0;
        for (let j: int = 0; j < n; j = j + 1) { s = s + a(j, i) * v[j]; }
        atv[i] = s;
    }
}

function spectral(n: int) => f64 {
    let bytes: usize = (n as usize) * 8;
    let u: f64* = CMem.malloc(bytes) as f64*;
    let v: f64* = CMem.malloc(bytes) as f64*;
    let t: f64* = CMem.malloc(bytes) as f64*;
    for (let i: int = 0; i < n; i = i + 1) { u[i] = 1.0; }
    for (let k: int = 0; k < 10; k = k + 1) {
        mulAv(n, u, t); mulAtv(n, t, v);
        mulAv(n, v, t); mulAtv(n, t, u);
    }
    let vbv: f64 = 0.0;
    let vv: f64 = 0.0;
    for (let i: int = 0; i < n; i = i + 1) {
        vbv = vbv + u[i] * v[i];
        vv = vv + v[i] * v[i];
    }
    CMem.free(u as void*); CMem.free(v as void*); CMem.free(t as void*);
    return sqrt(vbv / vv);
}

let got: f64 = spectral(N);
let want: f64 = 0.0;
let known: int = 0;
if (N == 100) { want = 1.274219991; known = 1; }
if (N == 5500) { want = 1.274224153; known = 1; }

if (known == 0) { Console.print("N/A   spectral  (no known value for this N)  norm="); }
else if (absf(got - want) < TOL) { Console.print("PASS  spectral  norm="); }
else { Console.print("FAIL  spectral  norm="); }
Console.println(got);
