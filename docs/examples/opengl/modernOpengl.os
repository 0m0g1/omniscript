extern "C" {
    fn printf(...fmt: char*) => int;
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
}

// Legacy OpenGL
extern 
"C:/Windows/System32/opengl32.dll",
"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64/OpenGL32.lib" {
    fn glClearColor(r: float, g: float, b: float, a: float) => void;
    fn glClear(mask: uint) => void;
    fn glBegin(mode: uint) => void;
    fn glEnd() => void;
    fn glVertex2f(x: float, y: float) => void;
    fn glColor3f(r: float, g: float, b: float) => void;
    fn glLoadIdentity() => void;
    fn glViewport(x: int, y: int, width: int, height: int) => void;
    fn glMatrixMode(mode: uint) => void;
    fn glOrtho(left: double, right: double, bottom: double, top: double, near: double, far: double) => void;
}

// Constants
const GL_COLOR_BUFFER_BIT = 0x00004000;
const GL_PROJECTION = 0x1701;
const GL_MODELVIEW = 0x1700;
const GL_TRIANGLES = 0x0004;
const GL_ARRAY_BUFFER = 0x8892;
const GL_STATIC_DRAW = 0x88E4;

// Function pointer typedefs
type GLGENBUFFERS = fn(n: int, buffers: uint*) => void;
type GLBINDBUFFER = fn(target: uint, buffer: uint) => void;
type GLBUFFERDATA = fn(target: uint, size: int, data: void*, usage: uint) => void;

// Load glad.dll
extern "dependencies/glad/glad_x86-64/glad.dll" {
    // Dummy symbol to force DLL load, real functions are loaded dynamically.
    fn dummy() => void;
}

// Entry
if (glfwInit() == 0) {
    printf("Failed to initialize GLFW\n");
    return;
}

let window = glfwCreateWindow(800, 600, "OS Hello Triangle", nullptr, nullptr);
if (window == nullptr) {
    printf("Failed to create window\n");
    glfwTerminate();
    return;
}
glfwMakeContextCurrent(window);

// Load modern OpenGL functions
let glGenBuffers = glfwGetProcAddress("glGenBuffers") as GLGENBUFFERS;
let glBindBuffer = glfwGetProcAddress("glBindBuffer") as GLBINDBUFFER;
let glBufferData = glfwGetProcAddress("glBufferData") as GLBUFFERDATA;

if (glGenBuffers == nullptr or glBindBuffer == nullptr or glBufferData == nullptr) {
    printf("Failed to load modern OpenGL functions\n");
    glfwTerminate();
    return;
}

// Init OpenGL state
glViewport(0, 0, 800, 600);
glMatrixMode(GL_PROJECTION);
glLoadIdentity();
glOrtho(-1, 1, -1, 1, -1, 1);
glMatrixMode(GL_MODELVIEW);

// Create a VBO (just as an example)
var vbo: uint = 0;
glGenBuffers(1, &vbo);
glBindBuffer(GL_ARRAY_BUFFER, vbo);
let vertices: float[] = [0.0, 0.5,  -0.5, -0.5,  0.5, -0.5];
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);

// Main loop
while (glfwWindowShouldClose(window) == 0) {
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    // Draw classic triangle (legacy path)
    glBegin(GL_TRIANGLES);
    glColor3f(1.0, 0.0, 0.0); glVertex2f(0.0, 0.5);
    glColor3f(0.0, 1.0, 0.0); glVertex2f(-0.5, -0.5);
    glColor3f(0.0, 0.0, 1.0); glVertex2f(0.5, -0.5);
    glEnd();

    glfwSwapBuffers(window);
    glfwPollEvents();
}

glfwTerminate();
