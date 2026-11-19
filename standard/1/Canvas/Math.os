// ---------------------------------------------------------------------------------------------
// small math (no math module assumed)
// ---------------------------------------------------------------------------------------------
let PI: float = 3.14159265;

function msqrt(x: float) => float {
    if (x <= 0.0) return 0.0;
    let g: float = (x + 1.0) * 0.5;
    let i: int = 0;
    while (i < 20) { g = 0.5 * (g + x / g); i = i + 1; }
    return g;
}

function mwrap(a: float) => float {
    let r: float = a;
    while (r > PI) { r = r - 2.0 * PI; }
    while (r < 0.0 - PI) { r = r + 2.0 * PI; }
    return r;
}

function msin(a: float) => float {
    let x: float = mwrap(a);
    let x2: float = x * x;
    let t: float = x;
    let s: float = x;
    t = t * (0.0 - x2) / 6.0;      s = s + t;
    t = t * (0.0 - x2) / 20.0;     s = s + t;
    t = t * (0.0 - x2) / 42.0;     s = s + t;
    t = t * (0.0 - x2) / 72.0;     s = s + t;
    t = t * (0.0 - x2) / 110.0;    s = s + t;
    t = t * (0.0 - x2) / 156.0;    s = s + t;
    t = t * (0.0 - x2) / 210.0;    s = s + t;
    return s;
}

function mcos(a: float) => float { return msin(a + PI * 0.5); }
function mtan(a: float) => float { return msin(a) / mcos(a); }

// ---- 4x4 matrices: column-major float*(16). `out` must not alias inputs. ----
function matIdentity(m: float*) => void {
    let i: int = 0;
    while (i < 16) { m[i] = 0.0; i = i + 1; }
    m[0] = 1.0; m[5] = 1.0; m[10] = 1.0; m[15] = 1.0;
}

function matMul(out: float*, a: float*, b: float*) => void {
    let c: int = 0;
    while (c < 4) {
        let r: int = 0;
        while (r < 4) {
            let s: float = 0.0;
            let k: int = 0;
            while (k < 4) { s = s + a[k * 4 + r] * b[c * 4 + k]; k = k + 1; }
            out[c * 4 + r] = s;
            r = r + 1;
        }
        c = c + 1;
    }
}

function matTranslate(m: float*, x: float, y: float, z: float) => void {
    matIdentity(m); m[12] = x; m[13] = y; m[14] = z;
}
function matScale(m: float*, x: float, y: float, z: float) => void {
    matIdentity(m); m[0] = x; m[5] = y; m[10] = z;
}
function matRotateY(m: float*, a: float) => void {
    matIdentity(m);
    let c: float = mcos(a); let s: float = msin(a);
    m[0] = c; m[2] = 0.0 - s; m[8] = s; m[10] = c;
}
function matRotateX(m: float*, a: float) => void {
    matIdentity(m);
    let c: float = mcos(a); let s: float = msin(a);
    m[5] = c; m[6] = s; m[9] = 0.0 - s; m[10] = c;
}
function matPerspective(m: float*, fovDeg: float, aspect: float, near: float, far: float) => void {
    let i: int = 0;
    while (i < 16) { m[i] = 0.0; i = i + 1; }
    let f: float = 1.0 / mtan(fovDeg * PI / 360.0);
    m[0] = f / aspect; m[5] = f;
    m[10] = (far + near) / (near - far); m[11] = 0.0 - 1.0;
    m[14] = 2.0 * far * near / (near - far);
}
function matLookAt(m: float*, ex: float, ey: float, ez: float, tx: float, ty: float, tz: float) => void {
    let fx: float = tx - ex; let fy: float = ty - ey; let fz: float = tz - ez;
    let fl: float = msqrt(fx * fx + fy * fy + fz * fz);
    if (fl == 0.0) fl = 1.0;
    fx = fx / fl; fy = fy / fl; fz = fz / fl;
    // up = (0,1,0); s = f x up
    let sx: float = 0.0 - fz; let sy: float = 0.0; let sz: float = fx;
    let sl: float = msqrt(sx * sx + sz * sz);
    if (sl == 0.0) { sx = 1.0; sz = 0.0; sl = 1.0; }
    sx = sx / sl; sz = sz / sl;
    // u = s x f
    let ux: float = sy * fz - sz * fy;
    let uy: float = sz * fx - sx * fz;
    let uz: float = sx * fy - sy * fx;
    matIdentity(m);
    m[0] = sx; m[4] = sy; m[8] = sz;
    m[1] = ux; m[5] = uy; m[9] = uz;
    m[2] = 0.0 - fx; m[6] = 0.0 - fy; m[10] = 0.0 - fz;
    m[12] = 0.0 - (sx * ex + sy * ey + sz * ez);
    m[13] = 0.0 - (ux * ex + uy * ey + uz * ez);
    m[14] = fx * ex + fy * ey + fz * ez;
}

function chan(c: u32, shift: int) => float {
    let v: u32 = (c >> (shift as u32)) & 255;
    return (v as float) / 255.0;
}
