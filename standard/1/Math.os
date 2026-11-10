// standard/1/Math.os
// Tier 1: scalar math on double. Vec2/Vec3/Vec4/Mat4 come in a later file (Vec.os).
// Imports the raw C math under an alias so these wrappers can use the plain names (sqrt, sin, ...).
// Use Core.minF64 / maxF64 / clampF64 for min / max / clamp.

module CMath = import "standard/1/c/math.os";

const PI: double = 3.14159265358979323846;
const TAU: double = 6.28318530717958647692;
const E: double = 2.71828182845904523536;
const SQRT2: double = 1.41421356237309504880;
const EPSILON: double = 0.000000000001;
const INF: double = 1.0 / 0.0;
const NAN: double = 0.0 / 0.0;

// ---- classification ----
function isNaN(x: double) => bool { return x != x; }
function isInf(x: double) => bool { return x == INF || x == (0.0 - INF); }
function isFinite(x: double) => bool { return !isNaN(x) && !isInf(x); }

// ---- basics ----
function abs(x: double) => double { return CMath.fabs(x); }
function sign(x: double) => double {
    if (x > 0.0) return 1.0;
    if (x < 0.0) return 0.0 - 1.0;
    return 0.0;
}
function sqrt(x: double) => double { return CMath.sqrt(x); }
function cbrt(x: double) => double { return CMath.cbrt(x); }
function pow(x: double, y: double) => double { return CMath.pow(x, y); }
function exp(x: double) => double { return CMath.exp(x); }
function log(x: double) => double { return CMath.log(x); }
function log2(x: double) => double { return CMath.log2(x); }
function log10(x: double) => double { return CMath.log10(x); }
function hypot(x: double, y: double) => double { return CMath.hypot(x, y); }

// ---- rounding ----
function floor(x: double) => double { return CMath.floor(x); }
function ceil(x: double) => double { return CMath.ceil(x); }
function round(x: double) => double { return CMath.round(x); }   // halves round away from zero
function trunc(x: double) => double { return CMath.trunc(x); }
function fract(x: double) => double { return x - CMath.floor(x); }

// C-style remainder (sign follows x) and a mathematical modulo (result in [0, m) for m > 0).
function fmod(x: double, y: double) => double { return CMath.fmod(x, y); }
function mod(x: double, m: double) => double {
    let r = CMath.fmod(x, m);
    if (r < 0.0) r = r + m;
    return r;
}

// ---- trigonometry (radians) ----
function sin(x: double) => double { return CMath.sin(x); }
function cos(x: double) => double { return CMath.cos(x); }
function tan(x: double) => double { return CMath.tan(x); }
function asin(x: double) => double { return CMath.asin(x); }
function acos(x: double) => double { return CMath.acos(x); }
function atan(x: double) => double { return CMath.atan(x); }
function atan2(y: double, x: double) => double { return CMath.atan2(y, x); }
function sinh(x: double) => double { return CMath.sinh(x); }
function cosh(x: double) => double { return CMath.cosh(x); }
function tanh(x: double) => double { return CMath.tanh(x); }
function radians(deg: double) => double { return deg * (PI / 180.0); }
function degrees(rad: double) => double { return rad * (180.0 / PI); }

// ---- interpolation ----
function lerp(a: double, b: double, t: double) => double { return a + (b - a) * t; }
function inverseLerp(a: double, b: double, v: double) => double {
    if (a == b) return 0.0;
    return (v - a) / (b - a);
}
function remap(v: double, inLo: double, inHi: double, outLo: double, outHi: double) => double {
    return lerp(outLo, outHi, inverseLerp(inLo, inHi, v));
}
function smoothstep(edge0: double, edge1: double, x: double) => double {
    let t = inverseLerp(edge0, edge1, x);
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return t * t * (3.0 - 2.0 * t);
}

function approxEq(a: double, b: double, eps: double) => bool { return abs(a - b) <= eps; }
