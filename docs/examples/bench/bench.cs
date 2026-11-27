// docs/examples/bench/langs/dotnet/bench.cs
// C# (.NET) reference for the OmniScript benchmark suite. Same algorithms, sizes and checks as bench.c.
// Build: dotnet build -c Release -o out        Run: dotnet out/bench_dotnet.dll fib|sieve|nbody|spectral|fannkuch|trees [n]
using System;
using System.Globalization;

public static class Bench
{
    static long Fib(int n) { return n < 2 ? n : Fib(n - 1) + Fib(n - 2); }

    static long Sieve(int n)
    {
        byte[] s = new byte[n + 1];
        long count = 0;
        for (int i = 2; i <= n; i++)
        {
            if (s[i] == 0) { count++; for (long j = (long)i * i; j <= n; j += i) s[j] = 1; }
        }
        return count;
    }

    const double PI = 3.141592653589793, SOLAR_MASS = 4 * PI * PI, DAYS = 365.24;
    static readonly double[] x = new double[5], y = new double[5], z = new double[5];
    static readonly double[] vx = new double[5], vy = new double[5], vz = new double[5], m = new double[5];

    static void NbInit()
    {
        double[] X = { 0, 4.84143144246472090e+00, 8.34336671824457987e+00, 1.28943695621391310e+01, 1.53796971148509165e+01 };
        double[] Y = { 0, -1.16032004402742839e+00, 4.12479856412430479e+00, -1.51111514016986312e+01, -2.59193146099879641e+01 };
        double[] Z = { 0, -1.03622044471123109e-01, -4.03523417114321381e-01, -2.23307578892655734e-01, 1.79258772950371181e-01 };
        double[] VX = { 0, 1.66007664274403694e-03, -2.76742510726862411e-03, 2.96460137564761618e-03, 2.68067772490389322e-03 };
        double[] VY = { 0, 7.69901118419740425e-03, 4.99852801234917238e-03, 2.37847173959480950e-03, 1.62824170038242295e-03 };
        double[] VZ = { 0, -6.90460016972063023e-05, 2.30417297573763929e-05, -2.96589568540237556e-05, -9.51592254519715870e-05 };
        double[] M = { 1, 9.54791938424326609e-04, 2.85885980666130812e-04, 4.36624404335156298e-05, 5.15138902046611451e-05 };
        for (int i = 0; i < 5; i++)
        {
            x[i] = X[i]; y[i] = Y[i]; z[i] = Z[i];
            vx[i] = VX[i] * DAYS; vy[i] = VY[i] * DAYS; vz[i] = VZ[i] * DAYS;
            m[i] = M[i] * SOLAR_MASS;
        }
        double px = 0, py = 0, pz = 0;
        for (int i = 0; i < 5; i++) { px += vx[i] * m[i]; py += vy[i] * m[i]; pz += vz[i] * m[i]; }
        vx[0] = -px / SOLAR_MASS; vy[0] = -py / SOLAR_MASS; vz[0] = -pz / SOLAR_MASS;
    }

    static double NbEnergy()
    {
        double e = 0;
        for (int i = 0; i < 5; i++)
        {
            e += 0.5 * m[i] * (vx[i] * vx[i] + vy[i] * vy[i] + vz[i] * vz[i]);
            for (int j = i + 1; j < 5; j++)
            {
                double dx = x[i] - x[j], dy = y[i] - y[j], dz = z[i] - z[j];
                e -= (m[i] * m[j]) / Math.Sqrt(dx * dx + dy * dy + dz * dz);
            }
        }
        return e;
    }

    static void NbAdvance(double dt)
    {
        for (int i = 0; i < 5; i++)
        {
            for (int j = i + 1; j < 5; j++)
            {
                double dx = x[i] - x[j], dy = y[i] - y[j], dz = z[i] - z[j];
                double d2 = dx * dx + dy * dy + dz * dz;
                double mag = dt / (d2 * Math.Sqrt(d2));
                vx[i] -= dx * m[j] * mag; vy[i] -= dy * m[j] * mag; vz[i] -= dz * m[j] * mag;
                vx[j] += dx * m[i] * mag; vy[j] += dy * m[i] * mag; vz[j] += dz * m[i] * mag;
            }
        }
        for (int i = 0; i < 5; i++) { x[i] += dt * vx[i]; y[i] += dt * vy[i]; z[i] += dt * vz[i]; }
    }

    static double SpA(int i, int j) { return 1.0 / ((i + j) * (i + j + 1) / 2 + i + 1); }
    static void SpMulAv(int n, double[] v, double[] av)
    {
        for (int i = 0; i < n; i++) { double s = 0; for (int j = 0; j < n; j++) s += SpA(i, j) * v[j]; av[i] = s; }
    }
    static void SpMulAtv(int n, double[] v, double[] atv)
    {
        for (int i = 0; i < n; i++) { double s = 0; for (int j = 0; j < n; j++) s += SpA(j, i) * v[j]; atv[i] = s; }
    }
    static double Spectral(int n)
    {
        double[] u = new double[n], v = new double[n], t = new double[n];
        for (int i = 0; i < n; i++) u[i] = 1.0;
        for (int k = 0; k < 10; k++)
        {
            SpMulAv(n, u, t); SpMulAtv(n, t, v);
            SpMulAv(n, v, t); SpMulAtv(n, t, u);
        }
        double vbv = 0, vv = 0;
        for (int i = 0; i < n; i++) { vbv += u[i] * v[i]; vv += v[i] * v[i]; }
        return Math.Sqrt(vbv / vv);
    }

    static void Fannkuch(int n, out long checksum, out int maxflips)
    {
        int[] p = new int[32], q = new int[32], s = new int[32];
        int sign = 1, maxf = 0; long sum = 0;
        for (int i = 0; i < n; i++) { p[i] = i; q[i] = i; s[i] = i; }
        for (;;)
        {
            int q0 = p[0];
            if (q0 != 0)
            {
                for (int i = 1; i < n; i++) q[i] = p[i];
                int flips = 1;
                for (;;)
                {
                    int qq = q[q0];
                    if (qq == 0) { sum += sign * flips; if (flips > maxf) maxf = flips; break; }
                    q[q0] = q0;
                    if (q0 >= 3) { int i = 1, j = q0 - 1, t; do { t = q[i]; q[i] = q[j]; q[j] = t; i++; j--; } while (i < j); }
                    q0 = qq; flips++;
                }
            }
            if (sign == 1) { int t = p[1]; p[1] = p[0]; p[0] = t; sign = -1; }
            else
            {
                int t = p[1]; p[1] = p[2]; p[2] = t; sign = 1;
                for (int i = 2; i < n; i++)
                {
                    int sx = s[i];
                    if (sx != 0) { s[i] = sx - 1; break; }
                    if (i == n - 1) { checksum = sum; maxflips = maxf; return; }
                    s[i] = i;
                    t = p[0]; for (int j = 0; j <= i; j++) p[j] = p[j + 1]; p[i + 1] = t;
                }
            }
        }
    }

    sealed class Node { public Node l, r; public Node(Node l, Node r) { this.l = l; this.r = r; } }
    static Node Mk(int d) { return d > 0 ? new Node(Mk(d - 1), Mk(d - 1)) : new Node(null, null); }
    static long Chk(Node n) { return n.l != null ? 1 + Chk(n.l) + Chk(n.r) : 1; }
    static long Trees(int maxd)
    {
        long total = 0;
        for (int d = 4; d <= maxd; d += 4)
        {
            long iters = 1L << (maxd - d + 4);
            for (long i = 0; i < iters; i++) total += Chk(Mk(d));
        }
        return total;
    }

    static string F9(double v) { return v.ToString("F9", CultureInfo.InvariantCulture); }

    public static int Main(string[] args)
    {
        string b = args.Length > 0 ? args[0] : "";
        long n = args.Length > 1 ? long.Parse(args[1], CultureInfo.InvariantCulture) : -1;
        switch (b)
        {
            case "fib": Console.Out.Write(Fib((int)(n < 0 ? 40 : n)) + "\n"); break;
            case "sieve": Console.Out.Write(Sieve((int)(n < 0 ? 100000000 : n)) + "\n"); break;
            case "nbody":
            {
                NbInit(); Console.Out.Write(F9(NbEnergy()) + "\n");
                long steps = n < 0 ? 50000000L : n;
                for (long i = 0; i < steps; i++) NbAdvance(0.01);
                Console.Out.Write(F9(NbEnergy()) + "\n");
                break;
            }
            case "spectral": Console.Out.Write(F9(Spectral((int)(n < 0 ? 5500 : n))) + "\n"); break;
            case "fannkuch":
            {
                int nn = (int)(n < 0 ? 11 : n);
                Fannkuch(nn, out long c, out int f);
                Console.Out.Write(c + "\nPfannkuchen(" + nn + ") = " + f + "\n");
                break;
            }
            case "trees": Console.Out.Write(Trees((int)(n < 0 ? 20 : n)) + "\n"); break;
            default: Console.Error.WriteLine("usage: dotnet bench_dotnet.dll fib|sieve|nbody|spectral|fannkuch|trees [n]"); return 1;
        }
        return 0;
    }
}
