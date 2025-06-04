module Math {
    // Constants
    public const pi: float        = 3.141592653589793;     // π :contentReference[oaicite:3]{index=3}
    // public const e: float         = 2.718281828459045;     // e :contentReference[oaicite:4]{index=4}

    // // Basic Arithmetic
    // public fn pow(base: float = 1.0, exponent: float = 1.0) => float {
    //     // Uses built-in exponentiation; fallback integer loop provided below
    //     return intrinsic_pow(base, exponent);
    // }
    // public fn powi(base: int = 1, exponent: int = 1) => int {
    //     let result: int = 1;
    //     for (let i = 0; i < exponent; i += 1) {
    //         result *= base;
    //     }
    //     return result;
    // }
    // public fn abs(x: int)    => int   { return intrinsic_abs(x); }
    // public fn abs(x: float)  => float { return intrinsic_fabs(x); }
    // public fn sqrt(x: float) => float { return intrinsic_sqrt(x); }    // sqrt from C std :contentReference[oaicite:5]{index=5}
    // public fn cbrt(x: float) => float { return intrinsic_cbrt(x); }    // cubic root :contentReference[oaicite:6]{index=6}

    // // Rounding
    // public fn floor(x: float) => float { return intrinsic_floor(x); }  // floor function :contentReference[oaicite:7]{index=7}
    // public fn ceil(x: float)  => float { return intrinsic_ceil(x); }   // ceil function :contentReference[oaicite:8]{index=8}
    // public fn round(x: float) => int   { return intrinsic_nearest_int(x); }

    // // Trigonometry
    // public fn sin(x: float)   => float { return intrinsic_sin(x); }    // sin :contentReference[oaicite:9]{index=9}
    // public fn cos(x: float)   => float { return intrinsic_cos(x); }    // cos :contentReference[oaicite:10]{index=10}
    // public fn tan(x: float)   => float { return intrinsic_tan(x); }    // tan :contentReference[oaicite:11]{index=11}
    // public fn asin(x: float)  => float { return intrinsic_asin(x); }   // asin :contentReference[oaicite:12]{index=12}
    // public fn acos(x: float)  => float { return intrinsic_acos(x); }   // acos :contentReference[oaicite:13]{index=13}
    // public fn atan(x: float)  => float { return intrinsic_atan(x); }   // atan :contentReference[oaicite:14]{index=14}
    // public fn atan2(y: float, x: float) => float { return intrinsic_atan2(y, x); } // atan2 :contentReference[oaicite:15]{index=15}

    // // Hyperbolic
    // public fn sinh(x: float)  => float { return intrinsic_sinh(x); }   // sinh :contentReference[oaicite:16]{index=16}
    // public fn cosh(x: float)  => float { return intrinsic_cosh(x); }   // cosh :contentReference[oaicite:17]{index=17}
    // public fn tanh(x: float)  => float { return intrinsic_tanh(x); }   // tanh :contentReference[oaicite:18]{index=18}
    // public fn asinh(x: float) => float { return intrinsic_asinh(x); }  // asinh :contentReference[oaicite:19]{index=19}
    // public fn acosh(x: float) => float { return intrinsic_acosh(x); }  // acosh :contentReference[oaicite:20]{index=20}
    // public fn atanh(x: float) => float { return intrinsic_atanh(x); }  // atanh :contentReference[oaicite:21]{index=21}

    // // Exponential & Logarithmic
    // public fn exp(x: float)    => float { return intrinsic_exp(x); }   // e^x :contentReference[oaicite:22]{index=22}
    // public fn log(x: float)    => float { return intrinsic_log(x); }   // natural log :contentReference[oaicite:23]{index=23}
    // public fn log2(x: float)   => float { return intrinsic_log2(x); }  // log base 2 :contentReference[oaicite:24]{index=24}
    // public fn log10(x: float)  => float { return intrinsic_log10(x); }// log10 :contentReference[oaicite:25]{index=25}
    // public fn exp2(x: float)   => float { return intrinsic_exp2(x); }  // 2^x :contentReference[oaicite:26]{index=26}
    // public fn hypot(x: float, y: float) => float { return intrinsic_hypot(x, y); } // sqrt(x²+y²) :contentReference[oaicite:27]{index=27}

    // // Special Functions
    // public fn erf(x: float)    => float { return intrinsic_erf(x); }   // error function :contentReference[oaicite:28]{index=28}
    // public fn erfc(x: float)   => float { return intrinsic_erfc(x); }  // complementary erf :contentReference[oaicite:29]{index=29}
    // public fn tgamma(x: float) => float { return intrinsic_tgamma(x); }// gamma function :contentReference[oaicite:30]{index=30}
    // public fn lgamma(x: float) => float { return intrinsic_lgamma(x); }// log-gamma :contentReference[oaicite:31]{index=31}

    // // Combinatorics & Number Theory
    // public fn factorial(n: int) => int {
    //     let result: int = 1;
    //     for (let i = 2; i <= n; i += 1) {
    //         result *= i;
    //     }
    //     return result;
    // } // factorial :contentReference[oaicite:32]{index=32}
    // public fn gcd(a: int, b: int) => int {
    //     // Euclidean algorithm
    //     while (b != 0) {
    //         let t: int = b;
    //         b = a % b;
    //         a = t;
    //     }
    //     return a;
    // } // gcd :contentReference[oaicite:33]{index=33}

    // // Angle Conversion
    public fn radians(deg: float) => float { return deg * (pi / 180.0); }
    // public fn degrees(rad: float) => float { return rad * (180.0 / pi); }

    // Random
    // public fn random() => float   { return intrinsic_random(); }           // [0,1) uniform
    // public fn randint(min: int, max: int) => int {
    //     return floor(intrinsic_random() * float(max - min + 1)) as int + min;
    // } // integer uniform

    // // Helper intrinsic declarations (to be bound in the runtime)
    // private intrinsic fn intrinsic_pow(x: float, y: float) => float;
    // private intrinsic fn intrinsic_abs(x: int) => int;
    // private intrinsic fn intrinsic_fabs(x: float) => float;
    // private intrinsic fn intrinsic_sqrt(x: float) => float;
    // private intrinsic fn intrinsic_cbrt(x: float) => float;
    // private intrinsic fn intrinsic_floor(x: float) => float;
    // private intrinsic fn intrinsic_ceil(x: float) => float;
    // private intrinsic fn intrinsic_nearest_int(x: float) => int;
    // private intrinsic fn intrinsic_sin(x: float) => float;
    // private intrinsic fn intrinsic_cos(x: float) => float;
    // private intrinsic fn intrinsic_tan(x: float) => float;
    // private intrinsic fn intrinsic_asin(x: float) => float;
    // private intrinsic fn intrinsic_acos(x: float) => float;
    // private intrinsic fn intrinsic_atan(x: float) => float;
    // private intrinsic fn intrinsic_atan2(y: float, x: float) => float;
    // private intrinsic fn intrinsic_sinh(x: float) => float;
    // private intrinsic fn intrinsic_cosh(x: float) => float;
    // private intrinsic fn intrinsic_tanh(x: float) => float;
    // private intrinsic fn intrinsic_asinh(x: float) => float;
    // private intrinsic fn intrinsic_acosh(x: float) => float;
    // private intrinsic fn intrinsic_atanh(x: float) => float;
    // private intrinsic fn intrinsic_exp(x: float) => float;
    // private intrinsic fn intrinsic_log(x: float) => float;
    // private intrinsic fn intrinsic_log2(x: float) => float;
    // private intrinsic fn intrinsic_log10(x: float) => float;
    // private intrinsic fn intrinsic_exp2(x: float) => float;
    // private intrinsic fn intrinsic_hypot(x: float, y: float) => float;
    // private intrinsic fn intrinsic_erf(x: float) => float;
    // private intrinsic fn intrinsic_erfc(x: float) => float;
    // private intrinsic fn intrinsic_tgamma(x: float) => float;
    // private intrinsic fn intrinsic_lgamma(x: float) => float;
    // private intrinsic fn intrinsic_random() => float;
}
