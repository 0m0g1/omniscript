module Math {
    // Constants
    public const pi: float   = 3.141592653589793;
    public const e: float    = 2.718281828459045;
    public const inf: double = 1.0 / 0.0;
    public const nan: double = 0.0 / 0.0;

    // Extern "C" function declarations

    // Basic Arithmetic
    extern "C" fn pow(x: float, y: float) => float;
    extern "C" fn abs(x: int) => int;

    // Floating point variants
    extern "C" fn fabs(x: float) => float;
    extern "C" fn sqrt(x: float) => float;
    extern "C" fn floor(x: float) => float;
    extern "C" fn ceil(x: float) => float;
    extern "C" fn trunc(x: float) => float;
    extern "C" fn round(x: float) => float;

    // Trigonometric
    extern "C" fn sin(x: float) => float;
    extern "C" fn cos(x: float) => float;
    extern "C" fn tan(x: float) => float;
    extern "C" fn asin(x: float) => float;
    extern "C" fn acos(x: float) => float;
    extern "C" fn atan(x: float) => float;
    extern "C" fn atan2(y: float, x: float) => float;

    // Hyperbolic
    extern "C" fn sinh(x: float) => float;
    extern "C" fn cosh(x: float) => float;
    extern "C" fn tanh(x: float) => float;

    // Exponential and Logarithmic
    extern "C" fn exp(x: float) => float;
    extern "C" fn exp2(x: float) => float;
    extern "C" fn log(x: float) => float;
    extern "C" fn log2(x: float) => float;
    extern "C" fn log10(x: float) => float;

    // Floating point remainder
    extern "C" fn fmod(x: float, y: float) => float;

    // Wrapper functions implemented using externs or basic logic

    public fn cbrt(x: float) => float {
        // return x < 0.0 ? -pow(-x, 1.0 / 3.0) : pow(x, 1.0 / 3.0);
        return 0;
    }

    public fn hypot(x: float, y: float) => float {
        return sqrt(x * x + y * y);
    }

    public fn erf(x: float) => float {
        // Abramowitz and Stegun approximation
        // let a1: float = 0.254829592;
        // let a2: float = -0.284496736;
        // let a3: float = 1.421413741;
        // let a4: float = -1.453152027;
        // let a5: float = 1.061405429;
        // let p: float = 0.3275911;

        // let sign = x < 0.0 ? -1.0 : 1.0;
        // let abs_x = fabs(x);

        // let t = 1.0 / (1.0 + p * abs_x);
        // let y = 1.0 - (((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * exp(-abs_x * abs_x);

        // return sign * y;
        return 0;
    }

    public fn erfc(x: float) => float {
        // return 1.0 - erf(x);
        return 0;
    }

    public fn factorial(n: int) => int {
        // if (n >= 0) {
        //     let result: int = 1;
        //     for (let i = 2; i <= n; i += 1) {
        //         result *= i;
        //     }
        //     return result;
        // }
        return 0;
        // throw an error?
    }

    public fn gcd(a: int, b: int) => int {
        // var x = abs(a);
        // var y = abs(b);
        // while (y != 0) {
        //     let t: int = y;
        //     y = x % y;
        //     x = t;
        // }
        // return x;
        return 0;
    }

    public fn lcm(a: int, b: int) => int {
        // return abs(a * b) / gcd(a, b);
        return 0;
    }

    public fn radians(deg: float) => float {
        return deg * (pi / 180.0);
    }

    public fn degrees(rad: float) => float {
        return rad * (180.0 / pi);
    }
}
