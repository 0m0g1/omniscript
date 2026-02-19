module Math {
    // Constants (double, float, long double)
    public const pi:    double = 3.141592653589793238462643383279502884;
    public const pi_f:  float  = 3.1415927F;
    public const pi_l:  f80    = 3.141592653589793238462643383279502884L;

    public const e:     double = 2.718281828459045235360287471352662498;
    public const e_f:   float  = 2.7182817F;
    public const e_l:   f80    = 2.718281828459045235360287471352662498L;

    public const log2e:  double = 1.44269504088896340735992468100189214;
    public const log2e_f:float  = 1.4426950F;
    public const log2e_l:f80    = 1.44269504088896340735992468100189214L;

    public const log10e:  double = 0.434294481903251827651128918916605082;
    public const log10e_f:float  = 0.43429449F;
    public const log10e_l:f80    = 0.434294481903251827651128918916605082L;

    public const ln2:   double = 0.693147180559945309417232121458176568;
    public const ln2_f: float  = 0.6931472F;
    public const ln2_l: f80    = 0.693147180559945309417232121458176568L;

    public const ln10:  double = 2.30258509299404568401799145468436421;
    public const ln10_f:float  = 2.3025851F;
    public const ln10_l:f80    = 2.30258509299404568401799145468436421L;

    public const sqrt2:   double = 1.41421356237309504880168872420969808;
    public const sqrt2_f: float  = 1.4142136F;
    public const sqrt2_l: f80    = 1.41421356237309504880168872420969808L;

    public const sqrt1_2:   double = 0.707106781186547524400844362104849039;
    public const sqrt1_2_f: float  = 0.7071068F;
    public const sqrt1_2_l: f80    = 0.707106781186547524400844362104849039L;

    public const inf:  double = 1.0 / 0.0;
    public const nan:  double = 0.0 / 0.0;

    // Floating-point constants
    public const huge_val: double = inf;

    // Elementary functions

    extern "C" {
        fn powf(x: float, y: float) => float;
        fn pow(x: double, y: double) => double;
        fn powl(x: f80, y: f80) => f80;
        fn sqrtf(x: float) => float;
        fn sqrt(x: double) => double;
        fn sqrtl(x: f80) => f80;
    
        fn cbrtf(x: float) => float;
        fn cbrt(x: double) => double;
        fn cbrtl(x: f80) => f80;
    
        fn hypotf(x: float, y: float) => float;
        fn hypot(x: double, y: double) => double;
        fn hypotl(x: f80, y: f80) => f80;
    
        // Exponential and logarithmic
        fn expm1f(x: float) => float;
        fn expm1(x: double) => double;
        fn expm1l(x: f80) => f80;
    
        fn log1pf(x: float) => float;
        fn log1p(x: double) => double;
        fn log1pl(x: f80) => f80;
    
        fn exp2f(x: float) => float;
        fn exp2(x: double) => double;
        fn exp2l(x: f80) => f80;
    
        fn expf(x: float) => float;
        fn exp(x: double) => double;
        fn expl(x: f80) => f80;
    
        fn logbf(x: float) => float;
        fn logb(x: double) => double;
        fn logbl(x: f80) => f80;
    
        fn ilogbf(x: float) => int;
        fn ilogb(x: double) => int;
        fn ilogbl(x: f80) => int;
    
        fn log2f(x: float) => float;
        fn log2(x: double) => double;
        fn log2l(x: f80) => f80;
    
        fn log10f(x: float) => float;
        fn log10(x: double) => double;
        fn log10l(x: f80) => f80;
    
        fn logf(x: float) => float;
        fn log(x: double) => double;
        fn logl(x: f80) => f80;
    
        // Error/gamma
        fn erf(x: double) => double;
        fn erff(x: float) => float;
        fn erfcl(x: f80) => f80;
    
        fn erfc(x: double) => double;
        fn erfcf(x: float) => float;
        fn erfcl(x: f80) => f80;
    
        fn tgamma(x: double) => double;
        fn tgammaf(x: float) => float;
        fn tgammal(x: f80) => f80;
    
        fn lgamma(x: double) => double;
        fn lgammaf(x: float) => float;
        fn lgammal(x: f80) => f80;
    
        // Arithmetic
        fn fmaf(a: float, b: float, c: float) => float;
        fn fma(a: double, b: double, c: double) => double;
        fn fmal(a: f80, b: f80, c: f80) => f80;
    
        fn fmodf(x: float, y: float) => float;
        fn fmod(x: double, y: double) => double;
        fn fmodl(x: f80, y: f80) => f80;
    
        fn remainderf(x: float, y: float) => float;
        fn remainder(x: double, y: double) => double;
        fn remainderl(x: f80, y: f80) => f80;
    
        fn remquof(x: float, y: float, quo: &int) => float;
        fn remquo(x: double, y: double, quo: &int) => double;
        fn remquol(x: f80, y: f80, quo: &int) => f80;
    
        fn fdimf(x: float, y: float) => float;
        fn fdim(x: double, y: double) => double;
        fn fdiml(x: f80, y: f80) => f80;
    
        // Min / max / signs
        fn fminf(x: float, y: float) => float;
        fn fmin(x: double, y: double) => double;
        fn fminl(x: f80, y: f80) => f80;
    
        fn fmaxf(x: float, y: float) => float;
        fn fmax(x: double, y: double) => double;
        fn fmaxl(x: f80, y: f80) => f80;
    
        fn copysignf(x: float, y: float) => float;
        fn copysign(x: double, y: double) => double;
        fn copysignl(x: f80, y: f80) => f80;
    
        fn nextafterf(x: float, y: float) => float;
        fn nextafter(x: double, y: double) => double;
        fn nextafterl(x: f80, y: f80) => f80;
    
        // Trigonometry
        fn sinf(x: float) => float;
        fn sin(x: double) => double;
        fn sinl(x: f80) => f80;
    
        fn cosf(x: float) => float;
        fn cos(x: double) => double;
        fn cosl(x: f80) => f80;
    
        fn tanf(x: float) => float;
        fn tan(x: double) => double;
        fn tanl(x: f80) => f80;
    
        fn asinf(x: float) => float;
        fn asin(x: double) => double;
        fn asinl(x: f80) => f80;
    
        fn acosf(x: float) => float;
        fn acos(x: double) => double;
        fn acosl(x: f80) => f80;
    
        fn atanf(x: float) => float;
        fn atan(x: double) => double;
        fn atanl(x: f80) => f80;
    
        fn atan2f(y: float, x: float) => float;
        fn atan2(y: double, x: double) => double;
        fn atan2l(y: f80, x: f80) => f80;
    
        // Hyperbolic
        fn sinhf(x: float) => float;
        fn sinh(x: double) => double;
        fn sinhl(x: f80) => f80;
    
        fn coshf(x: float) => float;
        fn cosh(x: double) => double;
        fn coshl(x: f80) => f80;
    
        fn tanhf(x: float) => float;
        fn tanh(x: double) => double;
        fn tanhl(x: f80) => f80;
    
        fn asinhf(x: float) => float;
        fn asinh(x: double) => double;
        fn asinhl(x: f80) => f80;
    
        fn acoshf(x: float) => float;
        fn acosh(x: double) => double;
        fn acoshl(x: f80) => f80;
    
        fn atanhf(x: float) => float;
        fn atanh(x: double) => double;
        fn atanhl(x: f80) => f80;
    
        // Nearest integer
        fn ceilf(x: float) => float;
        fn ceil(x: double) => double;
        fn ceill(x: f80) => f80;
    
        fn floorf(x: float) => float;
        fn floor(x: double) => double;
        fn floorl(x: f80) => f80;
    
        fn truncf(x: float) => float;
        fn trunc(x: double) => double;
        fn truncl(x: f80) => f80;
    
        fn roundf(x: float) => float;
        fn round(x: double) => double;
        fn roundl(x: f80) => f80;
    
        fn nearbyintf(x: float) => float;
        fn nearbyint(x: double) => double;
        fn nearbyintl(x: f80) => f80;
    
        fn rintf(x: float) => float;
        fn rint(x: double) => double;
        fn rintl(x: f80) => f80;
    
        // Decomposition and scaling
        fn modff(x: float, ip: &float) => float;
        fn modf(x: double, ip: &double) => double;
        fn modfl(x: f80, ip: &f80) => f80;
    
        fn frexpf(x: float, exp: &int) => float;
        fn frexp(x: double, exp: &int) => double;
        fn frexpl(x: f80, exp: &int) => f80;
    
        fn ldexpf(x: float, exp: int) => float;
        fn ldexp(x: double, exp: int) => double;
        fn ldexpl(x: f80, exp: int) => f80;
    
        fn scalbnf(x: float, n: int) => float;
        fn scalbn(x: double, n: int) => double;
        fn scalbnl(x: f80, n: int) => f80;
    
        // Integer arithmetic
        fn abs(x: int) => int;
        fn labs(x: i64) => i64;
        fn llabs(x: i64) => i64;
    
        // // Floating point absolute value
        fn fabsf(x: float) => float;
        fn fabs(x: double) => double;
        fn fabsl(x: f80) => f80;
    
        // C23 Bit-Manipulation Helpers (integer type-generic macros)
        fn stdc_bit_floor_u8(x: u8) => u8;
        fn stdc_bit_floor_u16(x: u16) => u16;
        fn stdc_bit_floor_u32(x: u32) => u32;
        fn stdc_bit_floor_u64(x: u64) => u64;
        // Generic macro: stdc_bit_floor(x)
    
        fn stdc_bit_ceil_u8(x: u8) => u8;
        fn stdc_bit_ceil_u16(x: u16) => u16;
        fn stdc_bit_ceil_u32(x: u32) => u32;
        fn stdc_bit_ceil_u64(x: u64) => u64;
        // Generic macro: stdc_bit_ceil(x)
    
        // IEEE 754 fused subtraction
        fn fsubf(x: f32, y: f32) => f32;
        fn fsub(x: f64, y: f64) => f64;
        fn fsubl(x: f80, y: f80) => f80;
    
        // Extended-precision remainder
        fn remainderf128(x: f128, y: f128) => f128;
    }


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
