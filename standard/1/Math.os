module Math {
    // Constants
    // public const pi: double   = 3.141592653589793;
    // public const e: double    = 2.718281828459045;
    // public const inf: double  = 1.0 / 0.0;
    // public const nan: double  = 0.0 / 0.0;

    // // Basic Arithmetic
    // extern "C" fn powf(x: float, y: float) => float;
    // extern "C" fn pow(x: double, y: double) => double;
    // // To Do: add double and long double types
    // // extern "C" fn powl(x: i64 double, y: i64 double) => f80;
    // // extern "C" fn powl(x: long double, y: long double) => f80;
    // extern "C" fn powl(x: f80, y: f80) => f80;

    // extern "C" fn abs(x: int) => int;
    // // To do: add long and long long types
    // // extern "C" fn labs(x: i64) => i64;
    // // extern "C" fn llabs(x: i64) => i64;
    // extern "C" fn labs(x: i64) => i64;
    // extern "C" fn llabs(x: i64) => i64;

    // // Floating point absolute value
    // extern "C" fn fabsf(x: float) => float;
    public fn fabsf(x: float) => float {
        return x < 0.0 ? -x : x;
    }
    // extern "C" fn fabs(x: double) => double;
    // // extern "C" fn fabsl(x: f80) => f80;

    // // Square root
    // extern "C" fn sqrtf(x: float) => float;
    // extern "C" fn sqrt(x: double) => double;
    // extern "C" fn sqrtl(x: f80) => f80;

    // // Floor
    // extern "C" fn floorf(x: float) => float;
    // extern "C" fn floor(x: double) => double;
    // extern "C" fn floorl(x: f80) => f80;

    // // Ceil
    // extern "C" fn ceilf(x: float) => float;
    // extern "C" fn ceil(x: double) => double;
    // extern "C" fn ceill(x: f80) => f80;

    // // Truncate
    // extern "C" fn truncf(x: float) => float;
    // extern "C" fn trunc(x: double) => double;
    // extern "C" fn truncl(x: f80) => f80;

    // // Round
    // extern "C" fn roundf(x: float) => float;
    // extern "C" fn round(x: double) => double;
    // extern "C" fn roundl(x: f80) => f80;

    // // Trigonometric
    // extern "C" fn sinf(x: float) => float;
    // extern "C" fn sin(x: double) => double;
    // extern "C" fn sinl(x: f80) => f80;

    // extern "C" fn cosf(x: float) => float;
    // extern "C" fn cos(x: double) => double;
    // extern "C" fn cosl(x: f80) => f80;

    // extern "C" fn tanf(x: float) => float;
    // extern "C" fn tan(x: double) => double;
    // extern "C" fn tanl(x: f80) => f80;

    // extern "C" fn asinf(x: float) => float;
    // extern "C" fn asin(x: double) => double;
    // extern "C" fn asinl(x: f80) => f80;

    // extern "C" fn acosf(x: float) => float;
    // extern "C" fn acos(x: double) => double;
    // extern "C" fn acosl(x: f80) => f80;

    // extern "C" fn atanf(x: float) => float;
    // extern "C" fn atan(x: double) => double;
    // extern "C" fn atanl(x: f80) => f80;

    // extern "C" fn atan2f(y: float, x: float) => float;
    // extern "C" fn atan2(y: double, x: double) => double;
    // extern "C" fn atan2l(y: f80, x: f80) => f80;

    // // Hyperbolic
    // extern "C" fn sinhf(x: float) => float;
    // extern "C" fn sinh(x: double) => double;
    // extern "C" fn sinhl(x: f80) => f80;

    // extern "C" fn coshf(x: float) => float;
    // extern "C" fn cosh(x: double) => double;
    // extern "C" fn coshl(x: f80) => f80;

    // extern "C" fn tanhf(x: float) => float;
    // extern "C" fn tanh(x: double) => double;
    // extern "C" fn tanhl(x: f80) => f80;

    // // Exponential
    extern "C" fn expf(x: float) => float;
    // extern "C" fn exp(x: double) => double;
    // extern "C" fn expl(x: f80) => f80;

    // extern "C" fn exp2f(x: float) => float;
    // extern "C" fn exp2(x: double) => double;
    // extern "C" fn exp2l(x: f80) => f80;

    // // Logarithmic
    // extern "C" fn logf(x: float) => float;
    // extern "C" fn log(x: double) => double;
    // extern "C" fn logl(x: f80) => f80;

    // extern "C" fn log2f(x: float) => float;
    // extern "C" fn log2(x: double) => double;
    // extern "C" fn log2l(x: f80) => f80;

    // extern "C" fn log10f(x: float) => float;
    // extern "C" fn log10(x: double) => double;
    // extern "C" fn log10l(x: f80) => f80;

    // // Floating point remainder
    // extern "C" fn fmodf(x: float, y: float) => float;
    // extern "C" fn fmod(x: double, y: double) => double;
    // extern "C" fn fmodl(x: f80, y: f80) => f80;

    // Wrapper functions implemented using externs or basic logic

    // public fn cbrt(x: float) => float {
    //     return x < 0.0 ? -powf(-x, 1.0 / 3.0) : powf(x, 1.0 / 3.0);
    // }

    // public fn hypot(x: float, y: float) => float {
    //     return sqrtf(x * x + y * y);
    // }

    // public fn erf(x: float) => float {
    //     // Abramowitz and Stegun approximation
    //     let a1: float = 0.254829592;
    //     let a2: float = -0.284496736;
    //     let a3: float = 1.421413741;
    //     let a4: float = -1.453152027;
    //     let a5: float = 1.061405429;
    //     let p: float = 0.3275911;

    //     let sign = x < 0.0 ? -1.0 : 1.0;
    //     let abs_x = fabsf(x);

    //     let t = 1.0 / (1.0 + p * abs_x);
    //     let y = 1.0 - (((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * expf(-abs_x * abs_x);

    //     return sign * y;
    // }

    // public fn erfc(x: float) => float {
    //     return 1.0 - erf(x);
    //     // return 0;
    // }

    public fn factorial(n: int) => int? {
        if (n >= 0) {
            let result: int = 1;
            for (let i = 2; i <= n; i += 1) {
                result *= i;
            }
            return result;
        }
        // return null;
        return -1;
    }

    // public fn gcd(a: int, b: int) => int {
    //     // var x = abs(a);
    //     // var y = abs(b);
    //     // while (y != 0) {
    //     //     let t: int = y;
    //     //     y = x % y;
    //     //     x = t;
    //     // }
    //     // return x;
    //     return 0;
    // }

    // public fn lcm(a: int, b: int) => int {
    //     // return abs(a * b) / gcd(a, b);
    //     return 0;
    // }

    // public fn radians(deg: float) => float {
    //     return deg * (pi as float / 180.0);
    // }

    // public fn degrees(rad: float) => float {
    //     return rad * (180.0 / pi as float);
    // }
}
