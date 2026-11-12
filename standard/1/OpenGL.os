// standard/1/OpenGL.os
// Tier 5: complete OpenGL 3.3 core helpers on top of c/opengl.os (GL 1.1) + c/glfw.os.
//   constants, runtime loader (glfwGetProcAddress), Shader program with error log + uniforms,
//   Mesh (multi-attribute VBO/VAO, optional index buffer), Texture (RGBA, mipmaps), Framebuffer.
// Usage: create the window with GL 3.3 core hints (see examples/gl/triangle.os), make the context
// current, call load() once, then use the helpers. No `return` inside loop bodies (compiler quirk).
module GL = import "standard/1/c/opengl.os";
module G = import "standard/1/c/glfw.os";
module CMem = import "standard/1/c/mem.os";

// ---- constants ----
let COLOR_BUFFER_BIT: u32 = 0x4000;
let DEPTH_BUFFER_BIT: u32 = 0x100;
let POINTS: u32 = 0;
let LINES: u32 = 1;
let LINE_STRIP: u32 = 3;
let TRIANGLES: u32 = 4;
let TRIANGLE_STRIP: u32 = 5;
let TRIANGLE_FAN: u32 = 6;
let DEPTH_TEST: u32 = 0xB71;
let CULL_FACE: u32 = 0xB44;
let BLEND: u32 = 0xBE2;
let SCISSOR_TEST: u32 = 0xC11;
let SRC_ALPHA: u32 = 0x302;
let ONE_MINUS_SRC_ALPHA: u32 = 0x303;
let LESS: u32 = 0x201;
let LEQUAL: u32 = 0x203;
let BACK: u32 = 0x405;
let CCW: u32 = 0x901;
let TEXTURE_2D: u32 = 0xDE1;
let TEXTURE0: u32 = 0x84C0;
let TEXTURE_MAG_FILTER: u32 = 0x2800;
let TEXTURE_MIN_FILTER: u32 = 0x2801;
let TEXTURE_WRAP_S: u32 = 0x2802;
let TEXTURE_WRAP_T: u32 = 0x2803;
let NEAREST: int = 0x2600;
let LINEAR: int = 0x2601;
let LINEAR_MIPMAP_LINEAR: int = 0x2703;
let REPEAT: int = 0x2901;
let CLAMP_TO_EDGE: int = 0x812F;
let RGBA: u32 = 0x1908;
let RGB: u32 = 0x1907;
let RGBA8: int = 0x8058;
let UNSIGNED_BYTE: u32 = 0x1401;
let UNSIGNED_SHORT: u32 = 0x1403;
let UNSIGNED_INT: u32 = 0x1405;
let FLOAT: u32 = 0x1406;
let ARRAY_BUFFER: u32 = 0x8892;
let ELEMENT_ARRAY_BUFFER: u32 = 0x8893;
let STATIC_DRAW: u32 = 0x88E4;
let DYNAMIC_DRAW: u32 = 0x88E8;
let VERTEX_SHADER: u32 = 0x8B31;
let FRAGMENT_SHADER: u32 = 0x8B30;
let COMPILE_STATUS: u32 = 0x8B81;
let LINK_STATUS: u32 = 0x8B82;
let FRAMEBUFFER: u32 = 0x8D40;
let COLOR_ATTACHMENT0: u32 = 0x8CE0;
let FRAMEBUFFER_COMPLETE: u32 = 0x8CD5;
let VERSION: u32 = 0x1F02;
let RENDERER: u32 = 0x1F01;

// ---- GL 2.0+ function pointers (filled by load()) ----
let glCreateShader: fn(u32) => u32 = nullptr;
let glShaderSource: fn(u32, int, char**, int*) => void = nullptr;
let glCompileShader: fn(u32) => void = nullptr;
let glGetShaderiv: fn(u32, u32, int*) => void = nullptr;
let glGetShaderInfoLog: fn(u32, int, int*, char*) => void = nullptr;
let glCreateProgram: fn() => u32 = nullptr;
let glAttachShader: fn(u32, u32) => void = nullptr;
let glLinkProgram: fn(u32) => void = nullptr;
let glGetProgramiv: fn(u32, u32, int*) => void = nullptr;
let glGetProgramInfoLog: fn(u32, int, int*, char*) => void = nullptr;
let glUseProgram: fn(u32) => void = nullptr;
let glDeleteShader: fn(u32) => void = nullptr;
let glDeleteProgram: fn(u32) => void = nullptr;
let glGenBuffers: fn(int, u32*) => void = nullptr;
let glDeleteBuffers: fn(int, u32*) => void = nullptr;
let glBindBuffer: fn(u32, u32) => void = nullptr;
let glBufferData: fn(u32, i64, void*, u32) => void = nullptr;
let glBufferSubData: fn(u32, i64, i64, void*) => void = nullptr;
let glGenVertexArrays: fn(int, u32*) => void = nullptr;
let glDeleteVertexArrays: fn(int, u32*) => void = nullptr;
let glBindVertexArray: fn(u32) => void = nullptr;
let glVertexAttribPointer: fn(u32, int, u32, u8, int, void*) => void = nullptr;
let glEnableVertexAttribArray: fn(u32) => void = nullptr;
let glGetUniformLocation: fn(u32, char*) => int = nullptr;
let glUniform1f: fn(int, float) => void = nullptr;
let glUniform2f: fn(int, float, float) => void = nullptr;
let glUniform3f: fn(int, float, float, float) => void = nullptr;
let glUniform4f: fn(int, float, float, float, float) => void = nullptr;
let glUniform1i: fn(int, int) => void = nullptr;
let glUniformMatrix4fv: fn(int, int, u8, float*) => void = nullptr;
let glActiveTexture: fn(u32) => void = nullptr;
let glGenerateMipmap: fn(u32) => void = nullptr;
let glGenFramebuffers: fn(int, u32*) => void = nullptr;
let glDeleteFramebuffers: fn(int, u32*) => void = nullptr;
let glBindFramebuffer: fn(u32, u32) => void = nullptr;
let glFramebufferTexture2D: fn(u32, u32, u32, u32, int) => void = nullptr;
let glCheckFramebufferStatus: fn(u32) => u32 = nullptr;

let lastLog: char* = nullptr;       // last shader/program error text (see lastError())
let loaded: bool = false;

function load() => void {
    glCreateShader = G.glfwGetProcAddress("glCreateShader") as fn(u32) => u32;
    glShaderSource = G.glfwGetProcAddress("glShaderSource") as fn(u32, int, char**, int*) => void;
    glCompileShader = G.glfwGetProcAddress("glCompileShader") as fn(u32) => void;
    glGetShaderiv = G.glfwGetProcAddress("glGetShaderiv") as fn(u32, u32, int*) => void;
    glGetShaderInfoLog = G.glfwGetProcAddress("glGetShaderInfoLog") as fn(u32, int, int*, char*) => void;
    glCreateProgram = G.glfwGetProcAddress("glCreateProgram") as fn() => u32;
    glAttachShader = G.glfwGetProcAddress("glAttachShader") as fn(u32, u32) => void;
    glLinkProgram = G.glfwGetProcAddress("glLinkProgram") as fn(u32) => void;
    glGetProgramiv = G.glfwGetProcAddress("glGetProgramiv") as fn(u32, u32, int*) => void;
    glGetProgramInfoLog = G.glfwGetProcAddress("glGetProgramInfoLog") as fn(u32, int, int*, char*) => void;
    glUseProgram = G.glfwGetProcAddress("glUseProgram") as fn(u32) => void;
    glDeleteShader = G.glfwGetProcAddress("glDeleteShader") as fn(u32) => void;
    glDeleteProgram = G.glfwGetProcAddress("glDeleteProgram") as fn(u32) => void;
    glGenBuffers = G.glfwGetProcAddress("glGenBuffers") as fn(int, u32*) => void;
    glDeleteBuffers = G.glfwGetProcAddress("glDeleteBuffers") as fn(int, u32*) => void;
    glBindBuffer = G.glfwGetProcAddress("glBindBuffer") as fn(u32, u32) => void;
    glBufferData = G.glfwGetProcAddress("glBufferData") as fn(u32, i64, void*, u32) => void;
    glBufferSubData = G.glfwGetProcAddress("glBufferSubData") as fn(u32, i64, i64, void*) => void;
    glGenVertexArrays = G.glfwGetProcAddress("glGenVertexArrays") as fn(int, u32*) => void;
    glDeleteVertexArrays = G.glfwGetProcAddress("glDeleteVertexArrays") as fn(int, u32*) => void;
    glBindVertexArray = G.glfwGetProcAddress("glBindVertexArray") as fn(u32) => void;
    glVertexAttribPointer = G.glfwGetProcAddress("glVertexAttribPointer") as fn(u32, int, u32, u8, int, void*) => void;
    glEnableVertexAttribArray = G.glfwGetProcAddress("glEnableVertexAttribArray") as fn(u32) => void;
    glGetUniformLocation = G.glfwGetProcAddress("glGetUniformLocation") as fn(u32, char*) => int;
    glUniform1f = G.glfwGetProcAddress("glUniform1f") as fn(int, float) => void;
    glUniform2f = G.glfwGetProcAddress("glUniform2f") as fn(int, float, float) => void;
    glUniform3f = G.glfwGetProcAddress("glUniform3f") as fn(int, float, float, float) => void;
    glUniform4f = G.glfwGetProcAddress("glUniform4f") as fn(int, float, float, float, float) => void;
    glUniform1i = G.glfwGetProcAddress("glUniform1i") as fn(int, int) => void;
    glUniformMatrix4fv = G.glfwGetProcAddress("glUniformMatrix4fv") as fn(int, int, u8, float*) => void;
    glActiveTexture = G.glfwGetProcAddress("glActiveTexture") as fn(u32) => void;
    glGenerateMipmap = G.glfwGetProcAddress("glGenerateMipmap") as fn(u32) => void;
    glGenFramebuffers = G.glfwGetProcAddress("glGenFramebuffers") as fn(int, u32*) => void;
    glDeleteFramebuffers = G.glfwGetProcAddress("glDeleteFramebuffers") as fn(int, u32*) => void;
    glBindFramebuffer = G.glfwGetProcAddress("glBindFramebuffer") as fn(u32, u32) => void;
    glFramebufferTexture2D = G.glfwGetProcAddress("glFramebufferTexture2D") as fn(u32, u32, u32, u32, int) => void;
    glCheckFramebufferStatus = G.glfwGetProcAddress("glCheckFramebufferStatus") as fn(u32) => u32;
    if (lastLog == nullptr) lastLog = CMem.malloc(4096) as char*;
    lastLog[0] = 0 as char;
    loaded = glCreateShader != nullptr && glGenFramebuffers != nullptr;
}

function isLoaded() => bool { return loaded; }

// Text of the last compile/link failure ("" if none).
function lastError() => char* { return lastLog; }

function glVersion() => char* { return GL.glGetString(VERSION); }

function clear(r: float, g: float, b: float, a: float) => void {
    GL.glClearColor(r, g, b, a);
    GL.glClear(COLOR_BUFFER_BIT | DEPTH_BUFFER_BIT);
}

function viewport(w: int, h: int) => void { GL.glViewport(0, 0, w, h); }
function enableBlend() => void { GL.glEnable(BLEND); GL.glBlendFunc(SRC_ALPHA, ONE_MINUS_SRC_ALPHA); }
function enableDepth() => void { GL.glEnable(DEPTH_TEST); GL.glDepthFunc(LESS); }
function enableCull() => void { GL.glEnable(CULL_FACE); GL.glCullFace(BACK); GL.glFrontFace(CCW); }

// ---- shaders ----
function compile(kind: u32, src: char*) => u32 {
    let sh: u32 = glCreateShader(kind);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    let ok: int = 0;
    glGetShaderiv(sh, COMPILE_STATUS, &ok);
    if (ok == 0) {
        let n: int = 0;
        glGetShaderInfoLog(sh, 4095, &n, lastLog);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

// Returns program id, 0 on failure (see lastError()).
function program(vs: char*, fs: char*) => u32 {
    lastLog[0] = 0 as char;
    let v: u32 = compile(VERTEX_SHADER, vs);
    if (v == 0) return 0;
    let f: u32 = compile(FRAGMENT_SHADER, fs);
    if (f == 0) { glDeleteShader(v); return 0; }
    let p: u32 = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f);
    let ok: int = 0;
    glGetProgramiv(p, LINK_STATUS, &ok);
    if (ok == 0) {
        let n: int = 0;
        glGetProgramInfoLog(p, 4095, &n, lastLog);
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

struct Program {
    id: u32 = 0;
    // false on failure; read OpenGL.lastError() for the compiler log
    build(vs: char*, fs: char*) => bool { this.id = program(vs, fs); return this.id != 0; }
    use() => void { glUseProgram(this.id); }
    loc(name: char*) => int { return glGetUniformLocation(this.id, name); }
    set1f(name: char*, v: float) => void { glUniform1f(this.loc(name), v); }
    set2f(name: char*, a: float, b: float) => void { glUniform2f(this.loc(name), a, b); }
    set3f(name: char*, a: float, b: float, c: float) => void { glUniform3f(this.loc(name), a, b, c); }
    set4f(name: char*, a: float, b: float, c: float, d: float) => void { glUniform4f(this.loc(name), a, b, c, d); }
    set1i(name: char*, v: int) => void { glUniform1i(this.loc(name), v); }
    // m: 16 floats, column-major
    setMat4(name: char*, m: float*) => void { glUniformMatrix4fv(this.loc(name), 1, 0, m); }
    free() => void { if (this.id != 0) glDeleteProgram(this.id); this.id = 0; }
}

// ---- meshes ----
// Interleaved float vertices. layout = floats per attribute, e.g. {2, 3} = vec2 position + vec3 colour.
struct Mesh {
    vao: u32 = 0; vbo: u32 = 0; ebo: u32 = 0;
    count: int = 0;          // vertices, or indices when indexed
    indexed: bool = false;
    indexKind: u32 = 0;

    uploadLayout(verts: float*, floatCount: int, layout: int*, attrCount: int) => void {
        let stride: int = 0;
        let i: int = 0;
        while (i < attrCount) { stride = stride + layout[i]; i = i + 1; }
        glGenVertexArrays(1, &this.vao);
        glGenBuffers(1, &this.vbo);
        glBindVertexArray(this.vao);
        glBindBuffer(ARRAY_BUFFER, this.vbo);
        glBufferData(ARRAY_BUFFER, (floatCount * 4) as i64, verts as void*, STATIC_DRAW);
        let off: int = 0;
        i = 0;
        while (i < attrCount) {
            glVertexAttribPointer(i as u32, layout[i], FLOAT, 0, stride * 4, (off * 4) as i64 as void*);
            glEnableVertexAttribArray(i as u32);
            off = off + layout[i];
            i = i + 1;
        }
        this.count = floatCount / stride;
        glBindVertexArray(0);
    }

    // single attribute 0 (kept from the first version)
    upload(verts: float*, floatCount: int, floatsPerVertex: int) => void {
        let layout: int = floatsPerVertex;
        this.uploadLayout(verts, floatCount, &layout, 1);
    }

    // call after uploadLayout/upload; 32-bit indices
    setIndices(idx: u32*, indexCount: int) => void {
        glBindVertexArray(this.vao);
        glGenBuffers(1, &this.ebo);
        glBindBuffer(ELEMENT_ARRAY_BUFFER, this.ebo);
        glBufferData(ELEMENT_ARRAY_BUFFER, (indexCount * 4) as i64, idx as void*, STATIC_DRAW);
        this.count = indexCount; this.indexed = true; this.indexKind = UNSIGNED_INT;
        glBindVertexArray(0);
    }

    draw() => void {
        glBindVertexArray(this.vao);
        if (this.indexed) { GL.glDrawElements(TRIANGLES, this.count, this.indexKind, nullptr); }
        else { GL.glDrawArrays(TRIANGLES, 0, this.count); }
    }

    free() => void {
        if (this.vbo != 0) glDeleteBuffers(1, &this.vbo);
        if (this.ebo != 0) glDeleteBuffers(1, &this.ebo);
        if (this.vao != 0) glDeleteVertexArrays(1, &this.vao);
        this.vbo = 0; this.ebo = 0; this.vao = 0;
    }
}

// ---- textures ----
struct Texture {
    id: u32 = 0; width: int = 0; height: int = 0;
    fromRGBA(px: u8*, w: int, h: int, mipmaps: bool) => void {
        GL.glGenTextures(1, &this.id);
        GL.glBindTexture(TEXTURE_2D, this.id);
        GL.glTexParameteri(TEXTURE_2D, TEXTURE_WRAP_S, CLAMP_TO_EDGE);
        GL.glTexParameteri(TEXTURE_2D, TEXTURE_WRAP_T, CLAMP_TO_EDGE);
        GL.glTexParameteri(TEXTURE_2D, TEXTURE_MAG_FILTER, LINEAR);
        if (mipmaps) { GL.glTexParameteri(TEXTURE_2D, TEXTURE_MIN_FILTER, LINEAR_MIPMAP_LINEAR); }
        else { GL.glTexParameteri(TEXTURE_2D, TEXTURE_MIN_FILTER, LINEAR); }
        GL.glPixelStorei(0xCF5, 1);                      // UNPACK_ALIGNMENT
        GL.glTexImage2D(TEXTURE_2D, 0, RGBA8, w, h, 0, RGBA, UNSIGNED_BYTE, px as void*);
        if (mipmaps) glGenerateMipmap(TEXTURE_2D);
        this.width = w; this.height = h;
    }
    bind(unit: int) => void { glActiveTexture(TEXTURE0 + (unit as u32)); GL.glBindTexture(TEXTURE_2D, this.id); }
    free() => void { if (this.id != 0) GL.glDeleteTextures(1, &this.id); this.id = 0; }
}

function textureRGBA(px: u8*, w: int, h: int) => u32 {
    let t: Texture;
    t.fromRGBA(px, w, h, false);
    return t.id;
}

// ---- framebuffer (colour texture target) ----
struct Framebuffer {
    fbo: u32 = 0; tex: u32 = 0; width: int = 0; height: int = 0;
    create(w: int, h: int) => bool {
        this.width = w; this.height = h;
        GL.glGenTextures(1, &this.tex);
        GL.glBindTexture(TEXTURE_2D, this.tex);
        GL.glTexParameteri(TEXTURE_2D, TEXTURE_MIN_FILTER, LINEAR);
        GL.glTexParameteri(TEXTURE_2D, TEXTURE_MAG_FILTER, LINEAR);
        GL.glTexParameteri(TEXTURE_2D, TEXTURE_WRAP_S, CLAMP_TO_EDGE);
        GL.glTexParameteri(TEXTURE_2D, TEXTURE_WRAP_T, CLAMP_TO_EDGE);
        GL.glTexImage2D(TEXTURE_2D, 0, RGBA8, w, h, 0, RGBA, UNSIGNED_BYTE, nullptr);
        glGenFramebuffers(1, &this.fbo);
        glBindFramebuffer(FRAMEBUFFER, this.fbo);
        glFramebufferTexture2D(FRAMEBUFFER, COLOR_ATTACHMENT0, TEXTURE_2D, this.tex, 0);
        let status: u32 = glCheckFramebufferStatus(FRAMEBUFFER);
        glBindFramebuffer(FRAMEBUFFER, 0);
        return status == FRAMEBUFFER_COMPLETE;
    }
    bind() => void { glBindFramebuffer(FRAMEBUFFER, this.fbo); GL.glViewport(0, 0, this.width, this.height); }
    unbind(winW: int, winH: int) => void { glBindFramebuffer(FRAMEBUFFER, 0); GL.glViewport(0, 0, winW, winH); }
    free() => void {
        if (this.fbo != 0) glDeleteFramebuffers(1, &this.fbo);
        if (this.tex != 0) GL.glDeleteTextures(1, &this.tex);
        this.fbo = 0; this.tex = 0;
    }
}
