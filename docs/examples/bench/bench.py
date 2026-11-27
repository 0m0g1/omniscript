# docs/examples/bench/langs/bench.py
# Python reference for the OmniScript benchmark suite. Same algorithms, sizes and checks as bench.c.
# Run: python bench.py fib|sieve|nbody|spectral|fannkuch|trees [n]
# NOTE: CPython is 50-100x slower than C; the full sizes take many minutes (nbody: hours). Pass a smaller n to try it.
import sys
from math import sqrt

sys.setrecursionlimit(10000)

def fib(n): return n if n < 2 else fib(n - 1) + fib(n - 2)

def sieve(n):
    s = bytearray(n + 1)
    count = 0
    for i in range(2, n + 1):
        if s[i] == 0:
            count += 1
            for j in range(i * i, n + 1, i): s[j] = 1
    return count

PI = 3.141592653589793
SOLAR_MASS = 4 * PI * PI
DAYS = 365.24
X = [0, 4.84143144246472090e+00, 8.34336671824457987e+00, 1.28943695621391310e+01, 1.53796971148509165e+01]
Y = [0, -1.16032004402742839e+00, 4.12479856412430479e+00, -1.51111514016986312e+01, -2.59193146099879641e+01]
Z = [0, -1.03622044471123109e-01, -4.03523417114321381e-01, -2.23307578892655734e-01, 1.79258772950371181e-01]
VX = [0, 1.66007664274403694e-03, -2.76742510726862411e-03, 2.96460137564761618e-03, 2.68067772490389322e-03]
VY = [0, 7.69901118419740425e-03, 4.99852801234917238e-03, 2.37847173959480950e-03, 1.62824170038242295e-03]
VZ = [0, -6.90460016972063023e-05, 2.30417297573763929e-05, -2.96589568540237556e-05, -9.51592254519715870e-05]
M = [1, 9.54791938424326609e-04, 2.85885980666130812e-04, 4.36624404335156298e-05, 5.15138902046611451e-05]
x, y, z = X[:], Y[:], Z[:]
vx = [v * DAYS for v in VX]; vy = [v * DAYS for v in VY]; vz = [v * DAYS for v in VZ]
m = [v * SOLAR_MASS for v in M]

def nb_init():
    px = sum(vx[i] * m[i] for i in range(5)); py = sum(vy[i] * m[i] for i in range(5)); pz = sum(vz[i] * m[i] for i in range(5))
    vx[0] = -px / SOLAR_MASS; vy[0] = -py / SOLAR_MASS; vz[0] = -pz / SOLAR_MASS

def nb_energy():
    e = 0.0
    for i in range(5):
        e += 0.5 * m[i] * (vx[i] * vx[i] + vy[i] * vy[i] + vz[i] * vz[i])
        for j in range(i + 1, 5):
            dx = x[i] - x[j]; dy = y[i] - y[j]; dz = z[i] - z[j]
            e -= (m[i] * m[j]) / sqrt(dx * dx + dy * dy + dz * dz)
    return e

def nb_advance(dt):
    for i in range(5):
        for j in range(i + 1, 5):
            dx = x[i] - x[j]; dy = y[i] - y[j]; dz = z[i] - z[j]
            d2 = dx * dx + dy * dy + dz * dz
            mag = dt / (d2 * sqrt(d2))
            vx[i] -= dx * m[j] * mag; vy[i] -= dy * m[j] * mag; vz[i] -= dz * m[j] * mag
            vx[j] += dx * m[i] * mag; vy[j] += dy * m[i] * mag; vz[j] += dz * m[i] * mag
    for i in range(5):
        x[i] += dt * vx[i]; y[i] += dt * vy[i]; z[i] += dt * vz[i]

def sp_a(i, j): return 1.0 / (((i + j) * (i + j + 1) // 2) + i + 1)
def sp_mul_av(n, v, av):
    for i in range(n):
        s = 0.0
        for j in range(n): s += sp_a(i, j) * v[j]
        av[i] = s
def sp_mul_atv(n, v, atv):
    for i in range(n):
        s = 0.0
        for j in range(n): s += sp_a(j, i) * v[j]
        atv[i] = s
def spectral(n):
    u = [1.0] * n; v = [0.0] * n; t = [0.0] * n
    for _ in range(10):
        sp_mul_av(n, u, t); sp_mul_atv(n, t, v)
        sp_mul_av(n, v, t); sp_mul_atv(n, t, u)
    vbv = sum(u[i] * v[i] for i in range(n)); vv = sum(v[i] * v[i] for i in range(n))
    return sqrt(vbv / vv)

def fannkuch(n):
    p = list(range(32)); q = list(range(32)); s = list(range(32))
    sign = 1; total = 0; maxf = 0
    while True:
        q0 = p[0]
        if q0 != 0:
            for i in range(1, n): q[i] = p[i]
            flips = 1
            while True:
                qq = q[q0]
                if qq == 0:
                    total += sign * flips
                    if flips > maxf: maxf = flips
                    break
                q[q0] = q0
                if q0 >= 3:
                    i = 1; j = q0 - 1
                    while True:
                        q[i], q[j] = q[j], q[i]; i += 1; j -= 1
                        if not i < j: break
                q0 = qq; flips += 1
        if sign == 1:
            p[1], p[0] = p[0], p[1]; sign = -1
        else:
            p[1], p[2] = p[2], p[1]; sign = 1
            i = 2
            while i < n:
                sx = s[i]
                if sx != 0:
                    s[i] = sx - 1; break
                if i == n - 1: return total, maxf
                s[i] = i
                t = p[0]
                for j in range(i + 1): p[j] = p[j + 1]
                p[i + 1] = t
                i += 1

def mk(d): return (mk(d - 1), mk(d - 1)) if d > 0 else None
def chk(t): return 1 if t is None else 1 + chk(t[0]) + chk(t[1])
def trees(maxd):
    total = 0
    for d in range(4, maxd + 1, 4):
        for _ in range(1 << (maxd - d + 4)): total += chk(mk(d))
    return total

b = sys.argv[1] if len(sys.argv) > 1 else ""
n = int(sys.argv[2]) if len(sys.argv) > 2 else -1
if b == "fib": print(fib(40 if n < 0 else n))
elif b == "sieve": print(sieve(100000000 if n < 0 else n))
elif b == "nbody":
    nb_init(); print("%.9f" % nb_energy())
    for _ in range(50000000 if n < 0 else n): nb_advance(0.01)
    print("%.9f" % nb_energy())
elif b == "spectral": print("%.9f" % spectral(5500 if n < 0 else n))
elif b == "fannkuch":
    nn = 11 if n < 0 else n; c, f = fannkuch(nn); print("%d\nPfannkuchen(%d) = %d" % (c, nn, f))
elif b == "trees": print(trees(20 if n < 0 else n))
else:
    sys.stderr.write("usage: python bench.py fib|sieve|nbody|spectral|fannkuch|trees [n]\n"); sys.exit(1)
