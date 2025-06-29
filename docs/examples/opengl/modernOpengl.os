extern "C" {
    fn printf(...fmt: char*) => int;
    fn malloc(size: int) => void*;
    fn free(ptr: void*) => void;
    fn strlen(str: char*) => int;
}

// GLFW
extern 
"dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dll", 
"dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/libglfw3.a" {
    fn glfwInit() => int;
    fn glfwCreateWindow(width: int, height: int, title: char*, monitor: void*, share: void*) => void*;
    fn glfwMakeContextCurrent(window: void*) => void;
    fn glfwWindowShouldClose(window: void*) => int;
    fn glfwPollEvents() => void;
    fn glfwSwapBuffers(window: void*) => void;
    fn glfwTerminate() => void;
    fn glfwGetProcAddress(name: char*) => void*;
    fn glfwWindowHint(hint: int, value: int) => void;
}

// OpenGL Constants
const GL_COLOR_BUFFER_BIT = 0x00004000;
const GL_ARRAY_BUFFER = 0x8892;
const GL_STATIC_DRAW = 0x88E4;
const GL_TRIANGLES = 0x0004;
const GL_VERTEX_SHADER = 0x8B31;
const GL_FRAGMENT_SHADER = 0x8B30;
const GL_COMPILE_STATUS = 0x8B81;
const GL_LINK_STATUS = 0x8B82;
const GL_INFO_LOG_LENGTH = 0x8B84;
const GL_FLOAT = 0x1406;
const GL_FALSE = 0;
const GL_TRUE = 1;

// GLFW constants
const GLFW_CONTEXT_VERSION_MAJOR = 0x00022002;
const GLFW_CONTEXT_VERSION_MINOR = 0x00022003;
const GLFW_OPENGL_PROFILE = 0x00022008;
const GLFW_OPENGL_CORE_PROFILE = 0x00032001;

// Modern OpenGL function pointers
type GLCLEAR = fn(mask: uint) => void;
type GLCLEARCOLOR = fn(r: float, g: float, b: float, a: float) => void;
type GLVIEWPORT = fn(x: int, y: int, width: int, height: int) => void;

// Buffer functions
type GLGENBUFFERS = fn(n: int, buffers: uint*) => void;
type GLBINDBUFFER = fn(target: uint, buffer: uint) => void;
type GLBUFFERDATA = fn(target: uint, size: int, data: void*, usage: uint) => void;

// Vertex Array functions
type GLGENVERTEXARRAYS = fn(n: int, arrays: uint*) => void;
type GLBINDVERTEXARRAY = fn(array: uint) => void;
//todo:: fix type aliasing in function pointer types
// type uchar = uint8;
// type GLVERTEXATTRIBPOINTER = fn(index: uint, size: int, type: uint, normalized: uchar, stride: int, pointer: void*) => void;
type GLVERTEXATTRIBPOINTER = fn(index: uint, size: int, type: uint, normalized: uint8, stride: int, pointer: void*) => void;
type GLENABLEVERTEXATTRIBARRAY = fn(index: uint) => void;

// Shader functions
type GLCREATESHADER = fn(type: uint) => uint;
type GLSHADERSOURCE = fn(shader: uint, count: int, string: char**, length: int*) => void;
type GLCOMPILESHADER = fn(shader: uint) => void;
type GLGETSHADERIV = fn(shader: uint, pname: uint, params: int*) => void;
type GLGETSHADERINFOLOG = fn(shader: uint, bufSize: int, length: int*, infoLog: char*) => void;
type GLDELETESHADER = fn(shader: uint) => void;

// Program functions
type GLCREATEPROGRAM = fn() => uint;
type GLATTACHSHADER = fn(program: uint, shader: uint) => void;
type GLLINKPROGRAM = fn(program: uint) => void;
type GLGETPROGRAMIV = fn(program: uint, pname: uint, params: int*) => void;
type GLGETPROGRAMINFOLOG = fn(program: uint, bufSize: int, length: int*, infoLog: char*) => void;
type GLUSEPROGRAM = fn(program: uint) => void;
type GLDELETEPROGRAM = fn(program: uint) => void;

// Draw functions
type GLDRAWARRAYS = fn(mode: uint, first: int, count: int) => void;

// Function pointers
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

// Shader sources
const vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aColor;\n"
    "out vec3 vertexColor;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos, 1.0);\n"
    "   vertexColor = aColor;\n"
    "}\0";

const fragmentShaderSource = "#version 330 core\n"
    "in vec3 vertexColor;\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(vertexColor, 1.0f);\n"
    "}\n\0";


function loadOpenGLFunctions() => int {
    glClear = glfwGetProcAddress("glClear") as GLCLEAR;
    glClearColor = glfwGetProcAddress("glClearColor") as GLCLEARCOLOR;
    glViewport = glfwGetProcAddress("glViewport") as GLVIEWPORT;
    glGenBuffers = glfwGetProcAddress("glGenBuffers") as GLGENBUFFERS;
    glBindBuffer = glfwGetProcAddress("glBindBuffer") as GLBINDBUFFER;
    glBufferData = glfwGetProcAddress("glBufferData") as GLBUFFERDATA;
    glGenVertexArrays = glfwGetProcAddress("glGenVertexArrays") as GLGENVERTEXARRAYS;
    glBindVertexArray = glfwGetProcAddress("glBindVertexArray") as GLBINDVERTEXARRAY;
    glVertexAttribPointer = glfwGetProcAddress("glVertexAttribPointer") as GLVERTEXATTRIBPOINTER;
    glEnableVertexAttribArray = glfwGetProcAddress("glEnableVertexAttribArray") as GLENABLEVERTEXATTRIBARRAY;
    glCreateShader = glfwGetProcAddress("glCreateShader") as GLCREATESHADER;
    glShaderSource = glfwGetProcAddress("glShaderSource") as GLSHADERSOURCE;
    glCompileShader = glfwGetProcAddress("glCompileShader") as GLCOMPILESHADER;
    glGetShaderiv = glfwGetProcAddress("glGetShaderiv") as GLGETSHADERIV;
    glGetShaderInfoLog = glfwGetProcAddress("glGetShaderInfoLog") as GLGETSHADERINFOLOG;
    glDeleteShader = glfwGetProcAddress("glDeleteShader") as GLDELETESHADER;
    glCreateProgram = glfwGetProcAddress("glCreateProgram") as GLCREATEPROGRAM;
    glAttachShader = glfwGetProcAddress("glAttachShader") as GLATTACHSHADER;
    glLinkProgram = glfwGetProcAddress("glLinkProgram") as GLLINKPROGRAM;
    glGetProgramiv = glfwGetProcAddress("glGetProgramiv") as GLGETPROGRAMIV;
    glGetProgramInfoLog = glfwGetProcAddress("glGetProgramInfoLog") as GLGETPROGRAMINFOLOG;
    glUseProgram = glfwGetProcAddress("glUseProgram") as GLUSEPROGRAM;
    glDeleteProgram = glfwGetProcAddress("glDeleteProgram") as GLDELETEPROGRAM;
    glDrawArrays = glfwGetProcAddress("glDrawArrays") as GLDRAWARRAYS;

    // Check if all functions were loaded
    if (
        // glClear == nullptr || glClearColor == nullptr || glViewport == nullptr ||
        // glGenBuffers == nullptr || glBindBuffer == nullptr || glBufferData == nullptr ||
        // glGenVertexArrays == nullptr || glBindVertexArray == nullptr ||
        // glVertexAttribPointer == nullptr || glEnableVertexAttribArray == nullptr ||
        // glCreateShader == nullptr || glShaderSource == nullptr || glCompileShader == nullptr ||
        // glGetShaderiv == nullptr || glGetShaderInfoLog == nullptr || glDeleteShader == nullptr ||
        // glCreateProgram == nullptr || glAttachShader == nullptr || glLinkProgram == nullptr ||
        // glGetProgramiv == nullptr || glGetProgramInfoLog == nullptr || glUseProgram == nullptr ||
        // glDeleteProgram == nullptr || glDrawArrays == nullptr
    ) {
        return 0;
    }
    return 1;
}

function compileShader(source: char*, type: uint) => uint {
    let shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    let success: int;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        let infoLogLength: int;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        let infoLog = malloc(infoLogLength) as char*;
        glGetShaderInfoLog(shader, infoLogLength, nullptr, infoLog);
        printf("Shader compilation failed: %s\n", infoLog);
        free(infoLog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

function createShaderProgram() => uint {
    let vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    let fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);

    if (vertexShader == 0 || fragmentShader == 0) {
        return 0;
    }

    let program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    let success: int;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        let infoLogLength: int;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        let infoLog = malloc(infoLogLength) as char*;
        glGetProgramInfoLog(program, infoLogLength, nullptr, infoLog);
        printf("Program linking failed: %s\n", infoLog);
        free(infoLog);
        glDeleteProgram(program);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

// Entry point
if (glfwInit() == 0) {
    printf("Failed to initialize GLFW\n");
    return;
}

// Request OpenGL 3.3 Core Profile
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

let window = glfwCreateWindow(800, 600, "Modern OpenGL Triangle", nullptr, nullptr);
if (window == nullptr) {
    printf("Failed to create window\n");
    glfwTerminate();
    return;
}

glfwMakeContextCurrent(window);

// Load OpenGL functions
if (loadOpenGLFunctions() == 0) {
    printf("Failed to load OpenGL functions\n");
    glfwTerminate();
    return;
}

// Create shader program
let shaderProgram = createShaderProgram();
if (shaderProgram == 0) {
    printf("Failed to create shader program\n");
    glfwTerminate();
    return;
}

// Triangle vertices with position and color
let vertices: float[15] = [
    // Position      // Color
     0.0,  0.5, 0.0,  1.0, 0.0, 0.0,  // Top vertex - Red
    -0.5, -0.5, 0.0,  0.0, 1.0, 0.0,  // Bottom-left vertex - Green
     0.5, -0.5, 0.0,  0.0, 0.0, 1.0   // Bottom-right vertex - Blue
];

// Create VAO and VBO
let VAO: uint, VBO: uint;
glGenVertexArrays(1, &VAO);
glGenBuffers(1, &VBO);

// Bind VAO first
glBindVertexArray(VAO);

// Bind and set up VBO
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);

// Position attribute (location = 0)
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0 as void*);
glEnableVertexAttribArray(0);

// Color attribute (location = 1)
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (3 * sizeof(float)) as void*);
glEnableVertexAttribArray(1);

// Unbind VAO
glBindVertexArray(0);

// Set viewport
glViewport(0, 0, 800, 600);

// Main render loop
while (glfwWindowShouldClose(window) == 0) {
    // Clear screen
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Use shader program
    glUseProgram(shaderProgram);
    
    // Bind VAO and draw triangle
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
}

// Cleanup
glDeleteProgram(shaderProgram);
glfwTerminate();