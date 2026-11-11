// standard/1/OpenGL.os
// Tier 5: thin GL helpers: constants, shader/program/buffer/texture wrappers. GL>=2 functions are loaded at runtime.
module GL = import "standard/1/c/opengl.os";
module G = import "standard/1/c/glfw.os";
module Core = import "standard/1/Core.os";

let COLOR_BUFFER_BIT: u32 = 0x4000;
let DEPTH_BUFFER_BIT: u32 = 0x100;
let TRIANGLES: u32 = 4;
let TEXTURE_2D: u32 = 0xDE1;
let RGBA: u32 = 0x1908;
let UNSIGNED_BYTE: u32 = 0x1401;
let FLOAT: u32 = 0x1406;
let ARRAY_BUFFER: u32 = 0x8892;
let ELEMENT_ARRAY_BUFFER: u32 = 0x8893;
let STATIC_DRAW: u32 = 0x88E4;
let VERTEX_SHADER: u32 = 0x8B31;
let FRAGMENT_SHADER: u32 = 0x8B30;
let COMPILE_STATUS: u32 = 0x8B81;
let LINK_STATUS: u32 = 0x8B82;

// Loaded function pointers (call load() after a context is current)
let glCreateShader: fn(u32) => u32 = nullptr;
let glShaderSource: fn(u32, int, char**, int*) => void = nullptr;
let glCompileShader: fn(u32) => void = nullptr;
let glGetShaderiv: fn(u32, u32, int*) => void = nullptr;
let glGetShaderInfoLog: fn(u32, int, int*, char*) => void = nullptr;
let glCreateProgram: fn() => u32 = nullptr;
let glAttachShader: fn(u32, u32) => void = nullptr;
let glLinkProgram: fn(u32) => void = nullptr;
let glGetProgramiv: fn(u32, u32, int*) => void = nullptr;
let glUseProgram: fn(u32) => void = nullptr;
let glDeleteShader: fn(u32) => void = nullptr;
let glDeleteProgram: fn(u32) => void = nullptr;
let glGenBuffers: fn(int, u32*) => void = nullptr;
let glBindBuffer: fn(u32, u32) => void = nullptr;
let glBufferData: fn(u32, i64, void*, u32) => void = nullptr;
let glGenVertexArrays: fn(int, u32*) => void = nullptr;
let glBindVertexArray: fn(u32) => void = nullptr;
let glVertexAttribPointer: fn(u32, int, u32, u8, int, void*) => void = nullptr;
let glEnableVertexAttribArray: fn(u32) => void = nullptr;
let glGetUniformLocation: fn(u32, char*) => int = nullptr;
let glUniform1f: fn(int, float) => void = nullptr;
let glUniform2f: fn(int, float, float) => void = nullptr;
let glUniform1i: fn(int, int) => void = nullptr;
let glUniformMatrix4fv: fn(int, int, u8, float*) => void = nullptr;

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
    glUseProgram = G.glfwGetProcAddress("glUseProgram") as fn(u32) => void;
    glDeleteShader = G.glfwGetProcAddress("glDeleteShader") as fn(u32) => void;
    glDeleteProgram = G.glfwGetProcAddress("glDeleteProgram") as fn(u32) => void;
    glGenBuffers = G.glfwGetProcAddress("glGenBuffers") as fn(int, u32*) => void;
    glBindBuffer = G.glfwGetProcAddress("glBindBuffer") as fn(u32, u32) => void;
    glBufferData = G.glfwGetProcAddress("glBufferData") as fn(u32, i64, void*, u32) => void;
    glGenVertexArrays = G.glfwGetProcAddress("glGenVertexArrays") as fn(int, u32*) => void;
    glBindVertexArray = G.glfwGetProcAddress("glBindVertexArray") as fn(u32) => void;
    glVertexAttribPointer = G.glfwGetProcAddress("glVertexAttribPointer") as fn(u32, int, u32, u8, int, void*) => void;
    glEnableVertexAttribArray = G.glfwGetProcAddress("glEnableVertexAttribArray") as fn(u32) => void;
    glGetUniformLocation = G.glfwGetProcAddress("glGetUniformLocation") as fn(u32, char*) => int;
    glUniform1f = G.glfwGetProcAddress("glUniform1f") as fn(int, float) => void;
    glUniform2f = G.glfwGetProcAddress("glUniform2f") as fn(int, float, float) => void;
    glUniform1i = G.glfwGetProcAddress("glUniform1i") as fn(int, int) => void;
    glUniformMatrix4fv = G.glfwGetProcAddress("glUniformMatrix4fv") as fn(int, int, u8, float*) => void;
}

function clear(r: float, g: float, b: float, a: float) => void {
    GL.glClearColor(r, g, b, a);
    GL.glClear(COLOR_BUFFER_BIT | DEPTH_BUFFER_BIT);
}

function compile(kind: u32, src: char*) => u32 {
    let sh: u32 = glCreateShader(kind);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    let ok: int = 0;
    glGetShaderiv(sh, COMPILE_STATUS, &ok);
    if (ok == 0) { glDeleteShader(sh); return 0; }
    return sh;
}

// Returns program id, 0 on failure.
function program(vs: char*, fs: char*) => u32 {
    let v: u32 = compile(VERTEX_SHADER, vs);
    let f: u32 = compile(FRAGMENT_SHADER, fs);
    if (v == 0 || f == 0) return 0;
    let p: u32 = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f);
    let ok: int = 0;
    glGetProgramiv(p, LINK_STATUS, &ok);
    if (ok == 0) { glDeleteProgram(p); return 0; }
    return p;
}

// Static vertex buffer with a VAO. attribs: floats per vertex for attribute 0 (extend as needed).
struct Mesh {
    vao: u32 = 0; vbo: u32 = 0; count: int = 0;
    upload(verts: float*, floatCount: int, floatsPerVertex: int) => void {
        glGenVertexArrays(1, &this.vao);
        glGenBuffers(1, &this.vbo);
        glBindVertexArray(this.vao);
        glBindBuffer(ARRAY_BUFFER, this.vbo);
        glBufferData(ARRAY_BUFFER, (floatCount * 4) as i64, verts as void*, STATIC_DRAW);
        glVertexAttribPointer(0, floatsPerVertex, FLOAT, 0, floatsPerVertex * 4, nullptr);
        glEnableVertexAttribArray(0);
        this.count = floatCount / floatsPerVertex;
    }
    draw() => void { glBindVertexArray(this.vao); GL.glDrawArrays(TRIANGLES, 0, this.count); }
}

function textureRGBA(px: u8*, w: int, h: int) => u32 {
    let id: u32 = 0;
    GL.glGenTextures(1, &id);
    GL.glBindTexture(TEXTURE_2D, id);
    GL.glTexParameteri(TEXTURE_2D, 0x2801, 0x2601);   // MIN_FILTER linear
    GL.glTexParameteri(TEXTURE_2D, 0x2800, 0x2601);   // MAG_FILTER linear
    GL.glTexImage2D(TEXTURE_2D, 0, 0x1908, w, h, 0, RGBA, UNSIGNED_BYTE, px as void*);
    return id;
}
