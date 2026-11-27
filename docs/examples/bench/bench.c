// C reference for the OmniScript benchmark suite. Same algorithms, same sizes, same checks.
// Build: gcc -O3 -march=native bench.c -o bench_c -lm      (or: clang -O3 -march=native ...)
// Run:   bench_c fib|sieve|nbody|spectral|fannkuch|trees [n]
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

static long long fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }

static long long sieve(long long n) {
    unsigned char* s = (unsigned char*)malloc((size_t)n + 1);
    for (long long k = 0; k <= n; k++) s[k] = 0;
    long long count = 0;
    for (long long i = 2; i <= n; i++) {
        if (s[i] == 0) { count++; for (long long j = i * i; j <= n; j += i) s[j] = 1; }
    }
    free(s);
    return count;
}

#define PI 3.141592653589793
#define SOLAR_MASS (4 * PI * PI)
#define DAYS 365.24
static double x[5], y[5], z[5], vx[5], vy[5], vz[5], m[5];

static void nb_init(void) {
    double X[5] = {0, 4.84143144246472090e+00, 8.34336671824457987e+00, 1.28943695621391310e+01, 1.53796971148509165e+01};
    double Y[5] = {0, -1.16032004402742839e+00, 4.12479856412430479e+00, -1.51111514016986312e+01, -2.59193146099879641e+01};
    double Z[5] = {0, -1.03622044471123109e-01, -4.03523417114321381e-01, -2.23307578892655734e-01, 1.79258772950371181e-01};
    double VX[5] = {0, 1.66007664274403694e-03, -2.76742510726862411e-03, 2.96460137564761618e-03, 2.68067772490389322e-03};
    double VY[5] = {0, 7.69901118419740425e-03, 4.99852801234917238e-03, 2.37847173959480950e-03, 1.62824170038242295e-03};
    double VZ[5] = {0, -6.90460016972063023e-05, 2.30417297573763929e-05, -2.96589568540237556e-05, -9.51592254519715870e-05};
    double M[5] = {1, 9.54791938424326609e-04, 2.85885980666130812e-04, 4.36624404335156298e-05, 5.15138902046611451e-05};
    for (int i = 0; i < 5; i++) {
        x[i] = X[i]; y[i] = Y[i]; z[i] = Z[i];
        vx[i] = VX[i] * DAYS; vy[i] = VY[i] * DAYS; vz[i] = VZ[i] * DAYS;
        m[i] = M[i] * SOLAR_MASS;
    }
    double px = 0, py = 0, pz = 0;
    for (int i = 0; i < 5; i++) { px += vx[i] * m[i]; py += vy[i] * m[i]; pz += vz[i] * m[i]; }
    vx[0] = -px / SOLAR_MASS; vy[0] = -py / SOLAR_MASS; vz[0] = -pz / SOLAR_MASS;
}
static double nb_energy(void) {
    double e = 0;
    for (int i = 0; i < 5; i++) {
        e += 0.5 * m[i] * (vx[i] * vx[i] + vy[i] * vy[i] + vz[i] * vz[i]);
        for (int j = i + 1; j < 5; j++) {
            double dx = x[i] - x[j], dy = y[i] - y[j], dz = z[i] - z[j];
            e -= (m[i] * m[j]) / sqrt(dx * dx + dy * dy + dz * dz);
        }
    }
    return e;
}
static void nb_advance(double dt) {
    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) {
            double dx = x[i] - x[j], dy = y[i] - y[j], dz = z[i] - z[j];
            double d2 = dx * dx + dy * dy + dz * dz;
            double mag = dt / (d2 * sqrt(d2));
            vx[i] -= dx * m[j] * mag; vy[i] -= dy * m[j] * mag; vz[i] -= dz * m[j] * mag;
            vx[j] += dx * m[i] * mag; vy[j] += dy * m[i] * mag; vz[j] += dz * m[i] * mag;
        }
    }
    for (int i = 0; i < 5; i++) { x[i] += dt * vx[i]; y[i] += dt * vy[i]; z[i] += dt * vz[i]; }
}

static double sp_a(int i, int j) { return 1.0 / (double)((i + j) * (i + j + 1) / 2 + i + 1); }
static void sp_mulAv(int n, const double* v, double* av) {
    for (int i = 0; i < n; i++) { double s = 0; for (int j = 0; j < n; j++) s += sp_a(i, j) * v[j]; av[i] = s; }
}
static void sp_mulAtv(int n, const double* v, double* atv) {
    for (int i = 0; i < n; i++) { double s = 0; for (int j = 0; j < n; j++) s += sp_a(j, i) * v[j]; atv[i] = s; }
}
static double spectral(int n) {
    double* u = malloc(n * sizeof(double)); double* v = malloc(n * sizeof(double)); double* t = malloc(n * sizeof(double));
    for (int i = 0; i < n; i++) u[i] = 1.0;
    for (int k = 0; k < 10; k++) {
        sp_mulAv(n, u, t); sp_mulAtv(n, t, v);
        sp_mulAv(n, v, t); sp_mulAtv(n, t, u);
    }
    double vbv = 0, vv = 0;
    for (int i = 0; i < n; i++) { vbv += u[i] * v[i]; vv += v[i] * v[i]; }
    free(u); free(v); free(t);
    return sqrt(vbv / vv);
}

static void fannkuch(int n, long long* checksum, long long* maxflips) {
    int p[32], q[32], s[32], cnt[32];
    int sign = 1; long long sum = 0; int maxf = 0;
    for (int i = 0; i < n; i++) { p[i] = i; q[i] = i; s[i] = i; }
    for (;;) {
        int q0 = p[0];
        if (q0 != 0) {
            for (int i = 1; i < n; i++) q[i] = p[i];
            int flips = 1;
            for (;;) {
                int qq = q[q0];
                if (qq == 0) { sum += sign * flips; if (flips > maxf) maxf = flips; break; }
                q[q0] = q0;
                if (q0 >= 3) { int i = 1, j = q0 - 1, t; do { t = q[i]; q[i] = q[j]; q[j] = t; i++; j--; } while (i < j); }
                q0 = qq; flips++;
            }
        }
        if (sign == 1) { int t = p[1]; p[1] = p[0]; p[0] = t; sign = -1; }
        else {
            int t = p[1]; p[1] = p[2]; p[2] = t; sign = 1;
            for (int i = 2; i < n; i++) {
                int sx = s[i];
                if (sx != 0) { s[i] = sx - 1; break; }
                if (i == n - 1) { *checksum = sum; *maxflips = maxf; return; }
                s[i] = i;
                t = p[0]; for (int j = 0; j <= i; j++) p[j] = p[j + 1]; p[i + 1] = t;
            }
        }
    }
}

typedef struct Node { struct Node* l; struct Node* r; } Node;
static Node* mk(int d) { Node* n = malloc(sizeof(Node)); if (d > 0) { n->l = mk(d - 1); n->r = mk(d - 1); } else { n->l = n->r = 0; } return n; }
static long long chk(Node* n) { return n->l ? 1 + chk(n->l) + chk(n->r) : 1; }
static void fr(Node* n) { if (n->l) { fr(n->l); fr(n->r); } free(n); }
static long long trees(int maxd) {
    long long total = 0;
    for (int d = 4; d <= maxd; d += 4) {
        long long iters = 1LL << (maxd - d + 4);
        for (long long i = 0; i < iters; i++) { Node* t = mk(d); total += chk(t); fr(t); }
    }
    return total;
}

int main(int argc, char** argv) {
    const char* b = argc > 1 ? argv[1] : "";
    long long n = argc > 2 ? atoll(argv[2]) : -1;
    if (!strcmp(b, "fib")) { printf("%lld\n", fib(n < 0 ? 40 : (int)n)); }
    else if (!strcmp(b, "sieve")) { printf("%lld\n", sieve(n < 0 ? 100000000LL : n)); }
    else if (!strcmp(b, "nbody")) { nb_init(); printf("%.9f\n", nb_energy()); long long s = n < 0 ? 50000000LL : n; for (long long i = 0; i < s; i++) nb_advance(0.01); printf("%.9f\n", nb_energy()); }
    else if (!strcmp(b, "spectral")) { printf("%.9f\n", spectral(n < 0 ? 5500 : (int)n)); }
    else if (!strcmp(b, "fannkuch")) { long long c, f; fannkuch(n < 0 ? 11 : (int)n, &c, &f); printf("%lld\nPfannkuchen(%d) = %lld\n", c, n < 0 ? 11 : (int)n, f); }
    else if (!strcmp(b, "trees")) { printf("%lld\n", trees(n < 0 ? 20 : (int)n)); }
    else { fprintf(stderr, "usage: bench_c fib|sieve|nbody|spectral|fannkuch|trees [n]\n"); return 1; }
    return 0;
}
