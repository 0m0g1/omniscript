// standard/1/Canvas.os
// Tier 7: the display root. Canvas creates windows; every Win owns one GL backend and two pens:
//   Canvas  app root: createWindow(), frame(), present(), time(), quit()
//   Win     one native window + context. Input (mouse/keys/typed text, with per-frame edges) lives here.
//   Pen2D   batched 2D drawing: fillRect, rect, line, circle, triangle, text, image, clip, shaderRect.
//           Everything draws through ONE texture atlas (font + white pixel) so a whole GUI is 1 draw call.
//           2D is flushed in present(), so it always lands on top of 3D drawn earlier in the frame.
//   Pen3D   immediate 3D: meshes, camera, lit shader, textures, depth. Draws on the call.
// Gui.os sits on a Pen2D + Win and never touches GL.
//
// Backends (one per window, chosen at creation; Pen2D code is identical on all three):
//   API_GL        OpenGL 3.3 core. Windows share textures. Pen2D + Pen3D.
//   API_VULKAN    Vulkan 1.1 through Vulkan.os: one shared device (initCore) + one swapchain per window (attach).
//                 Pen2D only; for 3D use win.vk (a Vulkan.os target) directly. Needs shaders/*.spv next to the
//                 working directory (or app.shaderDir).
//   API_SOFTWARE  CPU rasteriser into an RGBA buffer (pen.pixels()); shown in a GL window by a texture blit, or
//                 used headless via Pen2D.initSoftware() with no window at all. Custom 2D shaders are skipped.
// Every layer also works on its own: Window.os, OpenGL.os, Vulkan.os are used exactly as before, and a Pen2D
// can be driven without Canvas: pen.init(glWindow) / pen.initVulkan(&vk, dir) / pen.initSoftware(w, h).
// Portable images: pen.createImage(rgba, w, h) -> id for pen.image(); portable shaders: pen.createShaderBoth().
// Colours are 0xRRGGBBAA. Not compiled/run yet. No `return` inside loop bodies (compiler quirk).
module W = import "standard/1/Window.os";
module G = import "standard/1/c/glfw.os";
module GL = import "standard/1/c/opengl.os";
module OGL = import "standard/1/OpenGL.os";
module F = import "standard/1/FontData.os";
module L = import "standard/1/List.os";
module LF = import "standard/1/ListF32.os";
module LU = import "standard/1/ListU32.os";
module VK = import "standard/1/Vulkan.os";
module VKC = import "standard/1/c/vulkan.os";
module CMem = import "standard/1/c/mem.os";

let API_AUTO: int = 0 - 1;
let API_GL: int = 0;
let API_VULKAN: int = 1;
let API_SOFTWARE: int = 2;
let MAX_IMAGES: int = 64;
let MAX_WINDOWS: int = 8;

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

// ---------------------------------------------------------------------------------------------
// shaders
// ---------------------------------------------------------------------------------------------
function vs2d() => char* {
    return "#version 330 core\nlayout(location=0) in vec2 aPos;\nlayout(location=1) in vec2 aUV;\nlayout(location=2) in vec4 aCol;\nuniform vec2 uScreen;\nout vec2 vUV;\nout vec4 vCol;\nvoid main() {\n  vec2 p = aPos / uScreen * 2.0 - 1.0;\n  gl_Position = vec4(p.x, -p.y, 0.0, 1.0);\n  vUV = aUV; vCol = aCol;\n}\n";
}
function fs2d() => char* {
    return "#version 330 core\nin vec2 vUV;\nin vec4 vCol;\nuniform sampler2D uTex;\nout vec4 o;\nvoid main() { o = texture(uTex, vUV) * vCol; }\n";
}
// User 2D shaders: write only the fragment shader. You get vUV (0..1 across the rect), vCol,
// uSize (rect size in px), uTime (seconds). Output to `o`.
function fs2dPrefix() => char* {
    return "#version 330 core\nin vec2 vUV;\nin vec4 vCol;\nuniform vec2 uSize;\nuniform float uTime;\nout vec4 o;\n";
}
function vs3d() => char* {
    return "#version 330 core\nlayout(location=0) in vec3 aPos;\nlayout(location=1) in vec3 aN;\nlayout(location=2) in vec2 aUV;\nuniform mat4 uVP;\nuniform mat4 uModel;\nout vec3 vN;\nout vec2 vUV;\nvoid main() {\n  vN = mat3(uModel) * aN; vUV = aUV;\n  gl_Position = uVP * uModel * vec4(aPos, 1.0);\n}\n";
}
function fs3d() => char* {
    return "#version 330 core\nin vec3 vN;\nin vec2 vUV;\nuniform vec4 uColor;\nuniform vec3 uLight;\nuniform sampler2D uTex;\nuniform int uUseTex;\nout vec4 o;\nvoid main() {\n  float d = max(dot(normalize(vN), normalize(uLight)), 0.0);\n  vec4 base = uColor;\n  if (uUseTex != 0) base *= texture(uTex, vUV);\n  o = vec4(base.rgb * (0.25 + 0.75 * d), base.a);\n}\n";
}

function copyBytes(dst: u8*, src: u8*, n: u64) => void {
    let i: u64 = 0;
    while (i < n) { dst[i] = src[i]; i = i + 1; }
}

function clampi(v: int, lo: int, hi: int) => int {
    let r: int = v;
    if (r < lo) r = lo;
    if (r > hi) r = hi;
    return r;
}

function catStr(a: char*, b: char*) => char* {
    let la: int = 0; while (a[la] as int != 0) { la = la + 1; }
    let lb: int = 0; while (b[lb] as int != 0) { lb = lb + 1; }
    let r = CMem.malloc((la + lb + 1) as u64) as char*;
    let i: int = 0; while (i < la) { r[i] = a[i]; i = i + 1; }
    i = 0; while (i < lb) { r[la + i] = b[i]; i = i + 1; }
    r[la + lb] = 0 as char;
    return r;
}

// ---------------------------------------------------------------------------------------------
// Pen2D
// ---------------------------------------------------------------------------------------------
struct DrawCmd {
    tex: u32; prog: u32;
    clipOn: int; cx: int; cy: int; cw: int; ch: int;
    first: u32; count: u32;
    rw: float; rh: float;
}

struct Pen2D {
    h: void* = nullptr;                 // GLFW window (to make its context current)
    verts: LF.ListF32;                  // x y u v r g b a  per vertex
    idx: LU.ListU32;
    cmds: L.List<DrawCmd>;
    prog: OGL.Program;
    vao: u32 = 0; vbo: u32 = 0; ebo: u32 = 0;
    atlas: u32 = 0;
    vcount: u32 = 0;
    w: int = 0; ht: int = 0;
    clipStack: int* = nullptr;          // 16 levels x 4 ints
    clipDepth: int = 0;
    clearR: float = 0.1; clearG: float = 0.1; clearB: float = 0.12; clearA: float = 1.0;
    ready: bool = false;
    backend: int = 0;                   // API_GL / API_VULKAN / API_SOFTWARE
    // portable image table (Vulkan + software): id = slot + 1, slot 0 is the font atlas (id 1)
    imgUsed: u8* = nullptr; imgW: int* = nullptr; imgH: int* = nullptr; imgPx: u8** = nullptr;
    // software backend
    swBuf: u8* = nullptr; swW: int = 0; swH: int = 0;
    // vulkan backend
    vk: VK.Vulkan* = nullptr;
    vkPipe: VK.Pipeline;
    vkShaders: VK.Pipeline* = nullptr; vkShaderCount: int = 0;
    vkTex: VK.Texture* = nullptr;
    vkVb: VK.Buffer* = nullptr; vkIb: VK.Buffer* = nullptr;
    vkPc: float* = nullptr;
    shaderDir: char* = nullptr;

    // Needs the window's GL context current and OpenGL.load() done.
    init(handle: void*) => bool {
        this.h = handle;
        this.backend = API_GL;
        this.clearR = 0.1; this.clearG = 0.1; this.clearB = 0.12; this.clearA = 1.0;
        if (!this.prog.build(vs2d(), fs2d())) return false;
        OGL.glGenVertexArrays(1, &this.vao);
        OGL.glGenBuffers(1, &this.vbo);
        OGL.glGenBuffers(1, &this.ebo);
        OGL.glBindVertexArray(this.vao);
        OGL.glBindBuffer(OGL.ARRAY_BUFFER, this.vbo);
        OGL.glVertexAttribPointer(0, 2, OGL.FLOAT, 0, 32, 0 as i64 as void*);
        OGL.glEnableVertexAttribArray(0);
        OGL.glVertexAttribPointer(1, 2, OGL.FLOAT, 0, 32, 8 as i64 as void*);
        OGL.glEnableVertexAttribArray(1);
        OGL.glVertexAttribPointer(2, 4, OGL.FLOAT, 0, 32, 16 as i64 as void*);
        OGL.glEnableVertexAttribArray(2);
        OGL.glBindBuffer(OGL.ELEMENT_ARRAY_BUFFER, this.ebo);
        OGL.glBindVertexArray(0);

        let px: u8* = F.buildAtlas();
        let t: OGL.Texture;
        t.fromRGBA(px, F.ATLAS_W, F.ATLAS_H, false);
        GL.glTexParameteri(OGL.TEXTURE_2D, OGL.TEXTURE_MAG_FILTER, OGL.NEAREST);
        GL.glTexParameteri(OGL.TEXTURE_2D, OGL.TEXTURE_MIN_FILTER, OGL.NEAREST);
        CMem.free(px as void*);
        this.atlas = t.id;
        this.clipStack = CMem.malloc(64 * 4) as int*;
        this.ready = true;
        return true;
    }

    // Shared setup for the non-GL backends: clip stack + image table.
    initTables() => void {
        this.clearR = 0.1; this.clearG = 0.1; this.clearB = 0.12; this.clearA = 1.0;
        this.clipStack = CMem.malloc(64 * 4) as int*;
        this.imgUsed = CMem.malloc(MAX_IMAGES as u64) as u8*;
        this.imgW = CMem.malloc((MAX_IMAGES * 4) as u64) as int*;
        this.imgH = CMem.malloc((MAX_IMAGES * 4) as u64) as int*;
        this.imgPx = CMem.malloc((MAX_IMAGES * 8) as u64) as u8**;
        CMem.memset(this.imgUsed as void*, 0, MAX_IMAGES as u64);
        CMem.memset(this.imgW as void*, 0, (MAX_IMAGES * 4) as u64);
        CMem.memset(this.imgH as void*, 0, (MAX_IMAGES * 4) as u64);
        CMem.memset(this.imgPx as void*, 0, (MAX_IMAGES * 8) as u64);
        this.atlas = 1;
        this.imgUsed[0] = 1 as u8;
    }

    // ---- software backend: no window, no GL. Draw, then read pixels() (RGBA8, row 0 = top). ----
    initSoftware(w: int, hh: int) => bool {
        this.backend = API_SOFTWARE;
        this.initTables();
        this.imgPx[0] = F.buildAtlas();
        this.imgW[0] = F.ATLAS_W; this.imgH[0] = F.ATLAS_H;
        this.swResize(w, hh);
        this.ready = true;
        return true;
    }

    swResize(w: int, hh: int) => void {
        if (w <= 0 || hh <= 0) return;
        if (this.swBuf != nullptr && this.swW == w && this.swH == hh) return;
        if (this.swBuf != nullptr) CMem.free(this.swBuf as void*);
        this.swBuf = CMem.malloc((w * hh * 4) as u64) as u8*;
        CMem.memset(this.swBuf as void*, 0, (w * hh * 4) as u64);
        this.swW = w; this.swH = hh;
    }

    pixels() => u8* { return this.swBuf; }
    pixelWidth() => int { return this.swW; }
    pixelHeight() => int { return this.swH; }

    // ---- vulkan backend: vk must already be a presentation target (Vulkan.init or Vulkan.attach) ----
    // dir = folder holding canvas2d.vert.spv / canvas2d.frag.spv (default "shaders/").
    initVulkan(target: VK.Vulkan*, dir: char*) => bool {
        this.backend = API_VULKAN;
        this.vk = target;
        this.shaderDir = dir;
        if (dir == nullptr) this.shaderDir = "shaders/";
        this.initTables();
        let vsPath: char* = catStr(this.shaderDir, "canvas2d.vert.spv");
        let fsPath: char* = catStr(this.shaderDir, "canvas2d.frag.spv");
        let vs: u64 = target.loadShader(vsPath);
        let fs: u64 = target.loadShader(fsPath);
        CMem.free(vsPath as void*); CMem.free(fsPath as void*);
        if (vs == 0 || fs == 0) return false;
        let formats = CMem.malloc(12) as u32*;
        let offsets = CMem.malloc(12) as u32*;
        formats[0] = VK.VK_FORMAT_R32G32_SFLOAT; formats[1] = VK.VK_FORMAT_R32G32_SFLOAT; formats[2] = VK.VK_FORMAT_R32G32B32A32_SFLOAT;
        offsets[0] = 0; offsets[1] = 8; offsets[2] = 16;
        let ok: bool = target.createPipeline(&this.vkPipe, vs, fs, 32, 3, formats, offsets, true, 20);
        CMem.free(formats as void*); CMem.free(offsets as void*);
        target.destroyShader(vs); target.destroyShader(fs);
        if (!ok) return false;
        this.vkVb = CMem.malloc(2 * sizeof(VK.Buffer)) as VK.Buffer*;
        this.vkIb = CMem.malloc(2 * sizeof(VK.Buffer)) as VK.Buffer*;
        CMem.memset(this.vkVb as void*, 0, 2 * sizeof(VK.Buffer));
        CMem.memset(this.vkIb as void*, 0, 2 * sizeof(VK.Buffer));
        this.vkTex = CMem.malloc((MAX_IMAGES as u64) * sizeof(VK.Texture)) as VK.Texture*;
        CMem.memset(this.vkTex as void*, 0, (MAX_IMAGES as u64) * sizeof(VK.Texture));
        this.vkShaders = CMem.malloc(16 * sizeof(VK.Pipeline)) as VK.Pipeline*;
        CMem.memset(this.vkShaders as void*, 0, 16 * sizeof(VK.Pipeline));
        this.vkPc = CMem.malloc(20) as float*;
        let px: u8* = F.buildAtlas();
        let good: bool = target.createTexture(&this.vkTex[0], px, F.ATLAS_W, F.ATLAS_H);
        CMem.free(px as void*);
        if (!good) return false;
        this.ready = true;
        return true;
    }

    // ---- portable images: works on every backend, returns an id for image()/imageTinted()/imageFlipped() ----
    findSlot() => int {
        let r: int = 0 - 1;
        let i: int = 1;
        while (i < MAX_IMAGES) {
            if (r < 0 && this.imgUsed[i] == 0) r = i;
            i = i + 1;
        }
        return r;
    }

    // px: RGBA8, w*h*4 bytes, row 0 = top. Returns 0 on failure. Free with freeImage().
    createImage(px: u8*, w: int, hh: int) => u32 {
        if (this.backend == API_GL) {
            G.glfwMakeContextCurrent(this.h);
            let t: OGL.Texture;
            t.fromRGBA(px, w, hh, false);
            return t.id;
        }
        let slot: int = this.findSlot();
        if (slot < 0) return 0;
        if (this.backend == API_SOFTWARE) {
            let copy = CMem.malloc((w * hh * 4) as u64) as u8*;
            copyBytes(copy, px, (w * hh * 4) as u64);
            this.imgPx[slot] = copy;
        } else {
            if (!this.vk.createTexture(&this.vkTex[slot], px, w, hh)) return 0;
        }
        this.imgW[slot] = w; this.imgH[slot] = hh;
        this.imgUsed[slot] = 1 as u8;
        return (slot + 1) as u32;
    }

    freeImage(id: u32) => void {
        if (id == 0) return;
        if (this.backend == API_GL) { G.glfwMakeContextCurrent(this.h); let tid: u32 = id; GL.glDeleteTextures(1, &tid); return; }
        let slot: int = (id as int) - 1;
        if (slot < 1 || slot >= MAX_IMAGES || this.imgUsed[slot] == 0) return;
        if (this.backend == API_SOFTWARE) { CMem.free(this.imgPx[slot] as void*); this.imgPx[slot] = nullptr; }
        else { this.vk.destroyTexture(&this.vkTex[slot]); }
        this.imgUsed[slot] = 0 as u8;
    }

    free() => void {
        if (!this.ready) return;
        if (this.backend == API_GL) {
            G.glfwMakeContextCurrent(this.h);
            this.prog.free();
            if (this.vbo != 0) OGL.glDeleteBuffers(1, &this.vbo);
            if (this.ebo != 0) OGL.glDeleteBuffers(1, &this.ebo);
            if (this.vao != 0) OGL.glDeleteVertexArrays(1, &this.vao);
            if (this.atlas != 0) GL.glDeleteTextures(1, &this.atlas);
        } else {
            if (this.backend == API_VULKAN) this.freeVulkan();
            let i: int = 0;
            while (i < MAX_IMAGES) {
                if (this.backend == API_SOFTWARE && this.imgPx[i] != nullptr) CMem.free(this.imgPx[i] as void*);
                i = i + 1;
            }
            CMem.free(this.imgUsed as void*); CMem.free(this.imgW as void*); CMem.free(this.imgH as void*); CMem.free(this.imgPx as void*);
            if (this.swBuf != nullptr) { CMem.free(this.swBuf as void*); this.swBuf = nullptr; }
        }
        this.verts.free(); this.idx.free(); this.cmds.free();
        if (this.clipStack != nullptr) CMem.free(this.clipStack as void*);
        this.clipStack = nullptr; this.ready = false;
    }

    // Called by Win each frame (size = framebuffer pixels).
    begin(w: int, hh: int) => void {
        this.w = w; this.ht = hh;
        if (this.backend == API_SOFTWARE) this.swResize(w, hh);
        this.verts.len = 0; this.idx.len = 0; this.cmds.len = 0;
        this.vcount = 0; this.clipDepth = 0;
    }

    width() => int { return this.w; }
    height() => int { return this.ht; }

    // Clears the window immediately (colour + depth). Call after the frame started.
    clear(r: float, g: float, b: float, a: float) => void {
        this.clearR = r; this.clearG = g; this.clearB = b; this.clearA = a;
        if (this.backend == API_GL) {
            G.glfwMakeContextCurrent(this.h);
            OGL.viewport(this.w, this.ht);
            OGL.clear(r, g, b, a);
        }
        if (this.backend == API_SOFTWARE && this.swBuf != nullptr) {
            let rb: u8 = (r * 255.0 + 0.5) as u8; let gb: u8 = (g * 255.0 + 0.5) as u8;
            let bb: u8 = (b * 255.0 + 0.5) as u8; let ab: u8 = (a * 255.0 + 0.5) as u8;
            let i: int = 0;
            let total: int = this.swW * this.swH;
            while (i < total) {
                this.swBuf[i * 4] = rb; this.swBuf[i * 4 + 1] = gb; this.swBuf[i * 4 + 2] = bb; this.swBuf[i * 4 + 3] = ab;
                i = i + 1;
            }
        }
        // Vulkan: the colour is applied when the frame's render pass begins (flush).
    }

    // ---- user 2D shaders ----
    // fragBody: GLSL fragment `void main() { ... o = vec4(...); }` (see fs2dPrefix for what's in scope).
    // Returns a program id, 0 on failure (OpenGL.lastError() has the log).
    createShader(fragBody: char*) => u32 {
        if (this.backend != API_GL) return 0;
        G.glfwMakeContextCurrent(this.h);
        let src: char* = catStr(fs2dPrefix(), fragBody);
        let p: u32 = OGL.program(vs2d(), src);
        CMem.free(src as void*);
        return p;
    }
    freeShader(p: u32) => void {
        if (p == 0) return;
        if (this.backend == API_GL) { G.glfwMakeContextCurrent(this.h); OGL.glDeleteProgram(p); }
    }

    // Vulkan: load a fragment shader compiled to SPIR-V (see shaders/plasma.frag). It gets location 0 vUV,
    // location 1 vCol and the push block { vec2 uScreen; vec2 uSize; float uTime; }. Returns 0 elsewhere / on failure.
    // Shaders live until the pen is freed (up to 16).
    createShaderSpv(fragSpvPath: char*) => u32 {
        if (this.backend != API_VULKAN || this.vkShaderCount >= 16) return 0;
        let vsPath: char* = catStr(this.shaderDir, "canvas2d.vert.spv");
        let vs: u64 = this.vk.loadShader(vsPath);
        CMem.free(vsPath as void*);
        let fs: u64 = this.vk.loadShader(fragSpvPath);
        if (vs == 0 || fs == 0) { this.vk.destroyShader(vs); this.vk.destroyShader(fs); return 0; }
        let formats = CMem.malloc(12) as u32*;
        let offsets = CMem.malloc(12) as u32*;
        formats[0] = VK.VK_FORMAT_R32G32_SFLOAT; formats[1] = VK.VK_FORMAT_R32G32_SFLOAT; formats[2] = VK.VK_FORMAT_R32G32B32A32_SFLOAT;
        offsets[0] = 0; offsets[1] = 8; offsets[2] = 16;
        let ok: bool = this.vk.createPipeline(&this.vkShaders[this.vkShaderCount], vs, fs, 32, 3, formats, offsets, false, 20);
        CMem.free(formats as void*); CMem.free(offsets as void*);
        this.vk.destroyShader(vs); this.vk.destroyShader(fs);
        if (!ok) return 0;
        this.vkShaderCount = this.vkShaderCount + 1;
        return this.vkShaderCount as u32;
    }

    // Portable: GLSL fragment body for GL, SPIR-V path for Vulkan; software returns 0 (shaderRect is skipped).
    createShaderBoth(glFragBody: char*, vkFragSpvPath: char*) => u32 {
        if (this.backend == API_GL) return this.createShader(glFragBody);
        if (this.backend == API_VULKAN) return this.createShaderSpv(vkFragSpvPath);
        return 0;
    }

    // ---- clipping ----
    curClipX() => int { return this.clipStack[(this.clipDepth - 1) * 4]; }
    curClipY() => int { return this.clipStack[(this.clipDepth - 1) * 4 + 1]; }
    curClipW() => int { return this.clipStack[(this.clipDepth - 1) * 4 + 2]; }
    curClipH() => int { return this.clipStack[(this.clipDepth - 1) * 4 + 3]; }

    // Nested clips intersect. Max depth 16.
    pushClip(x: int, y: int, cw: int, chh: int) => void {
        if (this.clipDepth >= 16) return;
        let x0: int = x; let y0: int = y; let x1: int = x + cw; let y1: int = y + chh;
        if (this.clipDepth > 0) {
            let px0: int = this.curClipX(); let py0: int = this.curClipY();
            let px1: int = px0 + this.curClipW(); let py1: int = py0 + this.curClipH();
            if (px0 > x0) x0 = px0;
            if (py0 > y0) y0 = py0;
            if (px1 < x1) x1 = px1;
            if (py1 < y1) y1 = py1;
        }
        if (x1 < x0) x1 = x0;
        if (y1 < y0) y1 = y0;
        let o: int = this.clipDepth * 4;
        this.clipStack[o] = x0; this.clipStack[o + 1] = y0; this.clipStack[o + 2] = x1 - x0; this.clipStack[o + 3] = y1 - y0;
        this.clipDepth = this.clipDepth + 1;
    }
    popClip() => void { if (this.clipDepth > 0) this.clipDepth = this.clipDepth - 1; }

    // ---- batching core ----
    // Starts (or continues) a draw command for this state.
    ensure(tex: u32, prog: u32, rw: float, rh: float) => void {
        let need: bool = true;
        let n: u64 = this.cmds.len;
        if (n > 0 && prog == 0) {
            let last: DrawCmd = this.cmds.data[n - 1];
            let sameClip: bool = true;
            if (this.clipDepth == 0) { if (last.clipOn != 0) sameClip = false; }
            else {
                if (last.clipOn == 0) sameClip = false;
                else if (last.cx != this.curClipX() || last.cy != this.curClipY() || last.cw != this.curClipW() || last.ch != this.curClipH()) sameClip = false;
            }
            if (last.tex == tex && last.prog == 0 && sameClip) need = false;
        }
        if (need) {
            let d: DrawCmd;
            d.tex = tex; d.prog = prog; d.first = this.idx.len as u32; d.count = 0;
            d.rw = rw; d.rh = rh;
            d.clipOn = 0; d.cx = 0; d.cy = 0; d.cw = 0; d.ch = 0;
            if (this.clipDepth > 0) {
                d.clipOn = 1; d.cx = this.curClipX(); d.cy = this.curClipY(); d.cw = this.curClipW(); d.ch = this.curClipH();
            }
            this.cmds.push(d);
        }
    }

    vert(x: float, y: float, u: float, v: float, c: u32) => void {
        this.verts.push(x); this.verts.push(y); this.verts.push(u); this.verts.push(v);
        this.verts.push(chan(c, 24)); this.verts.push(chan(c, 16)); this.verts.push(chan(c, 8)); this.verts.push(chan(c, 0));
        this.vcount = this.vcount + 1;
    }

    tri(a: u32, b: u32, c: u32, base: u32) => void {
        this.idx.push(base + a); this.idx.push(base + b); this.idx.push(base + c);
    }

    grow(k: u32) => void {
        let last: DrawCmd* = &this.cmds.data[this.cmds.len - 1];
        last.count = last.count + k;
    }

    quad(x: float, y: float, w: float, hh: float, u0: float, v0: float, u1: float, v1: float, c: u32, tex: u32, prog: u32) => void {
        this.ensure(tex, prog, w, hh);
        let base: u32 = this.vcount;
        this.vert(x, y, u0, v0, c);
        this.vert(x + w, y, u1, v0, c);
        this.vert(x + w, y + hh, u1, v1, c);
        this.vert(x, y + hh, u0, v1, c);
        this.tri(0, 1, 2, base); this.tri(0, 2, 3, base);
        this.grow(6);
    }

    whiteU() => float { return ((F.WHITE_X + 4) as float) / (F.ATLAS_W as float); }
    whiteV() => float { return ((F.WHITE_Y + 8) as float) / (F.ATLAS_H as float); }

    // ---- shapes ----
    fillRect(x: float, y: float, w: float, hh: float, c: u32) => void {
        let u: float = this.whiteU(); let v: float = this.whiteV();
        this.quad(x, y, w, hh, u, v, u, v, c, this.atlas, 0);
    }

    rect(x: float, y: float, w: float, hh: float, t: float, c: u32) => void {
        this.fillRect(x, y, w, t, c);
        this.fillRect(x, y + hh - t, w, t, c);
        this.fillRect(x, y + t, t, hh - 2.0 * t, c);
        this.fillRect(x + w - t, y + t, t, hh - 2.0 * t, c);
    }

    fillTriangle(x1: float, y1: float, x2: float, y2: float, x3: float, y3: float, c: u32) => void {
        let u: float = this.whiteU(); let v: float = this.whiteV();
        this.ensure(this.atlas, 0, 0.0, 0.0);
        let base: u32 = this.vcount;
        this.vert(x1, y1, u, v, c); this.vert(x2, y2, u, v, c); this.vert(x3, y3, u, v, c);
        this.tri(0, 1, 2, base);
        this.grow(3);
    }

    line(x1: float, y1: float, x2: float, y2: float, t: float, c: u32) => void {
        let dx: float = x2 - x1; let dy: float = y2 - y1;
        let l: float = msqrt(dx * dx + dy * dy);
        if (l == 0.0) return;
        let nx: float = (0.0 - dy) / l * t * 0.5;
        let ny: float = dx / l * t * 0.5;
        let u: float = this.whiteU(); let v: float = this.whiteV();
        this.ensure(this.atlas, 0, 0.0, 0.0);
        let base: u32 = this.vcount;
        this.vert(x1 + nx, y1 + ny, u, v, c); this.vert(x2 + nx, y2 + ny, u, v, c);
        this.vert(x2 - nx, y2 - ny, u, v, c); this.vert(x1 - nx, y1 - ny, u, v, c);
        this.tri(0, 1, 2, base); this.tri(0, 2, 3, base);
        this.grow(6);
    }

    circle(cx: float, cy: float, r: float, c: u32) => void {
        let segs: int = 32;
        let u: float = this.whiteU(); let v: float = this.whiteV();
        this.ensure(this.atlas, 0, 0.0, 0.0);
        let base: u32 = this.vcount;
        this.vert(cx, cy, u, v, c);
        let i: int = 0;
        while (i < segs) {
            let a: float = (i as float) * 2.0 * PI / (segs as float);
            this.vert(cx + mcos(a) * r, cy + msin(a) * r, u, v, c);
            i = i + 1;
        }
        i = 0;
        while (i < segs) {
            let nxt: int = (i + 1) % segs;
            this.tri(0, (i + 1) as u32, (nxt + 1) as u32, base);
            i = i + 1;
        }
        this.grow((segs * 3) as u32);
    }

    // ---- images (GL texture ids, e.g. OpenGL.Texture.id or Framebuffer.tex) ----
    image(tex: u32, x: float, y: float, w: float, hh: float) => void {
        this.quad(x, y, w, hh, 0.0, 0.0, 1.0, 1.0, 0xFFFFFFFF, tex, 0);
    }
    imageTinted(tex: u32, x: float, y: float, w: float, hh: float, c: u32) => void {
        this.quad(x, y, w, hh, 0.0, 0.0, 1.0, 1.0, c, tex, 0);
    }
    // Render-target textures are bottom-up: this flips v.
    imageFlipped(tex: u32, x: float, y: float, w: float, hh: float) => void {
        this.quad(x, y, w, hh, 0.0, 1.0, 1.0, 0.0, 0xFFFFFFFF, tex, 0);
    }

    // Fills a rect with a program from createShader(). Breaks the batch.
    shaderRect(p: u32, x: float, y: float, w: float, hh: float) => void {
        this.quad(x, y, w, hh, 0.0, 0.0, 1.0, 1.0, 0xFFFFFFFF, this.atlas, p);
    }

    // ---- text (8x16 glyphs * scale; '\n' starts a new line) ----
    text(x: float, y: float, s: char*, c: u32) => void { this.textScaled(x, y, s, c, 1.0); }

    textScaled(x: float, y: float, s: char*, c: u32, scale: float) => void {
        let gw: float = (F.GLYPH_W as float) * scale;
        let gh: float = (F.GLYPH_H as float) * scale;
        let px: float = x; let py: float = y;
        let aw: float = F.ATLAS_W as float; let ah: float = F.ATLAS_H as float;
        let i: int = 0;
        let more: bool = true;
        while (more) {
            let ch: int = s[i] as int;
            if (ch == 0) { more = false; }
            else {
                if (ch == 10) { px = x; py = py + gh; }
                else {
                    if (ch < 32 || ch > 126) ch = 63;
                    if (ch != 32) {
                        let gi: int = ch - 32;
                        let u0: float = ((gi % 16) * F.GLYPH_W) as float / aw;
                        let v0: float = ((gi / 16) * F.GLYPH_H) as float / ah;
                        let u1: float = u0 + (F.GLYPH_W as float) / aw;
                        let v1: float = v0 + (F.GLYPH_H as float) / ah;
                        this.quad(px, py, gw, gh, u0, v0, u1, v1, c, this.atlas, 0);
                    }
                    px = px + gw;
                }
                i = i + 1;
            }
        }
    }

    textWidth(s: char*, scale: float) => float {
        let n: int = 0; let best: int = 0;
        let i: int = 0; let more: bool = true;
        while (more) {
            let ch: int = s[i] as int;
            if (ch == 0) { more = false; }
            else {
                if (ch == 10) { n = 0; } else { n = n + 1; if (n > best) best = n; }
                i = i + 1;
            }
        }
        return (best * F.GLYPH_W) as float * scale;
    }
    textHeight(scale: float) => float { return (F.GLYPH_H as float) * scale; }

    // ---- backends: draw everything recorded this frame ----
    flush() => void {
        if (this.backend == API_VULKAN) { this.flushVulkan(); }
        else if (this.backend == API_SOFTWARE) { this.flushSoftware(); }
        else { this.flushGL(); }
    }

    reset() => void {
        this.verts.len = 0; this.idx.len = 0; this.cmds.len = 0; this.vcount = 0;
    }

    freeVulkan() => void {
        let vk: VK.Vulkan* = this.vk;
        vk.waitIdle();
        let i: int = 0;
        while (i < 2) {
            vk.destroyBuffer(&this.vkVb[i]); vk.destroyBuffer(&this.vkIb[i]);
            i = i + 1;
        }
        i = 0;
        while (i < MAX_IMAGES) {
            if (this.imgUsed[i] != 0) vk.destroyTexture(&this.vkTex[i]);
            i = i + 1;
        }
        i = 0;
        while (i < this.vkShaderCount) { vk.destroyPipeline(&this.vkShaders[i]); i = i + 1; }
        vk.destroyPipeline(&this.vkPipe);
        CMem.free(this.vkVb as void*); CMem.free(this.vkIb as void*); CMem.free(this.vkTex as void*);
        CMem.free(this.vkShaders as void*); CMem.free(this.vkPc as void*);
    }

    // Grows slot's host-visible vertex/index buffers (safe: that slot's fence was waited in beginFrame).
    vkEnsure(slot: u32, vbytes: u64, ibytes: u64) => bool {
        let vk: VK.Vulkan* = this.vk;
        let vb: VK.Buffer* = &this.vkVb[slot];
        let ib: VK.Buffer* = &this.vkIb[slot];
        if (vb.size < vbytes) {
            vk.destroyBuffer(vb);
            let want: u64 = vbytes * 2;
            if (want < 65536) want = 65536;
            if (!vk.createBuffer(vb, want, VK.VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK.VK_MEMORY_HOST_VISIBLE_COHERENT)) return false;
        }
        if (ib.size < ibytes) {
            vk.destroyBuffer(ib);
            let want2: u64 = ibytes * 2;
            if (want2 < 32768) want2 = 32768;
            if (!vk.createBuffer(ib, want2, VK.VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK.VK_MEMORY_HOST_VISIBLE_COHERENT)) return false;
        }
        return true;
    }

    flushVulkan() => void {
        let vk: VK.Vulkan* = this.vk;
        vk.setClear(this.clearR, this.clearG, this.clearB, this.clearA);
        if (!vk.beginFrame()) { this.reset(); return; }       // minimised / swapchain rebuilt: skip this frame
        let n: u64 = this.cmds.len;
        let okBuf: bool = false;
        if (n > 0) {
            let slot: u32 = vk.frame;
            let vbytes: u64 = this.verts.len * 4;
            let ibytes: u64 = this.idx.len * 4;
            okBuf = this.vkEnsure(slot, vbytes, ibytes);
            if (okBuf) {
                vk.writeBuffer(&this.vkVb[slot], this.verts.data as void*, vbytes);
                vk.writeBuffer(&this.vkIb[slot], this.idx.data as void*, ibytes);
                vk.bindVertexBuffer(&this.vkVb[slot]);
                vk.bindIndexBuffer(&this.vkIb[slot], VK.VK_INDEX_U32);
                let t: float = G.glfwGetTime() as float;
                let ew: int = vk.extent.width as int;
                let eh: int = vk.extent.height as int;
                let i: u64 = 0;
                while (i < n) {
                    let d: DrawCmd = this.cmds.data[i];
                    let pipe: VK.Pipeline* = &this.vkPipe;
                    if (d.prog != 0) pipe = &this.vkShaders[(d.prog as int) - 1];
                    vk.bindPipeline(pipe);
                    this.vkPc[0] = ew as float; this.vkPc[1] = eh as float;
                    this.vkPc[2] = d.rw; this.vkPc[3] = d.rh; this.vkPc[4] = t;
                    vk.pushConstants(pipe, this.vkPc as void*, 20);
                    if (d.prog == 0) vk.bindTexture(pipe, &this.vkTex[(d.tex as int) - 1]);
                    let sc: VK.VkRect2D;
                    let x0: int = 0; let y0: int = 0; let x1: int = ew; let y1: int = eh;
                    if (d.clipOn != 0) {
                        x0 = clampi(d.cx, 0, ew); y0 = clampi(d.cy, 0, eh);
                        x1 = clampi(d.cx + d.cw, 0, ew); y1 = clampi(d.cy + d.ch, 0, eh);
                    }
                    sc.offset.x = x0; sc.offset.y = y0;
                    sc.extent.width = (x1 - x0) as u32; sc.extent.height = (y1 - y0) as u32;
                    VKC.vkCmdSetScissor(vk.curCmd, 0, 1, &sc as void*);
                    VKC.vkCmdDrawIndexed(vk.curCmd, d.count, 1, d.first, 0, 0);
                    i = i + 1;
                }
            }
        }
        vk.endFrame();
        this.reset();
    }

    // ---- software rasteriser ----
    flushSoftware() => void {
        let n: u64 = this.cmds.len;
        if (this.swBuf == nullptr || n == 0) { this.reset(); return; }
        let ci: u64 = 0;
        while (ci < n) {
            let d: DrawCmd = this.cmds.data[ci];
            if (d.prog == 0) {
                let cx0: int = 0; let cy0: int = 0; let cx1: int = this.swW; let cy1: int = this.swH;
                if (d.clipOn != 0) {
                    cx0 = clampi(d.cx, 0, this.swW); cy0 = clampi(d.cy, 0, this.swH);
                    cx1 = clampi(d.cx + d.cw, 0, this.swW); cy1 = clampi(d.cy + d.ch, 0, this.swH);
                }
                let tris: u32 = d.count / 3;
                let t: u32 = 0;
                while (t < tris) {
                    let o: u32 = d.first + t * 3;
                    this.swTri(d.tex, this.idx.data[o], this.idx.data[o + 1], this.idx.data[o + 2], cx0, cy0, cx1, cy1);
                    t = t + 1;
                }
            }
            ci = ci + 1;
        }
        this.reset();
    }

    swTri(tex: u32, i0: u32, i1: u32, i2: u32, cx0: int, cy0: int, cx1: int, cy1: int) => void {
        let vd: float* = this.verts.data;
        let a: u64 = (i0 as u64) * 8; let b: u64 = (i1 as u64) * 8; let c: u64 = (i2 as u64) * 8;
        let x0: float = vd[a]; let y0: float = vd[a + 1];
        let x1: float = vd[b]; let y1: float = vd[b + 1];
        let x2: float = vd[c]; let y2: float = vd[c + 1];
        let area: float = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
        if (area == 0.0) return;
        let fminx: float = x0; if (x1 < fminx) fminx = x1; if (x2 < fminx) fminx = x2;
        let fmaxx: float = x0; if (x1 > fmaxx) fmaxx = x1; if (x2 > fmaxx) fmaxx = x2;
        let fminy: float = y0; if (y1 < fminy) fminy = y1; if (y2 < fminy) fminy = y2;
        let fmaxy: float = y0; if (y1 > fmaxy) fmaxy = y1; if (y2 > fmaxy) fmaxy = y2;
        if (fmaxx < 0.0 || fmaxy < 0.0) return;
        if (fminx < 0.0) fminx = 0.0;
        if (fminy < 0.0) fminy = 0.0;
        let minx: int = clampi(fminx as int, cx0, cx1);
        let miny: int = clampi(fminy as int, cy0, cy1);
        let maxx: int = clampi((fmaxx as int) + 1, cx0, cx1);
        let maxy: int = clampi((fmaxy as int) + 1, cy0, cy1);
        let slot: int = (tex as int) - 1;
        if (slot < 0 || slot >= MAX_IMAGES || this.imgUsed[slot] == 0) return;
        let tp: u8* = this.imgPx[slot];
        let tw: int = this.imgW[slot]; let th: int = this.imgH[slot];
        let inv: float = 1.0 / area;
        let y: int = miny;
        while (y < maxy) {
            let py: float = (y as float) + 0.5;
            let x: int = minx;
            while (x < maxx) {
                let px: float = (x as float) + 0.5;
                let w0: float = ((x1 - px) * (y2 - py) - (x2 - px) * (y1 - py)) * inv;
                let w1: float = ((x2 - px) * (y0 - py) - (x0 - px) * (y2 - py)) * inv;
                let w2: float = 1.0 - w0 - w1;
                if (w0 >= 0.0 && w1 >= 0.0 && w2 >= 0.0) {
                    let u: float = w0 * vd[a + 2] + w1 * vd[b + 2] + w2 * vd[c + 2];
                    let v: float = w0 * vd[a + 3] + w1 * vd[b + 3] + w2 * vd[c + 3];
                    let tx: int = clampi((u * (tw as float)) as int, 0, tw - 1);
                    let ty: int = clampi((v * (th as float)) as int, 0, th - 1);
                    let ti: int = (ty * tw + tx) * 4;
                    let cr: float = w0 * vd[a + 4] + w1 * vd[b + 4] + w2 * vd[c + 4];
                    let cg: float = w0 * vd[a + 5] + w1 * vd[b + 5] + w2 * vd[c + 5];
                    let cb: float = w0 * vd[a + 6] + w1 * vd[b + 6] + w2 * vd[c + 6];
                    let ca: float = w0 * vd[a + 7] + w1 * vd[b + 7] + w2 * vd[c + 7];
                    let sa: float = (tp[ti + 3] as float) / 255.0 * ca;
                    let di: int = (y * this.swW + x) * 4;
                    let isa: float = 1.0 - sa;
                    let sr: float = (tp[ti] as float) * cr;
                    let sg: float = (tp[ti + 1] as float) * cg;
                    let sb: float = (tp[ti + 2] as float) * cb;
                    this.swBuf[di] = (sr * sa + (this.swBuf[di] as float) * isa + 0.5) as u8;
                    this.swBuf[di + 1] = (sg * sa + (this.swBuf[di + 1] as float) * isa + 0.5) as u8;
                    this.swBuf[di + 2] = (sb * sa + (this.swBuf[di + 2] as float) * isa + 0.5) as u8;
                    this.swBuf[di + 3] = ((sa + (this.swBuf[di + 3] as float) / 255.0 * isa) * 255.0 + 0.5) as u8;
                }
                x = x + 1;
            }
            y = y + 1;
        }
    }

    flushGL() => void {
        let n: u64 = this.cmds.len;
        if (n == 0 || this.w == 0 || this.ht == 0) { this.verts.len = 0; this.idx.len = 0; this.cmds.len = 0; return; }
        G.glfwMakeContextCurrent(this.h);
        OGL.viewport(this.w, this.ht);
        GL.glDisable(OGL.DEPTH_TEST);
        GL.glDisable(OGL.CULL_FACE);
        OGL.enableBlend();
        GL.glEnable(OGL.SCISSOR_TEST);
        OGL.glBindVertexArray(this.vao);
        OGL.glBindBuffer(OGL.ARRAY_BUFFER, this.vbo);
        OGL.glBufferData(OGL.ARRAY_BUFFER, (this.verts.len * 4) as i64, this.verts.data as void*, OGL.DYNAMIC_DRAW);
        OGL.glBindBuffer(OGL.ELEMENT_ARRAY_BUFFER, this.ebo);
        OGL.glBufferData(OGL.ELEMENT_ARRAY_BUFFER, (this.idx.len * 4) as i64, this.idx.data as void*, OGL.DYNAMIC_DRAW);
        let t: float = G.glfwGetTime() as float;
        let i: u64 = 0;
        while (i < n) {
            let d: DrawCmd = this.cmds.data[i];
            let p: u32 = this.prog.id;
            if (d.prog != 0) p = d.prog;
            OGL.glUseProgram(p);
            OGL.glUniform2f(OGL.glGetUniformLocation(p, "uScreen"), this.w as float, this.ht as float);
            if (d.prog == 0) {
                OGL.glActiveTexture(OGL.TEXTURE0);
                GL.glBindTexture(OGL.TEXTURE_2D, d.tex);
                OGL.glUniform1i(OGL.glGetUniformLocation(p, "uTex"), 0);
            } else {
                OGL.glUniform2f(OGL.glGetUniformLocation(p, "uSize"), d.rw, d.rh);
                OGL.glUniform1f(OGL.glGetUniformLocation(p, "uTime"), t);
            }
            if (d.clipOn != 0) { GL.glScissor(d.cx, this.ht - d.cy - d.ch, d.cw, d.ch); }
            else { GL.glScissor(0, 0, this.w, this.ht); }
            GL.glDrawElements(OGL.TRIANGLES, d.count as int, OGL.UNSIGNED_INT, ((d.first * 4) as i64) as void*);
            i = i + 1;
        }
        GL.glDisable(OGL.SCISSOR_TEST);
        OGL.glBindVertexArray(0);
        this.verts.len = 0; this.idx.len = 0; this.cmds.len = 0; this.vcount = 0;
    }
}

// ---------------------------------------------------------------------------------------------
// Pen3D (GL windows only: ready == false on Vulkan/software windows and every call is a no-op)
// ---------------------------------------------------------------------------------------------
struct Pen3D {
    h: void* = nullptr;
    prog: OGL.Program;
    view: float* = nullptr; proj: float* = nullptr; vp: float* = nullptr; tmp: float* = nullptr;
    lightX: float = 0.4; lightY: float = 0.8; lightZ: float = 0.5;
    w: int = 0; ht: int = 0;
    ready: bool = false;

    init(handle: void*) => bool {
        this.h = handle;
        this.lightX = 0.4; this.lightY = 0.8; this.lightZ = 0.5;       // Win memsets us to 0, so set defaults here
        if (!this.prog.build(vs3d(), fs3d())) return false;
        this.view = CMem.malloc(64) as float*; this.proj = CMem.malloc(64) as float*;
        this.vp = CMem.malloc(64) as float*; this.tmp = CMem.malloc(64) as float*;
        matIdentity(this.view); matIdentity(this.proj); matIdentity(this.vp);
        this.ready = true;
        return true;
    }

    free() => void {
        if (!this.ready) return;
        G.glfwMakeContextCurrent(this.h);
        this.prog.free();
        CMem.free(this.view as void*); CMem.free(this.proj as void*); CMem.free(this.vp as void*); CMem.free(this.tmp as void*);
        this.ready = false;
    }

    size(w: int, hh: int) => void { this.w = w; this.ht = hh; }

    // Clears colour + depth.
    clear(r: float, g: float, b: float, a: float) => void {
        if (!this.ready) return;
        G.glfwMakeContextCurrent(this.h);
        OGL.viewport(this.w, this.ht);
        GL.glEnable(OGL.DEPTH_TEST);
        GL.glDepthMask(1 as u8);
        OGL.clear(r, g, b, a);
    }

    // Eye/target in world space, y up. Aspect comes from the window.
    camera(ex: float, ey: float, ez: float, tx: float, ty: float, tz: float, fovDeg: float) => void {
        if (!this.ready) return;
        let aspect: float = 1.0;
        if (this.ht > 0) aspect = (this.w as float) / (this.ht as float);
        matLookAt(this.view, ex, ey, ez, tx, ty, tz);
        matPerspective(this.proj, fovDeg, aspect, 0.1, 500.0);
        matMul(this.vp, this.proj, this.view);
    }

    light(x: float, y: float, z: float) => void { this.lightX = x; this.lightY = y; this.lightZ = z; }

    // Interleaved pos(3) normal(3) uv(2). idx may be nullptr / 0 for non-indexed.
    createMesh(out: OGL.Mesh*, verts: float*, floatCount: int, idx: u32*, idxCount: int) => void {
        if (!this.ready) return;
        G.glfwMakeContextCurrent(this.h);
        let layout = CMem.malloc(12) as int*;
        layout[0] = 3; layout[1] = 3; layout[2] = 2;
        out.uploadLayout(verts, floatCount, layout, 3);
        CMem.free(layout as void*);
        if (idxCount > 0) out.setIndices(idx, idxCount);
    }

    // Unit cube centred on the origin (24 verts, 36 indices).
    createCube(out: OGL.Mesh*) => void {
        if (!this.ready) return;
        let v = CMem.malloc(24 * 8 * 4) as float*;
        let ix = CMem.malloc(36 * 4) as u32*;
        // face: normal, u axis, v axis
        this.face(v, 0, 1.0, 0.0, 0.0,  0.0, 0.0, 0.0 - 1.0,  0.0, 1.0, 0.0);
        this.face(v, 1, 0.0 - 1.0, 0.0, 0.0,  0.0, 0.0, 1.0,  0.0, 1.0, 0.0);
        this.face(v, 2, 0.0, 1.0, 0.0,  1.0, 0.0, 0.0,  0.0, 0.0, 0.0 - 1.0);
        this.face(v, 3, 0.0, 0.0 - 1.0, 0.0,  1.0, 0.0, 0.0,  0.0, 0.0, 1.0);
        this.face(v, 4, 0.0, 0.0, 1.0,  1.0, 0.0, 0.0,  0.0, 1.0, 0.0);
        this.face(v, 5, 0.0, 0.0, 0.0 - 1.0,  0.0 - 1.0, 0.0, 0.0,  0.0, 1.0, 0.0);
        let f: int = 0;
        while (f < 6) {
            let b: u32 = (f * 4) as u32;
            let o: int = f * 6;
            ix[o] = b; ix[o + 1] = b + 1; ix[o + 2] = b + 2; ix[o + 3] = b; ix[o + 4] = b + 2; ix[o + 5] = b + 3;
            f = f + 1;
        }
        this.createMesh(out, v, 24 * 8, ix, 36);
        CMem.free(v as void*); CMem.free(ix as void*);
    }

    face(v: float*, f: int, nx: float, ny: float, nz: float, ux: float, uy: float, uz: float, vx: float, vy: float, vz: float) => void {
        let k: int = 0;
        while (k < 4) {
            let su: float = 0.0 - 0.5; let sv: float = 0.0 - 0.5;
            if (k == 1 || k == 2) su = 0.5;
            if (k == 2 || k == 3) sv = 0.5;
            let o: int = (f * 4 + k) * 8;
            v[o] = nx * 0.5 + ux * su + vx * sv;
            v[o + 1] = ny * 0.5 + uy * su + vy * sv;
            v[o + 2] = nz * 0.5 + uz * su + vz * sv;
            v[o + 3] = nx; v[o + 4] = ny; v[o + 5] = nz;
            v[o + 6] = su + 0.5; v[o + 7] = sv + 0.5;
            k = k + 1;
        }
    }

    // model: float*(16) column-major. Colour is 0xRRGGBBAA.
    draw(m: OGL.Mesh*, model: float*, c: u32) => void { this.drawTextured(m, model, c, 0); }

    // tex = GL texture id, 0 = untextured (colour only). Texture is multiplied by the colour.
    drawTextured(m: OGL.Mesh*, model: float*, c: u32, tex: u32) => void {
        if (!this.ready) return;
        G.glfwMakeContextCurrent(this.h);
        OGL.viewport(this.w, this.ht);
        GL.glEnable(OGL.DEPTH_TEST);
        GL.glDepthFunc(OGL.LESS);
        GL.glDisable(OGL.SCISSOR_TEST);
        OGL.enableBlend();
        this.prog.use();
        this.prog.setMat4("uVP", this.vp);
        this.prog.setMat4("uModel", model);
        this.prog.set4f("uColor", chan(c, 24), chan(c, 16), chan(c, 8), chan(c, 0));
        this.prog.set3f("uLight", this.lightX, this.lightY, this.lightZ);
        if (tex != 0) {
            OGL.glActiveTexture(OGL.TEXTURE0);
            GL.glBindTexture(OGL.TEXTURE_2D, tex);
            this.prog.set1i("uTex", 0);
            this.prog.set1i("uUseTex", 1);
        } else { this.prog.set1i("uUseTex", 0); }
        m.draw();
        OGL.glBindVertexArray(0);
        GL.glDisable(OGL.DEPTH_TEST);
    }
}

// ---------------------------------------------------------------------------------------------
// Win: native window + backend + pens + input
// ---------------------------------------------------------------------------------------------
let KEY_FIRST: int = 32;
let KEY_LAST: int = 348;
let KEY_COUNT: int = 349;

// ASCII for a GLFW printable key code (0 if none), honouring shift on a US layout.
function keyToChar(k: int, shift: bool) => int {
    let r: int = 0;
    if (k >= 65 && k <= 90) { r = k + 32; if (shift) r = k; }
    else if (k >= 48 && k <= 57) {
        r = k;
        if (shift) {
            if (k == 48) r = 41; if (k == 49) r = 33; if (k == 50) r = 64; if (k == 51) r = 35; if (k == 52) r = 36;
            if (k == 53) r = 37; if (k == 54) r = 94; if (k == 55) r = 38; if (k == 56) r = 42; if (k == 57) r = 40;
        }
    }
    else if (k == 32) r = 32;
    else if (k == 39) { r = 39; if (shift) r = 34; }
    else if (k == 44) { r = 44; if (shift) r = 60; }
    else if (k == 45) { r = 45; if (shift) r = 95; }
    else if (k == 46) { r = 46; if (shift) r = 62; }
    else if (k == 47) { r = 47; if (shift) r = 63; }
    else if (k == 59) { r = 59; if (shift) r = 58; }
    else if (k == 61) { r = 61; if (shift) r = 43; }
    else if (k == 91) { r = 91; if (shift) r = 123; }
    else if (k == 92) { r = 92; if (shift) r = 124; }
    else if (k == 93) { r = 93; if (shift) r = 125; }
    else if (k == 96) { r = 96; if (shift) r = 126; }
    return r;
}

struct Win {
    win: W.Window;
    pen: Pen2D;
    scene: Pen3D;
    api: int = 0;
    vk: VK.Vulkan;                      // Vulkan windows: this window's presentation target (use it for your own 3D)
    blit: Pen2D;                        // software windows: GL pen that shows the CPU buffer
    blitTex: OGL.Texture;
    blitW: int = 0; blitH: int = 0;
    isOpen: bool = false;
    active: bool = false;               // false while minimised: skip heavy work
    fbW: int = 0; fbH: int = 0;
    // mouse (framebuffer pixel coordinates; same space the pens draw in)
    mx: float = 0.0; my: float = 0.0;
    ldown: bool = false; lprev: bool = false;
    rdown: bool = false; rprev: bool = false;
    // keyboard
    keyNow: u8* = nullptr; keyPrev: u8* = nullptr;
    typed: char* = nullptr; ntyped: int = 0;     // characters typed this frame (NUL-terminated)
    bsHeld: double = 0.0; bsNext: double = 0.0; bsEvent: bool = false;

    // core: Canvas-owned Vulkan device holder (zeroed; first Vulkan window fills it). share: window to share GL
    // objects with (or nullptr). dir: SPIR-V folder for the Vulkan backend.
    init(title: char*, w: int, hh: int, apiWanted: int, core: VK.Vulkan*, share: W.Window*, dir: char*, validate: bool) => bool {
        this.api = apiWanted;
        let ok: bool = false;
        if (apiWanted == API_VULKAN) { ok = this.initVulkanWin(title, w, hh, core, dir, validate); }
        else { ok = this.initGLWin(title, w, hh, share, apiWanted == API_SOFTWARE); }
        if (!ok) return false;
        this.keyNow = CMem.malloc(KEY_COUNT as u64) as u8*;
        this.keyPrev = CMem.malloc(KEY_COUNT as u64) as u8*;
        CMem.memset(this.keyNow as void*, 0, KEY_COUNT as u64);
        CMem.memset(this.keyPrev as void*, 0, KEY_COUNT as u64);
        this.typed = CMem.malloc(32) as char*;
        this.typed[0] = 0 as char;
        this.isOpen = true;
        return true;
    }

    initGLWin(title: char*, w: int, hh: int, share: W.Window*, software: bool) => bool {
        let opened: bool = false;
        if (share != nullptr) { opened = this.win.openGLShared(w, hh, title, share); }
        else { opened = this.win.openGL(w, hh, title); }
        if (!opened) return false;
        this.win.vsync(true);
        if (!OGL.isLoaded()) OGL.load();
        if (!OGL.isLoaded()) { this.win.close(); return false; }
        if (software) {
            if (!this.blit.init(this.win.h)) { this.win.close(); return false; }
            this.pen.initSoftware(w, hh);
            return true;
        }
        if (!this.pen.init(this.win.h)) { this.win.close(); return false; }
        if (!this.scene.init(this.win.h)) { this.pen.free(); this.win.close(); return false; }
        return true;
    }

    initVulkanWin(title: char*, w: int, hh: int, core: VK.Vulkan*, dir: char*, validate: bool) => bool {
        if (!this.win.openVulkan(w, hh, title)) return false;
        if (core.device == nullptr) {
            core.owner = true; core.vsync = true;
            if (!core.initCore(this.win.h, "OmniScript", validate)) { this.win.close(); return false; }
        }
        this.vk.vsync = true;
        if (!this.vk.attach(core, this.win.h)) { this.vk.free(); this.win.close(); return false; }
        if (!this.pen.initVulkan(&this.vk, dir)) { this.pen.free(); this.vk.free(); this.win.close(); return false; }
        return true;
    }

    context2d() => Pen2D* { return &this.pen; }
    context3d() => Pen3D* { return &this.scene; }

    setTitle(t: char*) => void { this.win.setTitle(t); }
    close() => void { this.win.requestClose(); }

    // ---- input queries (valid after frame() started) ----
    mouseDown() => bool { return this.ldown; }
    mousePressed() => bool { return this.ldown && !this.lprev; }
    mouseReleased() => bool { return !this.ldown && this.lprev; }
    rightDown() => bool { return this.rdown; }
    rightPressed() => bool { return this.rdown && !this.rprev; }
    keyDown(k: int) => bool { return this.keyNow[k] != 0; }
    keyPressed(k: int) => bool { return this.keyNow[k] != 0 && this.keyPrev[k] == 0; }
    shiftDown() => bool { return this.keyNow[W.KEY_LEFT_SHIFT] != 0 || this.keyNow[344] != 0; }
    ctrlDown() => bool { return this.keyNow[W.KEY_LEFT_CONTROL] != 0 || this.keyNow[345] != 0; }
    // Backspace with key repeat (fires on press, then repeats while held).
    backspace() => bool { return this.bsEvent; }
    // Characters typed this frame, NUL-terminated (ASCII only; poll-based, no callback).
    typedText() => char* { return this.typed; }
    width() => int { return this.fbW; }
    height() => int { return this.fbH; }

    pollInput() => void {
        let k: int = KEY_FIRST;
        this.ntyped = 0;
        let shift: bool = false;
        while (k <= KEY_LAST) {
            this.keyPrev[k] = this.keyNow[k];
            let d: bool = this.win.keyDown(k);
            if (d) this.keyNow[k] = 1 as u8; else this.keyNow[k] = 0 as u8;
            k = k + 1;
        }
        shift = this.shiftDown();
        k = 32;
        while (k <= 96) {
            if (this.keyNow[k] != 0 && this.keyPrev[k] == 0 && this.ntyped < 30) {
                let c: int = keyToChar(k, shift);
                if (c != 0) { this.typed[this.ntyped] = c as char; this.ntyped = this.ntyped + 1; }
            }
            k = k + 1;
        }
        this.typed[this.ntyped] = 0 as char;

        let now: double = W.time();
        this.bsEvent = false;
        if (this.keyNow[W.KEY_BACKSPACE] != 0) {
            if (this.keyPrev[W.KEY_BACKSPACE] == 0) { this.bsEvent = true; this.bsNext = now + 0.4; }
            else if (now >= this.bsNext) { this.bsEvent = true; this.bsNext = now + 0.04; }
        }

        this.lprev = this.ldown; this.rprev = this.rdown;
        this.ldown = this.win.mouseDown(W.MOUSE_LEFT);
        this.rdown = this.win.mouseDown(W.MOUSE_RIGHT);
        let cx: double = 0.0; let cy: double = 0.0;
        this.win.cursor(&cx, &cy);
        let ww: int = 0; let wh: int = 0;
        this.win.size(&ww, &wh);
        let sx: float = 1.0; let sy: float = 1.0;
        if (ww > 0) sx = (this.fbW as float) / (ww as float);
        if (wh > 0) sy = (this.fbH as float) / (wh as float);
        this.mx = (cx as float) * sx;
        this.my = (cy as float) * sy;
    }

    beginFrame() => void {
        if (this.api != API_VULKAN) this.win.makeCurrent();
        let w: int = 0; let hh: int = 0;
        this.win.framebufferSize(&w, &hh);
        this.fbW = w; this.fbH = hh;
        this.active = w > 0 && hh > 0;
        this.pollInput();
        this.pen.begin(w, hh);
        this.scene.size(w, hh);
    }

    endFrame() => void {
        if (this.api == API_VULKAN) { this.pen.flush(); return; }       // flush records, submits and presents
        this.win.makeCurrent();
        this.pen.flush();
        if (this.api == API_SOFTWARE) this.blitSoftware();
        this.win.swap();
    }

    // Uploads the CPU framebuffer to a GL texture and draws it 1:1 through the blit pen.
    blitSoftware() => void {
        let w: int = this.pen.pixelWidth(); let hh: int = this.pen.pixelHeight();
        if (w <= 0 || hh <= 0 || this.fbW != w || this.fbH != hh) return;
        if (this.blitW != w || this.blitH != hh) {
            if (this.blitTex.id != 0) this.blitTex.free();
            this.blitTex.fromRGBA(this.pen.pixels(), w, hh, false);
            this.blitW = w; this.blitH = hh;
        } else {
            GL.glBindTexture(OGL.TEXTURE_2D, this.blitTex.id);
            GL.glTexSubImage2D(OGL.TEXTURE_2D, 0, 0, 0, w, hh, OGL.RGBA, OGL.UNSIGNED_BYTE, this.pen.pixels() as void*);
        }
        this.blit.begin(w, hh);
        this.blit.image(this.blitTex.id, 0.0, 0.0, w as float, hh as float);
        this.blit.flush();
    }

    destroy() => void {
        if (!this.isOpen) return;
        if (this.api == API_VULKAN) {
            this.pen.free();              // waits for the device, destroys buffers/pipelines/textures
            this.vk.free();               // this window's swapchain, surface, sync (device stays with the core)
        } else {
            this.win.makeCurrent();
            if (this.api == API_SOFTWARE) { if (this.blitTex.id != 0) this.blitTex.free(); this.blit.free(); }
            this.pen.free();
            this.scene.free();
        }
        this.win.close();
        if (this.keyNow != nullptr) { CMem.free(this.keyNow as void*); this.keyNow = nullptr; }
        if (this.keyPrev != nullptr) { CMem.free(this.keyPrev as void*); this.keyPrev = nullptr; }
        if (this.typed != nullptr) { CMem.free(this.typed as void*); this.typed = nullptr; }
        this.isOpen = false;
    }
}

// ---------------------------------------------------------------------------------------------
// Canvas: app root
// ---------------------------------------------------------------------------------------------
struct Canvas {
    wins: Win** = nullptr;
    count: int = 0;
    started: bool = false;
    vkCore: VK.Vulkan* = nullptr;       // shared Vulkan device, created by the first Vulkan window
    firstGL: Win* = nullptr;            // GL windows share textures/buffers with this one
    shaderDir: char* = "shaders/";      // where canvas2d.{vert,frag}.spv live (Vulkan windows)
    validation: bool = false;           // set before the first Vulkan window to enable the validation layer

    init() => bool {
        if (!W.init()) return false;
        this.wins = CMem.malloc((MAX_WINDOWS * 8) as u64) as Win**;
        this.count = 0;
        this.started = true;
        return true;
    }

    // api: API_AUTO (GL, falling back to Vulkan), API_GL, API_VULKAN or API_SOFTWARE. Windows of different
    // backends can coexist. Returns nullptr on failure.
    createWindow(title: char*, w: int, hh: int) => Win* { return this.createWindowEx(title, w, hh, API_AUTO); }

    createWindowEx(title: char*, w: int, hh: int, api: int) => Win* {
        if (!this.started || this.count >= MAX_WINDOWS) return nullptr;
        let want: int = api;
        if (want == API_AUTO) want = API_GL;
        let win: Win* = this.makeWin(title, w, hh, want);
        if (win == nullptr && api == API_AUTO) win = this.makeWin(title, w, hh, API_VULKAN);   // no GL: try Vulkan
        if (win == nullptr) return nullptr;
        this.wins[this.count] = win;
        this.count = this.count + 1;
        if (this.firstGL == nullptr && win.api != API_VULKAN) this.firstGL = win;
        return win;
    }

    makeWin(title: char*, w: int, hh: int, api: int) => Win* {
        if (api == API_VULKAN && this.vkCore == nullptr) {
            this.vkCore = CMem.malloc(sizeof(VK.Vulkan)) as VK.Vulkan*;
            CMem.memset(this.vkCore as void*, 0, sizeof(VK.Vulkan));
        }
        let share: W.Window* = nullptr;
        if (api != API_VULKAN && this.firstGL != nullptr && this.firstGL.isOpen) share = &this.firstGL.win;
        let win = CMem.malloc(sizeof(Win)) as Win*;
        CMem.memset(win as void*, 0, sizeof(Win));
        if (!win.init(title, w, hh, api, this.vkCore, share, this.shaderDir, this.validation)) { CMem.free(win as void*); return nullptr; }
        return win;
    }

    // Polls events, drops windows the user closed, starts a frame on every open window.
    // Returns false once no window is left open.
    frame() => bool {
        W.pollEvents();
        let i: int = 0;
        while (i < this.count) {
            let w: Win* = this.wins[i];
            if (w.isOpen && w.win.shouldClose()) { w.destroy(); }
            i = i + 1;
        }
        let alive: int = 0;
        i = 0;
        while (i < this.count) {
            let w: Win* = this.wins[i];
            if (w.isOpen) { w.beginFrame(); alive = alive + 1; }
            i = i + 1;
        }
        return alive > 0;
    }

    // Flushes every 2D pen and presents every open window.
    present() => void {
        let i: int = 0;
        while (i < this.count) {
            let w: Win* = this.wins[i];
            if (w.isOpen) w.endFrame();
            i = i + 1;
        }
    }

    time() => double { return W.time(); }

    quit() => void {
        let i: int = 0;
        while (i < this.count) {
            let w: Win* = this.wins[i];
            w.destroy();
            CMem.free(w as void*);
            i = i + 1;
        }
        this.count = 0;
        if (this.vkCore != nullptr) {
            if (this.vkCore.device != nullptr) this.vkCore.free();
            CMem.free(this.vkCore as void*);
            this.vkCore = nullptr;
        }
        if (this.wins != nullptr) { CMem.free(this.wins as void*); this.wins = nullptr; }
        if (this.started) { W.terminate(); this.started = false; }
    }
}
