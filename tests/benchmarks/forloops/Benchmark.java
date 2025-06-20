public class Benchmark {
    public static void main(String[] args) {
        // final long freq = 1_000_000_000L; // Using nanoTime
        
        long warmup = 0, warmupNoise = 0;
        for (long i = 0; i < 1_000_000; i++) {
            if (i % 1_000_000_001L == 0) {
                warmupNoise ^= System.nanoTime();
            }
            warmup += i;
        }

        long noise = 0;
        long x = warmup ^ warmupNoise;

        long start = System.nanoTime();
        for (long i = 0; i < 1_000_000_000L; i++) {
            if (i % 1_000_000_001L == 0) {
                noise ^= System.nanoTime();
            }
            x += i;
        }
        long end = System.nanoTime();

        x ^= noise;

        double elapsedMs = (end - start) / 1_000_000.0;
        System.out.printf("Result: %d%n", x);
        System.out.printf("Elapsed: %.4f ms%n", elapsedMs);
        System.out.printf("Ops/ms: %.1f%n", 1_000_000.0 / elapsedMs);
    }
}
