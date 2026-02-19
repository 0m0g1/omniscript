extern "C" fn printf(fmt: char*, ...) => int;

// GLFW
extern 
"dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dll", 
"dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/lib glfw3.a" {
    fn glfwInit() => int;
    fn glfwCreateWindow(width: int, height: int, title: char*, monitor: void*, share: void*) => void*;
    fn glfwMakeContextCurrent(window: void*) => void;
    fn glfwWindowShouldClose(window: void*) => int;
    fn glfwPollEvents() => void;
    fn glfwSwapBuffers(window: void*) => void;
    fn glfwTerminate() => void;
    fn glfwGetProcAddress(name: char*) => void*;
    fn glfwWindowHint(hint: int, value: int) => void;
    fn glfwSetErrorCallback(callback: void*) => void*;
}

// OpenGL Constants
const GL_COLOR_BUFFER_BIT: uint = 0x00004000;
const GL_ARRAY_BUFFER: uint = 0x8892;
const GL_STATIC_DRAW: uint = 0x88E4;
const GL_TRIANGLES: uint = 0x0004;
const GL_VERTEX_SHADER: uint = 0x8B31;
const GL_FRAGMENT_SHADER: uint = 0x8B30;
const GL_COMPILE_STATUS: uint = 0x8B81;
const GL_LINK_STATUS: uint = 0x8B82;
const GL_INFO_LOG_LENGTH: uint = 0x8B84;
const GL_FLOAT: uint = 0x1406;
const GL_FALSE: uint = 0;
const GL_TRUE: int = 1;

// GLFW constants
const GLFW_CONTEXT_VERSION_MAJOR: uint = 0x00022002;
const GLFW_CONTEXT_VERSION_MINOR: uint = 0x00022003;
const GLFW_OPENGL_PROFILE: uint = 0x00022008;
const GLFW_OPENGL_CORE_PROFILE: uint = 0x00032001;

// OpenGL function types
type GLGETERROR = fn() => uint;
type GLCLEAR = fn(mask: uint) => void;
type GLCLEARCOLOR = fn(r: float, g: float, b: float, a: float) => void;
type GLVIEWPORT = fn(x: int, y: int, width: int, height: int) => void;
type GLGENBUFFERS = fn(n: int, buffers: uint*) => void;
type GLBINDBUFFER = fn(target: uint, buffer: uint) => void;
type GLBUFFERDATA = fn(target: uint, size: int, data: void*, usage: uint) => void;
type GLGENVERTEXARRAYS = fn(n: int, arrays: uint*) => void;
type GLBINDVERTEXARRAY = fn(array: uint) => void;
type GLVERTEXATTRIBPOINTER = fn(index: uint, size: int, type: uint, normalized: uint, stride: int, pointer: void*) => void;
type GLENABLEVERTEXATTRIBARRAY = fn(index: uint) => void;
type GLCREATESHADER = fn(type: uint) => uint;
type GLSHADERSOURCE = fn(shader: uint, count: int, string: char**, length: int*) => void;
type GLCOMPILESHADER = fn(shader: uint) => void;
type GLGETSHADERIV = fn(shader: uint, pname: uint, params: int*) => void;
type GLGETSHADERINFOLOG = fn(shader: uint, bufSize: int, length: int*, infoLog: char*) => void;
type GLDELETESHADER = fn(shader: uint) => void;
type GLCREATEPROGRAM = fn() => uint;
type GLATTACHSHADER = fn(program: uint, shader: uint) => void;
type GLLINKPROGRAM = fn(program: uint) => void;
type GLGETPROGRAMIV = fn(program: uint, pname: uint, params: int*) => void;
type GLGETPROGRAMINFOLOG = fn(program: uint, bufSize: int, length: int*, infoLog: char*) => void;
type GLUSEPROGRAM = fn(program: uint) => void;
type GLDELETEPROGRAM = fn(program: uint) => void;
type GLDRAWARRAYS = fn(mode: uint, first: int, count: int) => void;
type GLDELETEVERTEXARRAYS = fn(n: int, arrays: uint*) => void;
type GLDELETEBUFFERS = fn(n: int, buffers: uint*) => void;


// Global OpenGL function pointers
let glGetError: GLGETERROR;
let glClear: GLCLEAR;
let glClearColor: GLCLEARCOLOR;
let glViewport: GLVIEWPORT;
let glGenBuffers: GLGENBUFFERS;
let glBindBuffer: GLBINDBUFFER;
let glBufferData: GLBUFFERDATA;
let glGenVertexArrays: GLGENVERTEXARRAYS;
let glBindVertexArray: GLBINDVERTEXARRAY;
let glVertexAttribPointer: GLVERTEXATTRIBPOINTER;
let glEnableVertexAttribArray: GLENABLEVERTEXATTRIBARRAY;
let glCreateShader: GLCREATESHADER;
let glShaderSource: GLSHADERSOURCE;
let glCompileShader: GLCOMPILESHADER;
let glGetShaderiv: GLGETSHADERIV;
let glGetShaderInfoLog: GLGETSHADERINFOLOG;
let glDeleteShader: GLDELETESHADER;
let glCreateProgram: GLCREATEPROGRAM;
let glAttachShader: GLATTACHSHADER;
let glLinkProgram: GLLINKPROGRAM;
let glGetProgramiv: GLGETPROGRAMIV;
let glGetProgramInfoLog: GLGETPROGRAMINFOLOG;
let glUseProgram: GLUSEPROGRAM;
let glDeleteProgram: GLDELETEPROGRAM;
let glDrawArrays: GLDRAWARRAYS;
let glDeleteVertexArrays: GLDELETEVERTEXARRAYS;
let glDeleteBuffers: GLDELETEBUFFERS;

// Shader sources
const vertexShaderSource = r"#version 330 core
    layout(location = 0) in vec3 aPos;
    void main() {
        gl_Position = vec4(aPos, 1.0);
    }";

const fragmentShaderSource = r"#version 330 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(1.0, 0.5, 0.2, 1.0);
    }";

// Error callback for GLFW
fn glfwErrorCallback(error: int, description: char*) => void {
    printf("GLFW Error %d: %s\n", error, description);
}

function loadGLFunction(name: char*, required: int) => void* {
    let func = glfwGetProcAddress(name);
    if (func == nullptr && required) {
        printf("Failed to load required OpenGL function: %s\n", name);
        glfwTerminate();
        return nullptr;
    }
    return func;
}

function loadOpenGLFunctions() => void {
    glGetError = loadGLFunction("glGetError", 1) as GLGETERROR;
    glClear = loadGLFunction("glClear", 1) as GLCLEAR;
    glClearColor = loadGLFunction("glClearColor", 1) as GLCLEARCOLOR;
    glViewport = loadGLFunction("glViewport", 1) as GLVIEWPORT;
    glGenBuffers = loadGLFunction("glGenBuffers", 1) as GLGENBUFFERS;
    glBindBuffer = loadGLFunction("glBindBuffer", 1) as GLBINDBUFFER;
    glBufferData = loadGLFunction("glBufferData", 1) as GLBUFFERDATA;
    glGenVertexArrays = loadGLFunction("glGenVertexArrays", 1) as GLGENVERTEXARRAYS;
    glBindVertexArray = loadGLFunction("glBindVertexArray", 1) as GLBINDVERTEXARRAY;
    glVertexAttribPointer = loadGLFunction("glVertexAttribPointer", 1) as GLVERTEXATTRIBPOINTER;
    glEnableVertexAttribArray = loadGLFunction("glEnableVertexAttribArray", 1) as GLENABLEVERTEXATTRIBARRAY;
    glCreateShader = loadGLFunction("glCreateShader", 1) as GLCREATESHADER;
    glShaderSource = loadGLFunction("glShaderSource", 1) as GLSHADERSOURCE;
    glCompileShader = loadGLFunction("glCompileShader", 1) as GLCOMPILESHADER;
    glGetShaderiv = loadGLFunction("glGetShaderiv", 1) as GLGETSHADERIV;
    glGetShaderInfoLog = loadGLFunction("glGetShaderInfoLog", 1) as GLGETSHADERINFOLOG;
    glDeleteShader = loadGLFunction("glDeleteShader", 1) as GLDELETESHADER;
    glCreateProgram = loadGLFunction("glCreateProgram", 1) as GLCREATEPROGRAM;
    glAttachShader = loadGLFunction("glAttachShader", 1) as GLATTACHSHADER;
    glLinkProgram = loadGLFunction("glLinkProgram", 1) as GLLINKPROGRAM;
    glGetProgramiv = loadGLFunction("glGetProgramiv", 1) as GLGETPROGRAMIV;
    glGetProgramInfoLog = loadGLFunction("glGetProgramInfoLog", 1) as GLGETPROGRAMINFOLOG;
    glUseProgram = loadGLFunction("glUseProgram", 1) as GLUSEPROGRAM;
    glDeleteProgram = loadGLFunction("glDeleteProgram", 1) as GLDELETEPROGRAM;
    glDrawArrays = loadGLFunction("glDrawArrays", 1) as GLDRAWARRAYS;
    glDeleteVertexArrays = loadGLFunction("glDeleteVertexArrays", 1) as GLDELETEVERTEXARRAYS;
    glDeleteBuffers = loadGLFunction("glDeleteBuffers", 1) as GLDELETEBUFFERS;
}

function compileShader(source: char*, type: uint) => uint {
    let shader = glCreateShader(type);
    if (shader == 0) {
        printf("Failed to create \n");
        return 0;
    }

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    let success: int;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        let infoLog: char*;
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        printf("Shader compilation error:\n%s\n", infoLog);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

function createShaderProgram() => uint {
    printf("creating the shader program\n");
    let vertexShader = compileShader(&vertexShaderSource, GL_VERTEX_SHADER);
    printf("compiled the shader\n");
    if (vertexShader == 0) return 0;

    let fragmentShader = compileShader(&fragmentShaderSource, GL_FRAGMENT_SHADER);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return 0;
    }

    let program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    let success: int;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        let infoLog: char*;
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        printf("Shader linking error:\n%s\n", infoLog);
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

function main() => void {
    // Initialize GLFW
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    let window = glfwCreateWindow(800, 600, "OpenGL Triangle", nullptr, nullptr);
    if (window == nullptr) {
        printf("Failed to create GLFW windown\n");
        glfwTerminate();
        return;
    }
    printf("Successfully created the window\n");
    
    glfwMakeContextCurrent(window);
    printf("Successfully made the context the current window\n");
    
    // Load OpenGL functions
    loadOpenGLFunctions();
    printf("Successfully loaded the opengl functions\n");

    // Create shader program
    let shaderProgram = createShaderProgram();
    printf("Checking if the shader program is valid\n");
    if (shaderProgram == 0) {
        glfwTerminate();
        return;
    }
    printf("Successfully created the shader program\n");

    // Set up vertex data
    let vertices: float[9] = [
        -0.5, -0.5, 0.0,  // left
         0.5, -0.5, 0.0,  // right
         0.0,  0.5, 0.0   // top
    ];

    let VAO: uint, VBO: uint;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 9 * 4 /*sizeof(vertices)*/, &vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * 4/*sizeof(float)*/, nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2, 0.3, 0.3, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
}

main();
printf("done");