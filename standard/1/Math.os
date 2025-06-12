module Math {
    // Constants (double, float, long double)
    public const pi:    double = 3.141592653589793238462643383279502884;
    public const pi_f:  float  = 3.1415927F;
    // public const pi_l:  f80    = 3.141592653589793238462643383279502884L;

    public const e:     double = 2.718281828459045235360287471352662498;
    public const e_f:   float  = 2.7182817F;
    // public const e_l:   f80    = 2.718281828459045235360287471352662498L;

    public const log2e:  double = 1.44269504088896340735992468100189214;
    public const log2e_f:float  = 1.4426950F;
    // public const log2e_l:f80    = 1.44269504088896340735992468100189214L;

    public const log10e:  double = 0.434294481903251827651128918916605082;
    public const log10e_f:float  = 0.43429449F;
    // public const log10e_l:f80    = 0.434294481903251827651128918916605082L;

    public const ln2:   double = 0.693147180559945309417232121458176568;
    public const ln2_f: float  = 0.6931472F;
    // public const ln2_l: f80    = 0.693147180559945309417232121458176568L;

    public const ln10:  double = 2.30258509299404568401799145468436421;
    public const ln10_f:float  = 2.3025851F;
    // public const ln10_l:f80    = 2.30258509299404568401799145468436421L;

    public const sqrt2:   double = 1.41421356237309504880168872420969808;
    public const sqrt2_f: float  = 1.4142136F;
    // public const sqrt2_l: f80    = 1.41421356237309504880168872420969808L;

    public const sqrt1_2:   double = 0.707106781186547524400844362104849039;
    public const sqrt1_2_f: float  = 0.7071068F;
    // public const sqrt1_2_l: f80    = 0.707106781186547524400844362104849039L;

    public const inf:  double = 1.0 / 0.0;
    public const nan:  double = 0.0 / 0.0;

    // Floating-point constants
    public const huge_val: double = inf;

    // Elementary functions

    extern "C" {
        fn powf(x: float, y: float) => float;
        fn pow(x: double, y: double) => double;
        fn powl(x: f80, y: f80) => f80;
    }
    // extern "C" fn powf(x: float, y: float) => float;
    // extern "C" fn pow(x: double, y: double) => double;
    // extern "C" fn powl(x: f80, y: f80) => f80;

    extern "C" fn sqrtf(x: float) => float;
    extern "C" fn sqrt(x: double) => double;
    extern "C" fn sqrtl(x: f80) => f80;

    extern "C" fn cbrtf(x: float) => float;
    extern "C" fn cbrt(x: double) => double;
    extern "C" fn cbrtl(x: f80) => f80;

    extern "C" fn hypotf(x: float, y: float) => float;
    extern "C" fn hypot(x: double, y: double) => double;
    extern "C" fn hypotl(x: f80, y: f80) => f80;

    // Exponential and logarithmic
    extern "C" fn expm1f(x: float) => float;
    extern "C" fn expm1(x: double) => double;
    extern "C" fn expm1l(x: f80) => f80;

    extern "C" fn log1pf(x: float) => float;
    extern "C" fn log1p(x: double) => double;
    extern "C" fn log1pl(x: f80) => f80;

    extern "C" fn exp2f(x: float) => float;
    extern "C" fn exp2(x: double) => double;
    extern "C" fn exp2l(x: f80) => f80;

    extern "C" fn expf(x: float) => float;
    extern "C" fn exp(x: double) => double;
    extern "C" fn expl(x: f80) => f80;

    extern "C" fn logbf(x: float) => float;
    extern "C" fn logb(x: double) => double;
    extern "C" fn logbl(x: f80) => f80;

    extern "C" fn ilogbf(x: float) => int;
    extern "C" fn ilogb(x: double) => int;
    extern "C" fn ilogbl(x: f80) => int;

    extern "C" fn log2f(x: float) => float;
    extern "C" fn log2(x: double) => double;
    extern "C" fn log2l(x: f80) => f80;

    extern "C" fn log10f(x: float) => float;
    extern "C" fn log10(x: double) => double;
    extern "C" fn log10l(x: f80) => f80;

    extern "C" fn logf(x: float) => float;
    extern "C" fn log(x: double) => double;
    extern "C" fn logl(x: f80) => f80;

    // Error/gamma
    extern "C" fn erf(x: double) => double;
    extern "C" fn erff(x: float) => float;
    extern "C" fn erfcl(x: f80) => f80;

    extern "C" fn erfc(x: double) => double;
    extern "C" fn erfcf(x: float) => float;
    extern "C" fn erfcl(x: f80) => f80;

    extern "C" fn tgamma(x: double) => double;
    extern "C" fn tgammaf(x: float) => float;
    extern "C" fn tgammal(x: f80) => f80;

    extern "C" fn lgamma(x: double) => double;
    extern "C" fn lgammaf(x: float) => float;
    extern "C" fn lgammal(x: f80) => f80;

    // Arithmetic
    extern "C" fn fmaf(a: float, b: float, c: float) => float;
    extern "C" fn fma(a: double, b: double, c: double) => double;
    extern "C" fn fmal(a: f80, b: f80, c: f80) => f80;

    extern "C" fn fmodf(x: float, y: float) => float;
    extern "C" fn fmod(x: double, y: double) => double;
    extern "C" fn fmodl(x: f80, y: f80) => f80;

    extern "C" fn remainderf(x: float, y: float) => float;
    extern "C" fn remainder(x: double, y: double) => double;
    extern "C" fn remainderl(x: f80, y: f80) => f80;

    extern "C" fn remquof(x: float, y: float, quo: &int) => float;
    extern "C" fn remquo(x: double, y: double, quo: &int) => double;
    extern "C" fn remquol(x: f80, y: f80, quo: &int) => f80;

    extern "C" fn fdimf(x: float, y: float) => float;
    extern "C" fn fdim(x: double, y: double) => double;
    extern "C" fn fdiml(x: f80, y: f80) => f80;

    // Min / max / signs
    extern "C" fn fminf(x: float, y: float) => float;
    extern "C" fn fmin(x: double, y: double) => double;
    extern "C" fn fminl(x: f80, y: f80) => f80;

    extern "C" fn fmaxf(x: float, y: float) => float;
    extern "C" fn fmax(x: double, y: double) => double;
    extern "C" fn fmaxl(x: f80, y: f80) => f80;

    extern "C" fn copysignf(x: float, y: float) => float;
    extern "C" fn copysign(x: double, y: double) => double;
    extern "C" fn copysignl(x: f80, y: f80) => f80;

    extern "C" fn nextafterf(x: float, y: float) => float;
    extern "C" fn nextafter(x: double, y: double) => double;
    extern "C" fn nextafterl(x: f80, y: f80) => f80;

    // Trigonometry
    extern "C" fn sinf(x: float) => float;
    extern "C" fn sin(x: double) => double;
    extern "C" fn sinl(x: f80) => f80;

    extern "C" fn cosf(x: float) => float;
    extern "C" fn cos(x: double) => double;
    extern "C" fn cosl(x: f80) => f80;

    extern "C" fn tanf(x: float) => float;
    extern "C" fn tan(x: double) => double;
    extern "C" fn tanl(x: f80) => f80;

    extern "C" fn asinf(x: float) => float;
    extern "C" fn asin(x: double) => double;
    extern "C" fn asinl(x: f80) => f80;

    extern "C" fn acosf(x: float) => float;
    extern "C" fn acos(x: double) => double;
    extern "C" fn acosl(x: f80) => f80;

    extern "C" fn atanf(x: float) => float;
    extern "C" fn atan(x: double) => double;
    extern "C" fn atanl(x: f80) => f80;

    extern "C" fn atan2f(y: float, x: float) => float;
    extern "C" fn atan2(y: double, x: double) => double;
    extern "C" fn atan2l(y: f80, x: f80) => f80;

    // Hyperbolic
    extern "C" fn sinhf(x: float) => float;
    extern "C" fn sinh(x: double) => double;
    extern "C" fn sinhl(x: f80) => f80;

    extern "C" fn coshf(x: float) => float;
    extern "C" fn cosh(x: double) => double;
    extern "C" fn coshl(x: f80) => f80;

    extern "C" fn tanhf(x: float) => float;
    extern "C" fn tanh(x: double) => double;
    extern "C" fn tanhl(x: f80) => f80;

    extern "C" fn asinhf(x: float) => float;
    extern "C" fn asinh(x: double) => double;
    extern "C" fn asinhl(x: f80) => f80;

    extern "C" fn acoshf(x: float) => float;
    extern "C" fn acosh(x: double) => double;
    extern "C" fn acoshl(x: f80) => f80;

    extern "C" fn atanhf(x: float) => float;
    extern "C" fn atanh(x: double) => double;
    extern "C" fn atanhl(x: f80) => f80;

    // Nearest integer
    extern "C" fn ceilf(x: float) => float;
    extern "C" fn ceil(x: double) => double;
    extern "C" fn ceill(x: f80) => f80;

    extern "C" fn floorf(x: float) => float;
    extern "C" fn floor(x: double) => double;
    extern "C" fn floorl(x: f80) => f80;

    extern "C" fn truncf(x: float) => float;
    extern "C" fn trunc(x: double) => double;
    extern "C" fn truncl(x: f80) => f80;

    extern "C" fn roundf(x: float) => float;
    extern "C" fn round(x: double) => double;
    extern "C" fn roundl(x: f80) => f80;

    extern "C" fn nearbyintf(x: float) => float;
    extern "C" fn nearbyint(x: double) => double;
    extern "C" fn nearbyintl(x: f80) => f80;

    extern "C" fn rintf(x: float) => float;
    extern "C" fn rint(x: double) => double;
    extern "C" fn rintl(x: f80) => f80;

    // Decomposition and scaling
    extern "C" fn modff(x: float, ip: &float) => float;
    extern "C" fn modf(x: double, ip: &double) => double;
    extern "C" fn modfl(x: f80, ip: &f80) => f80;

    extern "C" fn frexpf(x: float, exp: &int) => float;
    extern "C" fn frexp(x: double, exp: &int) => double;
    extern "C" fn frexpl(x: f80, exp: &int) => f80;

    extern "C" fn ldexpf(x: float, exp: int) => float;
    extern "C" fn ldexp(x: double, exp: int) => double;
    extern "C" fn ldexpl(x: f80, exp: int) => f80;

    extern "C" fn scalbnf(x: float, n: int) => float;
    extern "C" fn scalbn(x: double, n: int) => double;
    extern "C" fn scalbnl(x: f80, n: int) => f80;

    // Integer arithmetic
    extern "C" fn abs(x: int) => int;
    extern "C" fn labs(x: i64) => i64;
    extern "C" fn llabs(x: i64) => i64;

    // // Floating point absolute value
    extern "C" fn fabsf(x: float) => float;
    extern "C" fn fabs(x: double) => double;
    extern "C" fn fabsl(x: f80) => f80;

    // C23 Bit-Manipulation Helpers (integer type-generic macros)
    extern "C" fn stdc_bit_floor_u8(x: u8) => u8;
    extern "C" fn stdc_bit_floor_u16(x: u16) => u16;
    extern "C" fn stdc_bit_floor_u32(x: u32) => u32;
    extern "C" fn stdc_bit_floor_u64(x: u64) => u64;
    // Generic macro: stdc_bit_floor(x)

    extern "C" fn stdc_bit_ceil_u8(x: u8) => u8;
    extern "C" fn stdc_bit_ceil_u16(x: u16) => u16;
    extern "C" fn stdc_bit_ceil_u32(x: u32) => u32;
    extern "C" fn stdc_bit_ceil_u64(x: u64) => u64;
    // Generic macro: stdc_bit_ceil(x)

    // IEEE 754 fused subtraction
    extern "C" fn fsubf(x: f32, y: f32) => f32;
    extern "C" fn fsub(x: f64, y: f64) => f64;
    extern "C" fn fsubl(x: f80, y: f80) => f80;

    // Extended-precision remainder
    extern "C" fn remainderf128(x: f128, y: f128) => f128;

    // Helpers implemented in script
    public fn radians(deg: float) => float { return deg * (pi_f / 180.0); }
    public fn degrees(rad: float) => float { return rad * (180.0 / pi_f); }

    public fn factorial(n: int) => int {
        if (n < 0) return -1;
        let r = 1;
        // todo: support this 
        // for (let i = 2; i <= n; i++) r *= i;
        for (let i = 2; i <= n; i++) {
            r *= i;
        };
        return r;
    }

    // Implemented helpers (you already have)
    public fn cbrt(x: float) => float {
        return x < 0.0 ? -powf(-x, 1.0 / 3.0) : powf(x, 1.0 / 3.0);
    }

    public fn hypot(x: float, y: float) => float {
        return sqrtf(x * x + y * y);
    }

    public fn erf(x: float) => float {
        // Approximation
        let a1: float = 0.254829592;
        let a2: float = -0.284496736;
        let a3: float = 1.421413741;
        let a4: float = -1.453152027;
        let a5: float = 1.061405429;
        let p: float = 0.3275911;

        let sign : float = x < 0.0 ? -1.0 : 1.0;
        let abs_x = fabsf(x);

        let t = 1.0 / (1.0 + p * abs_x);
        let y = 1.0 - (((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * expf(-abs_x * abs_x);

        return sign * y;
    }

    public fn erfc(x: float) => float {
        return 1.0 - erff(x);
    }

    public fn gcd(a: int, b: int) => int {
        var x = abs(a);
        var y = abs(b);
        while (y != 0) {
            let t: int = y;
            y = x % y;
            x = t;
        }
        return x;
    }

    public fn lcm(a: int, b: int) => int {
        return abs(a * b) / gcd(a, b);
    }
}
