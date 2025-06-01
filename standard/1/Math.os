module Math {
    // Constants
    const pi: f32 = 3.141592653589793;
    const e: float = 2.718281828459045;

    // // Basic Arithmetic
    public fn pow(x: float, y: float) => float {
        return intrinsic_pow(x, y);
    };
    
    private intrinsic fn intrinsic_pow(x: float, y: float) => float;
    

    // public fn powi(base: int = 1, exponent: int = 1) => int {
    //     let result: int = 1;
    //     for (let i = 0; i < exponent; i += 1) {
    //         result *= base;
    //     }
    //     return result;
    // }
    // public intrinsic fn abs(x: int) => int;
    // public intrinsic fn abs(x: float) => float;
    // public intrinsic fn sqrt(x: float) => float;
    // public intrinsic fn cbrt(x: float) => float;

    // // Rounding
    // public intrinsic fn floor(x: float) => float;
    // public intrinsic fn ceil(x: float) => float;
    // public intrinsic fn round(x: float) => int;

    // // Trigonometry
    // public intrinsic fn sin(x: float) => float;
    // public intrinsic fn cos(x: float) => float;
    // public intrinsic fn tan(x: float) => float;
    // public intrinsic fn asin(x: float) => float;
    // public intrinsic fn acos(x: float) => float;
    // public intrinsic fn atan(x: float) => float;
    // public intrinsic fn atan2(y: float, x: float) => float;

    // // Hyperbolic
    // public intrinsic fn sinh(x: float) => float;
    // public intrinsic fn cosh(x: float) => float;
    // public intrinsic fn tanh(x: float) => float;
    // public intrinsic fn asinh(x: float) => float;
    // public intrinsic fn acosh(x: float) => float;
    // public intrinsic fn atanh(x: float) => float;

    // // Exponential & Logarithmic
    // public intrinsic fn exp(x: float) => float;
    // public intrinsic fn log(x: float) => float;
    // public intrinsic fn log2(x: float) => float;
    // public intrinsic fn log10(x: float) => float;
    // public intrinsic fn exp2(x: float) => float;
    // public intrinsic fn hypot(x: float, y: float) => float;

    // // Special Functions
    // public intrinsic fn erf(x: float) => float;
    // public intrinsic fn erfc(x: float) => float;
    // public intrinsic fn tgamma(x: float) => float;
    // public intrinsic fn lgamma(x: float) => float;

    // // Combinatorics & Number Theory
    // public fn factorial(n: int) => int {
    //     let result: int = 1;
    //     for (let i = 2; i <= n; i += 1) {
    //         result *= i;
    //     }
    //     return result;
    // }
    // public fn gcd(a: int, b: int) => int {
    //     while (b != 0) {
    //         let t: int = b;
    //         b = a % b;
    //         a = t;
    //     }
    //     return a;
    // }

    // // Angle Conversion
    // public fn radians(deg: float) => float {
    //     return deg * (pi / 180.0);
    // }
    // public fn degrees(rad: float) => float {
    //     return rad * (180.0 / pi);
    // }

    // // Random
    // public intrinsic fn random() => float;
    // public fn randint(min: int, max: int) => int {
    //     return floor(random() * float(max - min + 1)) as int + min;
    // }
}
