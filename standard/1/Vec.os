// standard/1/Vec.os
// Tier 1: Vec2, Vec3, Vec4 (f32) and Mat4 (f32, column-major: element (row r, col c) = m[c * 4 + r]).
// Everything returns structs BY VALUE. Operands must have identical types, so f32 literals come from
// the typed constants below instead of bare 0.0 / 1.0 (bare literals default to f64).
//
//   let a = vec3(1.0 as f32, 2.0 as f32, 3.0 as f32);
//   let n = a.normalize();
//   let p = perspectiveVk(fovY, aspect, near, far).mul(lookAt(eye, center, up));
//
// Conventions: right-handed view space. perspectiveVk = depth 0..1 and Y pointing down (Vulkan clip space);
// perspectiveGL = depth -1..1, Y up.

module Math = import "standard/1/Math.os";

const ZERO: f32 = 0.0;
const ONE: f32 = 1.0;
const TWO: f32 = 2.0;
const HALF: f32 = 0.5;

function sqrtF(x: f32) => f32 { return Math.sqrt(x as f64) as f32; }

// ---------------------------------------------------------------- Vec2
struct Vec2 {
    x: f32 = 0.0;
    y: f32 = 0.0;

    add(o: Vec2) => Vec2 { return Vec2{ x: this.x + o.x, y: this.y + o.y }; }
    sub(o: Vec2) => Vec2 { return Vec2{ x: this.x - o.x, y: this.y - o.y }; }
    scale(s: f32) => Vec2 { return Vec2{ x: this.x * s, y: this.y * s }; }
    negate() => Vec2 { return Vec2{ x: -this.x, y: -this.y }; }
    dot(o: Vec2) => f32 { return this.x * o.x + this.y * o.y; }
    lengthSq() => f32 { return this.dot(*this); }
    length() => f32 { return sqrtF(this.lengthSq()); }
    distance(o: Vec2) => f32 { return this.sub(o).length(); }
    normalize() => Vec2 {
        let len = this.length();
        if (len == ZERO) return *this;
        return this.scale(ONE / len);
    }
    lerp(o: Vec2, t: f32) => Vec2 { return this.add(o.sub(*this).scale(t)); }
}
function vec2(x: f32, y: f32) => Vec2 { return Vec2{ x: x, y: y }; }

// ---------------------------------------------------------------- Vec3
struct Vec3 {
    x: f32 = 0.0;
    y: f32 = 0.0;
    z: f32 = 0.0;

    add(o: Vec3) => Vec3 { return Vec3{ x: this.x + o.x, y: this.y + o.y, z: this.z + o.z }; }
    sub(o: Vec3) => Vec3 { return Vec3{ x: this.x - o.x, y: this.y - o.y, z: this.z - o.z }; }
    scale(s: f32) => Vec3 { return Vec3{ x: this.x * s, y: this.y * s, z: this.z * s }; }
    negate() => Vec3 { return Vec3{ x: -this.x, y: -this.y, z: -this.z }; }
    dot(o: Vec3) => f32 { return this.x * o.x + this.y * o.y + this.z * o.z; }
    cross(o: Vec3) => Vec3 {
        return Vec3{
            x: this.y * o.z - this.z * o.y,
            y: this.z * o.x - this.x * o.z,
            z: this.x * o.y - this.y * o.x
        };
    }
    lengthSq() => f32 { return this.dot(*this); }
    length() => f32 { return sqrtF(this.lengthSq()); }
    distance(o: Vec3) => f32 { return this.sub(o).length(); }
    normalize() => Vec3 {
        let len = this.length();
        if (len == ZERO) return *this;
        return this.scale(ONE / len);
    }
    lerp(o: Vec3, t: f32) => Vec3 { return this.add(o.sub(*this).scale(t)); }
}
function vec3(x: f32, y: f32, z: f32) => Vec3 { return Vec3{ x: x, y: y, z: z }; }

// ---------------------------------------------------------------- Vec4
struct Vec4 {
    x: f32 = 0.0;
    y: f32 = 0.0;
    z: f32 = 0.0;
    w: f32 = 0.0;

    add(o: Vec4) => Vec4 { return Vec4{ x: this.x + o.x, y: this.y + o.y, z: this.z + o.z, w: this.w + o.w }; }
    sub(o: Vec4) => Vec4 { return Vec4{ x: this.x - o.x, y: this.y - o.y, z: this.z - o.z, w: this.w - o.w }; }
    scale(s: f32) => Vec4 { return Vec4{ x: this.x * s, y: this.y * s, z: this.z * s, w: this.w * s }; }
    dot(o: Vec4) => f32 { return this.x * o.x + this.y * o.y + this.z * o.z + this.w * o.w; }
    length() => f32 { return sqrtF(this.dot(*this)); }
    xyz() => Vec3 { return Vec3{ x: this.x, y: this.y, z: this.z }; }
}
function vec4(x: f32, y: f32, z: f32, w: f32) => Vec4 { return Vec4{ x: x, y: y, z: z, w: w }; }

// ---------------------------------------------------------------- Mat4
struct Mat4 {
    m: [16]f32;

    // this * o  (apply o first, then this)
    mul(o: Mat4) => Mat4 {
        let r: Mat4;
        for (let c: i32 = 0; c < 4; c++) {
            for (let row: i32 = 0; row < 4; row++) {
                let sum: f32 = ZERO;
                for (let k: i32 = 0; k < 4; k++) {
                    sum = sum + this.m[k * 4 + row] * o.m[c * 4 + k];
                }
                r.m[c * 4 + row] = sum;
            }
        }
        return r;
    }

    transpose() => Mat4 {
        let r: Mat4;
        for (let c: i32 = 0; c < 4; c++) {
            for (let row: i32 = 0; row < 4; row++) {
                r.m[row * 4 + c] = this.m[c * 4 + row];
            }
        }
        return r;
    }

    mulVec4(v: Vec4) => Vec4 {
        return Vec4{
            x: this.m[0] * v.x + this.m[4] * v.y + this.m[8] * v.z + this.m[12] * v.w,
            y: this.m[1] * v.x + this.m[5] * v.y + this.m[9] * v.z + this.m[13] * v.w,
            z: this.m[2] * v.x + this.m[6] * v.y + this.m[10] * v.z + this.m[14] * v.w,
            w: this.m[3] * v.x + this.m[7] * v.y + this.m[11] * v.z + this.m[15] * v.w
        };
    }

    // Transforms a point (w = 1) and drops w.
    transformPoint(p: Vec3) => Vec3 { return this.mulVec4(Vec4{ x: p.x, y: p.y, z: p.z, w: ONE }).xyz(); }
}

function mat4Zero() => Mat4 {
    let r: Mat4;
    for (let i: i32 = 0; i < 16; i++) r.m[i] = ZERO;
    return r;
}

function mat4Identity() => Mat4 {
    let r = mat4Zero();
    r.m[0] = ONE;
    r.m[5] = ONE;
    r.m[10] = ONE;
    r.m[15] = ONE;
    return r;
}

function translation(v: Vec3) => Mat4 {
    let r = mat4Identity();
    r.m[12] = v.x;
    r.m[13] = v.y;
    r.m[14] = v.z;
    return r;
}

function scaling(v: Vec3) => Mat4 {
    let r = mat4Identity();
    r.m[0] = v.x;
    r.m[5] = v.y;
    r.m[10] = v.z;
    return r;
}

// Angles in radians.
function rotationX(a: f32) => Mat4 {
    let c = Math.cos(a as f64) as f32;
    let s = Math.sin(a as f64) as f32;
    let r = mat4Identity();
    r.m[5] = c;
    r.m[6] = s;
    r.m[9] = -s;
    r.m[10] = c;
    return r;
}

function rotationY(a: f32) => Mat4 {
    let c = Math.cos(a as f64) as f32;
    let s = Math.sin(a as f64) as f32;
    let r = mat4Identity();
    r.m[0] = c;
    r.m[2] = -s;
    r.m[8] = s;
    r.m[10] = c;
    return r;
}

function rotationZ(a: f32) => Mat4 {
    let c = Math.cos(a as f64) as f32;
    let s = Math.sin(a as f64) as f32;
    let r = mat4Identity();
    r.m[0] = c;
    r.m[1] = s;
    r.m[4] = -s;
    r.m[5] = c;
    return r;
}

// fovY in radians.
function perspectiveGL(fovY: f32, aspect: f32, near: f32, far: f32) => Mat4 {
    let f = ONE / (Math.tan((fovY * HALF) as f64) as f32);
    let r = mat4Zero();
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (far + near) / (near - far);
    r.m[11] = -ONE;
    r.m[14] = (TWO * far * near) / (near - far);
    return r;
}

function perspectiveVk(fovY: f32, aspect: f32, near: f32, far: f32) => Mat4 {
    let f = ONE / (Math.tan((fovY * HALF) as f64) as f32);
    let r = mat4Zero();
    r.m[0] = f / aspect;
    r.m[5] = -f;
    r.m[10] = far / (near - far);
    r.m[11] = -ONE;
    r.m[14] = (near * far) / (near - far);
    return r;
}

function lookAt(eye: Vec3, center: Vec3, up: Vec3) => Mat4 {
    let f = center.sub(eye).normalize();
    let s = f.cross(up).normalize();
    let u = s.cross(f);
    let r = mat4Identity();
    r.m[0] = s.x;
    r.m[4] = s.y;
    r.m[8] = s.z;
    r.m[1] = u.x;
    r.m[5] = u.y;
    r.m[9] = u.z;
    r.m[2] = -f.x;
    r.m[6] = -f.y;
    r.m[10] = -f.z;
    r.m[12] = -s.dot(eye);
    r.m[13] = -u.dot(eye);
    r.m[14] = f.dot(eye);
    return r;
}
