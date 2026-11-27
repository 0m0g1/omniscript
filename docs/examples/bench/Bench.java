// docs/examples/bench/langs/Bench.java
// Java reference for the OmniScript benchmark suite. Same algorithms, sizes and checks as bench.c.
// Build: javac -d out Bench.java        Run: java -cp out Bench fib|sieve|nbody|spectral|fannkuch|trees [n]
import java.math.BigDecimal;
import java.math.RoundingMode;

public class Bench {
    static long fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }

    static long sieve(int n) {
        byte[] s = new byte[n + 1];
        long count = 0;
        for (int i = 2; i <= n; i++) {
            if (s[i] == 0) { count++; for (long j = (long) i * i; j <= n; j += i) s[(int) j] = 1; }
        }
        return count;
    }

    static final double PI = 3.141592653589793, SOLAR_MASS = 4 * PI * PI, DAYS = 365.24;
    static final double[] x = new double[5], y = new double[5], z = new double[5];
    static final double[] vx = new double[5], vy = new double[5], vz = new double[5], m = new double[5];

    static void nbInit() {
        double[] X = {0, 4.84143144246472090e+00, 8.34336671824457987e+00, 1.28943695621391310e+01, 1.53796971148509165e+01};
        double[] Y = {0, -1.16032004402742839e+00, 4.12479856412430479e+00, -1.51111514016986312e+01, -2.59193146099879641e+01};
        double[] Z = {0, -1.03622044471123109e-01, -4.03523417114321381e-01, -2.23307578892655734e-01, 1.79258772950371181e-01};
        double[] VX = {0, 1.66007664274403694e-03, -2.76742510726862411e-03, 2.96460137564761618e-03, 2.68067772490389322e-03};
        double[] VY = {0, 7.69901118419740425e-03, 4.99852801234917238e-03, 2.37847173959480950e-03, 1.62824170038242295e-03};
        double[] VZ = {0, -6.90460016972063023e-05, 2.30417297573763929e-05, -2.96589568540237556e-05, -9.51592254519715870e-05};
        double[] M = {1, 9.54791938424326609e-04, 2.85885980666130812e-04, 4.36624404335156298e-05, 5.15138902046611451e-05};
        for (int i = 0; i < 5; i++) {
            x[i] = X[i]; y[i] = Y[i]; z[i] = Z[i];
            vx[i] = VX[i] * DAYS; vy[i] = VY[i] * DAYS; vz[i] = VZ[i] * DAYS;
            m[i] = M[i] * SOLAR_MASS;
        }
        double px = 0, py = 0, pz = 0;
        for (int i = 0; i < 5; i++) { px += vx[i] * m[i]; py += vy[i] * m[i]; pz += vz[i] * m[i]; }
        vx[0] = -px / SOLAR_MASS; vy[0] = -py / SOLAR_MASS; vz[0] = -pz / SOLAR_MASS;
    }

    static double nbEnergy() {
        double e = 0;
        for (int i = 0; i < 5; i++) {
            e += 0.5 * m[i] * (vx[i] * vx[i] + vy[i] * vy[i] + vz[i] * vz[i]);
            for (int j = i + 1; j < 5; j++) {
                double dx = x[i] - x[j], dy = y[i] - y[j], dz = z[i] - z[j];
                e -= (m[i] * m[j]) / Math.sqrt(dx * dx + dy * dy + dz * dz);
            }
        }
        return e;
    }

    static void nbAdvance(double dt) {
        for (int i = 0; i < 5; i++) {
            for (int j = i + 1; j < 5; j++) {
                double dx = x[i] - x[j], dy = y[i] - y[j], dz = z[i] - z[j];
                double d2 = dx * dx + dy * dy + dz * dz;
                double mag = dt / (d2 * Math.sqrt(d2));
                vx[i] -= dx * m[j] * mag; vy[i] -= dy * m[j] * mag; vz[i] -= dz * m[j] * mag;
                vx[j] += dx * m[i] * mag; vy[j] += dy * m[i] * mag; vz[j] += dz * m[i] * mag;
            }
        }
        for (int i = 0; i < 5; i++) { x[i] += dt * vx[i]; y[i] += dt * vy[i]; z[i] += dt * vz[i]; }
    }

    static double spA(int i, int j) { return 1.0 / ((i + j) * (i + j + 1) / 2 + i + 1); }
    static void spMulAv(int n, double[] v, double[] av) {
        for (int i = 0; i < n; i++) { double s = 0; for (int j = 0; j < n; j++) s += spA(i, j) * v[j]; av[i] = s; }
    }
    static void spMulAtv(int n, double[] v, double[] atv) {
        for (int i = 0; i < n; i++) { double s = 0; for (int j = 0; j < n; j++) s += spA(j, i) * v[j]; atv[i] = s; }
    }
    static double spectral(int n) {
        double[] u = new double[n], v = new double[n], t = new double[n];
        java.util.Arrays.fill(u, 1.0);
        for (int k = 0; k < 10; k++) {
            spMulAv(n, u, t); spMulAtv(n, t, v);
            spMulAv(n, v, t); spMulAtv(n, t, u);
        }
        double vbv = 0, vv = 0;
        for (int i = 0; i < n; i++) { vbv += u[i] * v[i]; vv += v[i] * v[i]; }
        return Math.sqrt(vbv / vv);
    }

    static long[] fannkuch(int n) {
        int[] p = new int[32], q = new int[32], s = new int[32];
        int sign = 1, maxf = 0; long sum = 0;
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
                    if (i == n - 1) return new long[]{sum, maxf};
                    s[i] = i;
                    t = p[0]; for (int j = 0; j <= i; j++) p[j] = p[j + 1]; p[i + 1] = t;
                }
            }
        }
    }

    static final class Node { Node l, r; Node(Node l, Node r) { this.l = l; this.r = r; } }
    static Node mk(int d) { return d > 0 ? new Node(mk(d - 1), mk(d - 1)) : new Node(null, null); }
    static long chk(Node n) { return n.l != null ? 1 + chk(n.l) + chk(n.r) : 1; }
    static long trees(int maxd) {
        long total = 0;
        for (int d = 4; d <= maxd; d += 4) {
            long iters = 1L << (maxd - d + 4);
            for (long i = 0; i < iters; i++) total += chk(mk(d));
        }
        return total;
    }

    // exact decimal rounding, like C's printf("%.9f")
    static String f9(double v) { return new BigDecimal(v).setScale(9, RoundingMode.HALF_EVEN).toPlainString(); }

    public static void main(String[] args) {
        String b = args.length > 0 ? args[0] : "";
        long n = args.length > 1 ? Long.parseLong(args[1]) : -1;
        switch (b) {
            case "fib": System.out.println(fib((int) (n < 0 ? 40 : n))); break;
            case "sieve": System.out.println(sieve((int) (n < 0 ? 100000000 : n))); break;
            case "nbody": {
                nbInit(); System.out.println(f9(nbEnergy()));
                long steps = n < 0 ? 50000000L : n;
                for (long i = 0; i < steps; i++) nbAdvance(0.01);
                System.out.println(f9(nbEnergy()));
                break;
            }
            case "spectral": System.out.println(f9(spectral((int) (n < 0 ? 5500 : n)))); break;
            case "fannkuch": {
                int nn = (int) (n < 0 ? 11 : n);
                long[] r = fannkuch(nn);
                System.out.println(r[0] + "\nPfannkuchen(" + nn + ") = " + r[1]);
                break;
            }
            case "trees": System.out.println(trees((int) (n < 0 ? 20 : n))); break;
            default: System.err.println("usage: java Bench fib|sieve|nbody|spectral|fannkuch|trees [n]"); System.exit(1);
        }
    }
}
