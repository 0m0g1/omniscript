// docs/examples/dual_window_city/dual_window_city.os
// Same raymarched neon city, rendered by TWO GPU backends side by side: OpenGL window + Vulkan window.
// The scene is one fragment shader (pen.shaderRect), so the frame-time gap you see is GPU/driver, not script.
// Esc closes the focused window. Not compiled/run: check against your compiler.
//
// Needs: docs/examples/dual_window_city/city_raymarch.frag.spv (Vulkan build of the same GLSL, see city_raymarch.vk.frag in this folder)
//        standard/1/Shaders/canvas2d.{vert,frag}.spv (already used by canvas_test.os)
module C = import "standard/1/Canvas.os";
module CMem = import "standard/1/c/mem.os";

let GRAPH_N: int = 120;
let VIEW_ASPECT: float = 1.3333333;      // must match ASPECT in the shader

struct Slot {
    win: C.Win* = nullptr;
    shader: u32 = 0;
    ready: bool = false;
    api: int = 0;
    num: char* = nullptr;
    graph: float* = nullptr;
    gpos: int = 0;
    lastT: float = 0.0;
    fps: float = 0.0;
    ms: float = 0.0;
}

// String constants are functions (module-level char* globals read back as garbage in this engine).
function citySrc() => char* {
    return "const float S = 1.25; const float ASPECT = 1.3333333; const float TAU = 6.2831853; float hgt(vec2 c, float t) { float d = length(c); float h = 0.9 + 0.55 * sin(d * 0.85 - t * 2.4) + 0.35 * sin(c.x * 0.5 + t * 1.3) * cos(c.y * 0.5 - t * 0.9); return max(h, 0.15); } float sdBox(vec3 p, vec3 b) { vec3 q = abs(p) - b; return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0); } mat2 rot(float a) { float c = cos(a); float s = sin(a); return mat2(c, -s, s, c); } float city(vec3 p, float t, out float hue) { vec2 base = floor(p.xz / S); float best = 1e9; hue = 0.0; for (int j = 0; j < 2; j++) { for (int i = 0; i < 2; i++) { vec2 k = base + vec2(float(i), float(j)); if (abs(k.x) > 10.0 || abs(k.y) > 10.0) continue; vec2 c = k * S; float h = hgt(c, t); float d = sdBox(vec3(p.x - c.x, p.y - h * 0.5, p.z - c.y), vec3(S * 0.39, h * 0.5, S * 0.39)); if (d < best) { best = d; hue = length(c) * 0.035 + t * 0.05 + h * 0.12; } } } return best; } float props(vec3 p, float t, out float hue) { vec3 q = p - vec3(0.0, 6.5 + 0.6 * sin(t * 1.5), 0.0); q.xz = rot(t * 0.9) * q.xz; q.yz = rot(t * 0.6) * q.yz; float d = sdBox(q, vec3(1.3)); hue = t * 0.12; for (int s = 0; s < 4; s++) { float a = t * 1.4 + float(s) * 1.5707963; vec3 sp = p - vec3(5.0 * cos(a), 6.5 + 1.8 * sin(a * 2.0), 5.0 * sin(a)); sp.xz = rot(t * 2.0) * sp.xz; float ds = sdBox(sp, vec3(0.45)); if (ds < d) { d = ds; hue = t * 0.12 + float(s) * 0.25; } } return d; } float mapScene(vec3 p, float t, out float hue) { float h1; float h2; float a = city(p, t, h1); float b = props(p, t, h2); float d = a; hue = h1; if (b < d) { d = b; hue = h2; } if (p.y < d) { d = p.y; hue = -1.0; } return d; } vec3 palette(float h) { return 0.5 + 0.5 * sin(TAU * (h + vec3(0.0, 0.333, 0.667))); } vec3 calcNormal(vec3 p, float t) { float e = 0.004; float hh; vec2 k = vec2(1.0, -1.0); return normalize(k.xyy * mapScene(p + k.xyy * e, t, hh) + k.yyx * mapScene(p + k.yyx * e, t, hh) + k.yxy * mapScene(p + k.yxy * e, t, hh) + k.xxx * mapScene(p + k.xxx * e, t, hh)); } void main() { float t = uTime; vec2 uv = vec2((vUV.x - 0.5) * ASPECT, 0.5 - vUV.y); float ang = t * 0.15; vec3 ro = vec3(27.0 * sin(ang), 13.0, 27.0 * cos(ang)); vec3 f = normalize(vec3(0.0, 1.2, 0.0) - ro); vec3 r = normalize(cross(f, vec3(0.0, 1.0, 0.0))); vec3 u = cross(r, f); float focal = 0.5 / tan(radians(55.0) * 0.5); vec3 rd = normalize(uv.x * r + uv.y * u + focal * f); vec3 bg = vec3(0.015, 0.02, 0.05); float dist = 0.0; float hue = 0.0; float glow = 0.0; bool hit = false; for (int i = 0; i < 110; i++) { vec3 p = ro + rd * dist; float d = mapScene(p, t, hue); glow += exp(-d * 2.5) * 0.006; if (d < 0.002 + 0.0015 * dist) { hit = true; break; } dist += d * 0.85; if (dist > 80.0) break; } vec3 col = bg + vec3(0.25, 0.1, 0.4) * glow; if (hit) { vec3 p = ro + rd * dist; vec3 n = calcNormal(p, t); vec3 l = normalize(vec3(cos(t * 0.4), 0.9, sin(t * 0.4))); float diff = max(dot(n, l), 0.0) * 0.75 + 0.25; vec3 base; if (hue < -0.5) { vec2 gp = abs(fract(p.xz / S + 0.5) - 0.5); float line = smoothstep(0.03, 0.0, min(gp.x, gp.y)); base = vec3(0.03, 0.04, 0.08) + vec3(0.1, 0.5, 0.8) * line * 0.5; diff = 1.0; } else { base = palette(hue); } float rim = pow(1.0 - max(dot(n, -rd), 0.0), 3.0); col = base * diff + base * rim * 0.5 + vec3(0.25, 0.1, 0.4) * glow; col = mix(col, bg, 1.0 - exp(-dist * 0.018)); } o = vec4(sqrt(clamp(col, 0.0, 1.0)), 1.0); }";
}
function spvPath() => char* { return "docs/examples/dual_window_city/city_raymarch.frag.spv"; }

function itoa(buf: char*, v: int) => void {
    let n: int = v;
    if (n < 0) n = 0;
    if (n == 0) { buf[0] = 48 as char; buf[1] = 0 as char; return; }
    let len: int = 0;
    let t: int = n;
    while (t > 0) { len = len + 1; t = t / 10; }
    buf[len] = 0 as char;
    let i: int = len - 1;
    while (n > 0) {
        buf[i] = (48 + n % 10) as char;
        n = n / 10;
        i = i - 1;
    }
}

function labelInt(pen: C.Pen2D*, num: char*, x: float, y: float, label: char*, v: int, c: u32) => void {
    itoa(num, v);
    pen.text(x, y, label, c);
    pen.text(x + pen.textWidth(label, 1.0), y, num, c);
}

function labelMs(pen: C.Pen2D*, num: char*, x: float, y: float, label: char*, ms: float, c: u32) => void {
    let tenths: int = (ms * 10.0) as int;
    let whole: int = tenths / 10;
    let frac: int = tenths % 10;
    itoa(num, whole);
    let n: int = 0;
    while (num[n] != (0 as char)) n = n + 1;
    num[n] = 46 as char;                      // '.'
    num[n + 1] = (48 + frac) as char;
    num[n + 2] = 0 as char;
    pen.text(x, y, label, c);
    pen.text(x + pen.textWidth(label, 1.0), y, num, c);
}

function backendName(api: int) => char* {
    if (api == C.API_GL) return "OpenGL";
    if (api == C.API_VULKAN) return "Vulkan";
    return "Software";
}

function newSlot() => Slot* {
    let t = CMem.malloc(sizeof(Slot)) as Slot*;
    CMem.memset(t as void*, 0, sizeof(Slot));
    return t;
}

function openSlot(app: C.Canvas*, t: Slot*, api: int, title: char*) => bool {
    let win: C.Win* = app.createWindowEx(title, 800, 600, api);
    if (win == nullptr) return false;
    t.win = win;
    t.api = win.api;
    return true;
}

function setupSlot(t: Slot*) => void {
    let pen: C.Pen2D* = t.win.context2d();
    t.num = CMem.malloc(16 as u64) as char*;
    t.graph = CMem.malloc((GRAPH_N * 4) as u64) as float*;
    let z: int = 0;
    while (z < GRAPH_N) { t.graph[z] = 0.0; z = z + 1; }
    // GL compiles the GLSL body; Vulkan loads the .spv; software returns 0 (no custom shaders).
    t.shader = pen.createShaderBoth(citySrc(), spvPath());
    t.ready = true;
}

function drawGraph(pen: C.Pen2D*, t: Slot*, x: float, base: float) => void {
    pen.fillRect(x - 4.0, base - 70.0, (GRAPH_N as float) + 8.0, 74.0, 0x000000A0);
    let y16: float = base - 16.7 * 3.0;                       // 60 FPS reference line
    pen.line(x, y16, x + (GRAPH_N as float), y16, 1.0, 0xFFFFFF50);
    let i: int = 0;
    while (i < GRAPH_N) {
        let v: float = t.graph[(t.gpos + i) % GRAPH_N];
        let hgt: float = v * 3.0;
        if (hgt > 66.0) hgt = 66.0;
        let col: u32 = 0x55DD77FF;
        if (v > 16.8) col = 0xFFAA33FF;
        if (v > 33.4) col = 0xFF5555FF;
        pen.line(x + (i as float), base, x + (i as float), base - hgt, 1.0, col);
        i = i + 1;
    }
}

function runSlot(t: Slot*, now: float) => void {
    if (t.win == nullptr) return;
    let win: C.Win* = t.win;
    if (!win.isOpen) return;
    if (!win.active) return;
    if (!t.ready) setupSlot(t);

    let pen: C.Pen2D* = win.context2d();
    let w: float = win.width() as float;
    let h: float = win.height() as float;

    let dt: float = now - t.lastT;
    let first: bool = (t.lastT == 0.0);
    t.lastT = now;
    if (dt > 0.25) dt = 0.25;                 // clamp stalls (window drag, shader compile) so they don't wreck the average
    if (dt > 0.0 && !first) {
        if (t.fps == 0.0) {                   // seed the smoothing from the first real sample
            t.fps = 1.0 / dt;
            t.ms = dt * 1000.0;
        } else {
            t.fps = t.fps * 0.9 + (1.0 / dt) * 0.1;
            t.ms = t.ms * 0.9 + dt * 1000.0 * 0.1;
        }
        t.graph[t.gpos] = dt * 1000.0;
        t.gpos = (t.gpos + 1) % GRAPH_N;
    }

    if (win.keyPressed(256)) win.close();

    pen.clear(0.015, 0.02, 0.05, 1.0);

    // letterboxed 4:3 view so the shader's fixed aspect is never stretched
    let vw: float = w;
    let vh: float = vw / VIEW_ASPECT;
    if (vh > h) { vh = h; vw = vh * VIEW_ASPECT; }
    let vx: float = (w - vw) * 0.5;
    let vy: float = (h - vh) * 0.5;

    if (t.shader != 0) {
        pen.shaderRect(t.shader, vx, vy, vw, vh);
    } else {
        pen.text(20.0, 60.0, "Custom shader unavailable on this backend.", 0xFFAA33FF);
        pen.text(20.0, 80.0, "GL: GLSL compile failed?  Vulkan: city_raymarch.frag.spv missing or mismatched?", 0xAAAAAAFF);
    }

    // HUD
    pen.fillRect(0.0, 0.0, w, 28.0, 0x1E2230D0);
    pen.text(10.0, 7.0, backendName(t.api), 0x55AAFFFF);
    labelInt(pen, t.num, 140.0, 7.0, "FPS ", t.fps as int, 0x55DD77FF);
    labelMs(pen, t.num, 260.0, 7.0, "ms ", t.ms, 0xFFEE55FF);
    pen.text(w - 130.0, 7.0, "Esc: close", 0xAAAAAAFF);
    drawGraph(pen, t, 16.0, h - 16.0);
}

function main() => int {
    let app: C.Canvas;
    if (!app.init()) return 1;
    app.shaderDir = "standard/1/Shaders/";       // canvas2d.{vert,frag}.spv for Vulkan windows (trailing slash required)

    let gl: Slot* = newSlot();
    let vk: Slot* = newSlot();

    if (!openSlot(&app, gl, C.API_GL, "OmniScript - OpenGL")) { app.quit(); return 2; }
    // If Vulkan init fails (driver, or .spv not found) we keep running with the GL window only.
    let vkOk: bool = openSlot(&app, vk, C.API_VULKAN, "OmniScript - Vulkan");

    while (app.frame()) {
        let now: float = app.time() as float;
        runSlot(gl, now);
        runSlot(vk, now);
        app.present();
    }

    app.quit();
    return 0;
}
