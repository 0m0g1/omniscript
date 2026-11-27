// docs/examples/bench/langs/bench.js
// JavaScript (Node) reference for the OmniScript benchmark suite. Same algorithms, sizes and checks as bench.c.
// Run: node bench.js fib|sieve|nbody|spectral|fannkuch|trees [n]
"use strict";

function fib(n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }

function sieve(n) {
    const s = new Uint8Array(n + 1);
    let count = 0;
    for (let i = 2; i <= n; i++) {
        if (s[i] === 0) { count++; for (let j = i * i; j <= n; j += i) s[j] = 1; }
    }
    return count;
}

const PI = 3.141592653589793, SOLAR_MASS = 4 * PI * PI, DAYS = 365.24;
const x = new Float64Array(5), y = new Float64Array(5), z = new Float64Array(5);
const vx = new Float64Array(5), vy = new Float64Array(5), vz = new Float64Array(5), m = new Float64Array(5);

function nbInit() {
    const X = [0, 4.84143144246472090e+00, 8.34336671824457987e+00, 1.28943695621391310e+01, 1.53796971148509165e+01];
    const Y = [0, -1.16032004402742839e+00, 4.12479856412430479e+00, -1.51111514016986312e+01, -2.59193146099879641e+01];
    const Z = [0, -1.03622044471123109e-01, -4.03523417114321381e-01, -2.23307578892655734e-01, 1.79258772950371181e-01];
    const VX = [0, 1.66007664274403694e-03, -2.76742510726862411e-03, 2.96460137564761618e-03, 2.68067772490389322e-03];
    const VY = [0, 7.69901118419740425e-03, 4.99852801234917238e-03, 2.37847173959480950e-03, 1.62824170038242295e-03];
    const VZ = [0, -6.90460016972063023e-05, 2.30417297573763929e-05, -2.96589568540237556e-05, -9.51592254519715870e-05];
    const M = [1, 9.54791938424326609e-04, 2.85885980666130812e-04, 4.36624404335156298e-05, 5.15138902046611451e-05];
    for (let i = 0; i < 5; i++) {
        x[i] = X[i]; y[i] = Y[i]; z[i] = Z[i];
        vx[i] = VX[i] * DAYS; vy[i] = VY[i] * DAYS; vz[i] = VZ[i] * DAYS;
        m[i] = M[i] * SOLAR_MASS;
    }
    let px = 0, py = 0, pz = 0;
    for (let i = 0; i < 5; i++) { px += vx[i] * m[i]; py += vy[i] * m[i]; pz += vz[i] * m[i]; }
    vx[0] = -px / SOLAR_MASS; vy[0] = -py / SOLAR_MASS; vz[0] = -pz / SOLAR_MASS;
}
function nbEnergy() {
    let e = 0;
    for (let i = 0; i < 5; i++) {
        e += 0.5 * m[i] * (vx[i] * vx[i] + vy[i] * vy[i] + vz[i] * vz[i]);
        for (let j = i + 1; j < 5; j++) {
            const dx = x[i] - x[j], dy = y[i] - y[j], dz = z[i] - z[j];
            e -= (m[i] * m[j]) / Math.sqrt(dx * dx + dy * dy + dz * dz);
        }
    }
    return e;
}
function nbAdvance(dt) {
    for (let i = 0; i < 5; i++) {
        for (let j = i + 1; j < 5; j++) {
            const dx = x[i] - x[j], dy = y[i] - y[j], dz = z[i] - z[j];
            const d2 = dx * dx + dy * dy + dz * dz;
            const mag = dt / (d2 * Math.sqrt(d2));
            vx[i] -= dx * m[j] * mag; vy[i] -= dy * m[j] * mag; vz[i] -= dz * m[j] * mag;
            vx[j] += dx * m[i] * mag; vy[j] += dy * m[i] * mag; vz[j] += dz * m[i] * mag;
        }
    }
    for (let i = 0; i < 5; i++) { x[i] += dt * vx[i]; y[i] += dt * vy[i]; z[i] += dt * vz[i]; }
}

function spA(i, j) { return 1.0 / ((((i + j) * (i + j + 1)) >> 1) + i + 1); }
function spMulAv(n, v, av) {
    for (let i = 0; i < n; i++) { let s = 0; for (let j = 0; j < n; j++) s += spA(i, j) * v[j]; av[i] = s; }
}
function spMulAtv(n, v, atv) {
    for (let i = 0; i < n; i++) { let s = 0; for (let j = 0; j < n; j++) s += spA(j, i) * v[j]; atv[i] = s; }
}
function spectral(n) {
    const u = new Float64Array(n).fill(1.0), v = new Float64Array(n), t = new Float64Array(n);
    for (let k = 0; k < 10; k++) {
        spMulAv(n, u, t); spMulAtv(n, t, v);
        spMulAv(n, v, t); spMulAtv(n, t, u);
    }
    let vbv = 0, vv = 0;
    for (let i = 0; i < n; i++) { vbv += u[i] * v[i]; vv += v[i] * v[i]; }
    return Math.sqrt(vbv / vv);
}

function fannkuch(n) {
    const p = new Int32Array(32), q = new Int32Array(32), s = new Int32Array(32);
    let sign = 1, sum = 0, maxf = 0;
    for (let i = 0; i < n; i++) { p[i] = i; q[i] = i; s[i] = i; }
    for (;;) {
        let q0 = p[0];
        if (q0 !== 0) {
            for (let i = 1; i < n; i++) q[i] = p[i];
            let flips = 1;
            for (;;) {
                const qq = q[q0];
                if (qq === 0) { sum += sign * flips; if (flips > maxf) maxf = flips; break; }
                q[q0] = q0;
                if (q0 >= 3) { let i = 1, j = q0 - 1, t; do { t = q[i]; q[i] = q[j]; q[j] = t; i++; j--; } while (i < j); }
                q0 = qq; flips++;
            }
        }
        if (sign === 1) { const t = p[1]; p[1] = p[0]; p[0] = t; sign = -1; }
        else {
            let t = p[1]; p[1] = p[2]; p[2] = t; sign = 1;
            for (let i = 2; i < n; i++) {
                const sx = s[i];
                if (sx !== 0) { s[i] = sx - 1; break; }
                if (i === n - 1) return [sum, maxf];
                s[i] = i;
                t = p[0]; for (let j = 0; j <= i; j++) p[j] = p[j + 1]; p[i + 1] = t;
            }
        }
    }
}

class Node { constructor(l, r) { this.l = l; this.r = r; } }
function mk(d) { return d > 0 ? new Node(mk(d - 1), mk(d - 1)) : new Node(null, null); }
function chk(n) { return n.l !== null ? 1 + chk(n.l) + chk(n.r) : 1; }
function trees(maxd) {
    let total = 0;
    for (let d = 4; d <= maxd; d += 4) {
        const iters = 2 ** (maxd - d + 4);
        for (let i = 0; i < iters; i++) total += chk(mk(d));
    }
    return total;
}

const b = process.argv[2] || "";
const n = process.argv[3] !== undefined ? Number(process.argv[3]) : -1;
if (b === "fib") console.log(String(fib(n < 0 ? 40 : n)));
else if (b === "sieve") console.log(String(sieve(n < 0 ? 100000000 : n)));
else if (b === "nbody") {
    nbInit(); console.log(nbEnergy().toFixed(9));
    const steps = n < 0 ? 50000000 : n;
    for (let i = 0; i < steps; i++) nbAdvance(0.01);
    console.log(nbEnergy().toFixed(9));
} else if (b === "spectral") console.log(spectral(n < 0 ? 5500 : n).toFixed(9));
else if (b === "fannkuch") { const nn = n < 0 ? 11 : n; const [c, f] = fannkuch(nn); console.log(c + "\nPfannkuchen(" + nn + ") = " + f); }
else if (b === "trees") console.log(String(trees(n < 0 ? 20 : n)));
else { console.error("usage: node bench.js fib|sieve|nbody|spectral|fannkuch|trees [n]"); process.exit(1); }
