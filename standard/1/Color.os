// standard/1/Color.os
// Tier 1: packed colour. A colour is a u32 laid out 0xRRGGBBAA, the format Canvas Pen2D takes.
// Channels are ints 0..255; the F variants take f32 in 0..1 and clamp. Alpha is the last argument.
//
//   let c = Color.hsv(0.58 as f32, 0.85 as f32, 1.0 as f32, 255);   // blue, opaque
//   pen.fillRect(x, y, w, h, Color.withAlpha(c, 80));
//   let mid = Color.mix(Color.RED, Color.BLUE, 0.5 as f32);
//
// Float literals are f64 by default (see Vec.os), so f32 arguments need `as f32` or typed values.

module Math = import "standard/1/Math.os";

const ZERO: f32 = 0.0;
const ONE: f32 = 1.0;
const SIX: f32 = 6.0;
const BYTE_MAX: f32 = 255.0;

const TRANSPARENT: u32 = 0x00000000;
const BLACK: u32 = 0x000000FF;
const WHITE: u32 = 0xFFFFFFFF;
const RED: u32 = 0xFF0000FF;
const GREEN: u32 = 0x00FF00FF;
const BLUE: u32 = 0x0000FFFF;
const YELLOW: u32 = 0xFFFF00FF;

// ---- packing ----
// Each channel is masked to 0..255 (out-of-range values wrap, they do not clamp).
function rgba(r: int, g: int, b: int, a: int) => u32 {
    let ri: u32 = (r & 255) as u32;
    let gi: u32 = (g & 255) as u32;
    let bi: u32 = (b & 255) as u32;
    let ai: u32 = (a & 255) as u32;
    return (ri << 24) | (gi << 16) | (bi << 8) | ai;
}

function rgb(r: int, g: int, b: int) => u32 { return rgba(r, g, b, 255); }

// 0..1 -> 0..255, clamped (truncates, like a plain cast).
function toByte(x: f32) => int { return (Math.clampF32(x, ZERO, ONE) * BYTE_MAX) as int; }

function rgbaF(r: f32, g: f32, b: f32, a: f32) => u32 {
    return rgba(toByte(r), toByte(g), toByte(b), toByte(a));
}

// ---- channels ----
function red(c: u32) => int { return ((c >> 24) & 255) as int; }
function green(c: u32) => int { return ((c >> 16) & 255) as int; }
function blue(c: u32) => int { return ((c >> 8) & 255) as int; }
function alpha(c: u32) => int { return (c & 255) as int; }

function withAlpha(c: u32, a: int) => u32 { return (c & 0xFFFFFF00) | ((a & 255) as u32); }

// ---- HSV ----
// Hue wraps into [0, 1): 1.25 -> 0.25, -0.25 -> 0.75.
function wrapHue(h: f32) => f32 {
    let whole: f32 = Math.floor(h as f64) as f32;
    return h - whole;
}

// h wraps (see wrapHue); s and v are 0..1; a is 0..255.
function hsv(h: f32, s: f32, v: f32, a: int) => u32 {
    let hh: f32 = wrapHue(h);
    let h6: f32 = hh * SIX;
    let i: int = h6 as int;
    let f: f32 = h6 - (i as f32);
    let p: f32 = v * (ONE - s);
    let q: f32 = v * (ONE - s * f);
    let t: f32 = v * (ONE - s * (ONE - f));
    let r: f32 = v;
    let g: f32 = t;
    let b: f32 = p;
    if (i == 1) { r = q; g = v; b = p; }
    if (i == 2) { r = p; g = v; b = t; }
    if (i == 3) { r = p; g = q; b = v; }
    if (i == 4) { r = t; g = p; b = v; }
    if (i >= 5) { r = v; g = p; b = q; }
    return rgba(toByte(r), toByte(g), toByte(b), a);
}

// ---- blending ----
function lerpChannel(a: int, b: int, t: f32) => int {
    return a + (((b - a) as f32) * t) as int;
}

// Per-channel linear blend, t clamped to 0..1 (alpha included).
function mix(a: u32, b: u32, t: f32) => u32 {
    let tt: f32 = Math.clampF32(t, ZERO, ONE);
    return rgba(
        lerpChannel(red(a), red(b), tt),
        lerpChannel(green(a), green(b), tt),
        lerpChannel(blue(a), blue(b), tt),
        lerpChannel(alpha(a), alpha(b), tt)
    );
}
