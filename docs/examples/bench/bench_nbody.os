// docs/examples/bench/bench_nbody.os
// bench_nbody: n-body simulation (Computer Language Benchmarks Game); floating point + sqrt.
// Run:  .\bin\Release-windows-x86_64\Osengine.exe bench\bench_nbody.os
// Known energies (CLBG): start -0.169075164 ; after 1000 steps -0.169087605 ; after 50,000,000 steps -0.169059907.
// STEPS = 50000000 is the standard size. Use 1000 for a quick check.

module CMem = import "standard/1/c/mem.os";
module Console = import "standard/1/Console.os";

extern "C" fn sqrt(x: f64) => f64;

const STEPS: i64 = 50000000;
const TOL: f64 = 0.000000001;

function absf(v: f64) => f64 {
    if (v < 0.0) { return 0.0 - v; }
    return v;
}

function dump(label: char*, v: f64) => void {
    Console.print(label);
    Console.println((v * 1000000000.0) as i64);
}

function setup(x: f64*, y: f64*, z: f64*, vx: f64*, vy: f64*, vz: f64*, m: f64*) => void {
    let pi: f64 = 3.141592653589793;
    let solar: f64 = 4.0 * pi * pi;
    x[0] = 0.0;
    x[1] = 4.84143144246472090;
    x[2] = 8.34336671824457987;
    x[3] = 12.8943695621391310;
    x[4] = 15.3796971148509165;
    y[0] = 0.0;
    y[1] = -1.16032004402742839;
    y[2] = 4.12479856412430479;
    y[3] = -15.1111514016986312;
    y[4] = -25.9193146099879641;
    z[0] = 0.0;
    z[1] = -0.103622044471123109;
    z[2] = -0.403523417114321381;
    z[3] = -0.223307578892655734;
    z[4] = 0.179258772950371181;
    vx[0] = 0.0 * 365.24;
    vx[1] = 0.00166007664274403694 * 365.24;
    vx[2] = -0.00276742510726862411 * 365.24;
    vx[3] = 0.00296460137564761618 * 365.24;
    vx[4] = 0.00268067772490389322 * 365.24;
    vy[0] = 0.0 * 365.24;
    vy[1] = 0.00769901118419740425 * 365.24;
    vy[2] = 0.00499852801234917238 * 365.24;
    vy[3] = 0.00237847173959480950 * 365.24;
    vy[4] = 0.00162824170038242295 * 365.24;
    vz[0] = 0.0 * 365.24;
    vz[1] = -0.0000690460016972063023 * 365.24;
    vz[2] = 0.0000230417297573763929 * 365.24;
    vz[3] = -0.0000296589568540237556 * 365.24;
    vz[4] = -0.0000951592254519715870 * 365.24;
    m[0] = 1.0 * solar;
    m[1] = 0.000954791938424326609 * solar;
    m[2] = 0.000285885980666130812 * solar;
    m[3] = 0.0000436624404335156298 * solar;
    m[4] = 0.0000515138902046611451 * solar;
    let px: f64 = 0.0;
    let py: f64 = 0.0;
    let pz: f64 = 0.0;
    for (let i: int = 0; i < 5; i = i + 1) {
        px = px + vx[i] * m[i];
        py = py + vy[i] * m[i];
        pz = pz + vz[i] * m[i];
    }
    vx[0] = (0.0 - px) / solar;
    vy[0] = (0.0 - py) / solar;
    vz[0] = (0.0 - pz) / solar;
}

function energy(x: f64*, y: f64*, z: f64*, vx: f64*, vy: f64*, vz: f64*, m: f64*) => f64 {
    let e: f64 = 0.0;
    for (let i: int = 0; i < 5; i = i + 1) {
        e = e + 0.5 * m[i] * (vx[i] * vx[i] + vy[i] * vy[i] + vz[i] * vz[i]);
        for (let j: int = i + 1; j < 5; j = j + 1) {
            let dx: f64 = x[i] - x[j];
            let dy: f64 = y[i] - y[j];
            let dz: f64 = z[i] - z[j];
            e = e - (m[i] * m[j]) / sqrt(dx * dx + dy * dy + dz * dz);
        }
    }
    return e;
}

function advance(dt: f64, x: f64*, y: f64*, z: f64*, vx: f64*, vy: f64*, vz: f64*, m: f64*) => void {
    for (let i: int = 0; i < 5; i = i + 1) {
        for (let j: int = i + 1; j < 5; j = j + 1) {
            let dx: f64 = x[i] - x[j];
            let dy: f64 = y[i] - y[j];
            let dz: f64 = z[i] - z[j];
            let d2: f64 = dx * dx + dy * dy + dz * dz;
            let mag: f64 = dt / (d2 * sqrt(d2));
            vx[i] = vx[i] - dx * m[j] * mag;
            vy[i] = vy[i] - dy * m[j] * mag;
            vz[i] = vz[i] - dz * m[j] * mag;
            vx[j] = vx[j] + dx * m[i] * mag;
            vy[j] = vy[j] + dy * m[i] * mag;
            vz[j] = vz[j] + dz * m[i] * mag;
        }
    }
    for (let i: int = 0; i < 5; i = i + 1) {
        x[i] = x[i] + dt * vx[i];
        y[i] = y[i] + dt * vy[i];
        z[i] = z[i] + dt * vz[i];
    }
}

let bytes: usize = 40;
let x: f64* = CMem.malloc(bytes) as f64*;
let y: f64* = CMem.malloc(bytes) as f64*;
let z: f64* = CMem.malloc(bytes) as f64*;
let vx: f64* = CMem.malloc(bytes) as f64*;
let vy: f64* = CMem.malloc(bytes) as f64*;
let vz: f64* = CMem.malloc(bytes) as f64*;
let m: f64* = CMem.malloc(bytes) as f64*;

setup(x, y, z, vx, vy, vz, m);
let e0: f64 = energy(x, y, z, vx, vy, vz, m);

let s: i64 = 0;
while (s < STEPS) {
    advance(0.01, x, y, z, vx, vy, vz, m);
    s = s + 1;
}
let e1: f64 = energy(x, y, z, vx, vy, vz, m);

let want0: f64 = -0.169075164;
let want1: f64 = 0.0;
let known: int = 0;
if (STEPS == 1000) { want1 = -0.169087605; known = 1; }
if (STEPS == 50000000) { want1 = -0.169059907; known = 1; }

if (known == 0) { Console.print("N/A   nbody  (no known value for this STEPS)  energy after="); }
else if (absf(e0 - want0) < TOL && absf(e1 - want1) < TOL) { Console.print("PASS  nbody  energy after="); }
else { Console.print("FAIL  nbody  energy after="); }
Console.println(e1);

// Diagnostics: println(f64) only shows ~6 digits, so also print the energies in 1e-9 units.
// Expected at STEPS=1000:     e0 = -169075164  e1 = -169087605
// Expected at STEPS=50000000: e0 = -169075164  e1 = -169059907  (last digit may be off by one from truncation)
Console.print("  e0 (1e-9 units) = "); Console.println((e0 * 1000000000.0) as i64);
Console.print("  e1 (1e-9 units) = "); Console.println((e1 * 1000000000.0) as i64);

CMem.free(x as void*); CMem.free(y as void*); CMem.free(z as void*);
CMem.free(vx as void*); CMem.free(vy as void*); CMem.free(vz as void*); CMem.free(m as void*);
