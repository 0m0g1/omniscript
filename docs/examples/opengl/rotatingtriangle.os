extern "C" {
    fn printf(...fmt: char*) => int;
    fn sinf(x: float) => float;
    fn fabs(x: float) => float;
}

extern "C:/Windows/System32/kernel32.dll" {
    fn CreateThread(security: void*, stackSize: int, start: void*, param: void*, flags: uint, threadId: int*) => void*;
    fn Sleep(ms: uint) => void;
    fn ExitThread(code: uint) => void;
}

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
}

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
    fn glRotatef(angle: float, x: float, y: float, z: float) => void;
}

// OpenGL constants
const GL_COLOR_BUFFER_BIT = 0x00004000;
const GL_PROJECTION = 0x1701;
const GL_MODELVIEW = 0x1700;
const GL_TRIANGLES = 0x0004;

// Render loop function
function renderLoop(ptr: void*) => uint {
    let window = ptr;

    glfwMakeContextCurrent(window);

    glViewport(0, 0, 800, 600);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    let angle: float = 0.0;
    let t: float = 0.0;

    while (glfwWindowShouldClose(window) == 0) {
        t += 0.1;

        glClearColor(0.1, 0.1, 0.1, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glLoadIdentity();

        glRotatef(angle, 0.0, 0.0, 1.0);
        angle += 5.0;

        glBegin(GL_TRIANGLES);
        glColor3f(fabs(sinf(t)), 0.0, 0.0); glVertex2f(0.0, 0.5);
        glColor3f(0.0, fabs(sinf(t + 2.1)), 0.0); glVertex2f(-0.5, -0.5);
        glColor3f(0.0, 0.0, fabs(sinf(t + 4.2))); glVertex2f(0.5, -0.5);
        glEnd();

        glfwSwapBuffers(window);
        Sleep(16);
    }

    ExitThread(0);
    return 0;
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

// Cast renderLoop to void*
let threadId: int = 0;
let renderLoopFunc = renderLoop as void*;
CreateThread(nullptr, 0, renderLoopFunc, window, 0, &threadId);

// Main loop handles events
while (glfwWindowShouldClose(window) == 0) {
    glfwPollEvents();
    Sleep(1); // Small wait to yield CPU
}

glfwTerminate();
