extern "C" {
    fn printf(...fmt: char*) => int;
}

extern "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dll" {
    fn glfwInit() => int;
    fn glfwCreateWindow(width: int, height: int, title: char*, monitor: void*, share: void*) => void*;
    fn glfwMakeContextCurrent(window: void*) => void;
    fn glfwWindowShouldClose(window: void*) => int;
    fn glfwPollEvents() => void;
    fn glfwSwapBuffers(window: void*) => void;
    fn glfwTerminate() => void;
    fn glfwGetTime() => double;
}

extern "opengl32.dll" {
    fn glClearColor(r: float, g: float, b: float, a: float) => void;
    fn glClear(mask: uint) => void;
    fn glBegin(mode: uint) => void;
    fn glEnd() => void;
    fn glVertex3f(x: float, y: float, z: float) => void;
    fn glColor3f(r: float, g: float, b: float) => void;
    fn glLoadIdentity() => void;
    fn glViewport(x: int, y: int, width: int, height: int) => void;
    fn glMatrixMode(mode: uint) => void;
    fn glOrtho(left: double, right: double, bottom: double, top: double, near: double, far: double) => void;
    fn glRotatef(angle: float, x: float, y: float, z: float) => void;
    fn glTranslatef(x: float, y: float, z: float) => void;
    fn glEnable(cap: uint) => void;
    fn glDepthFunc(func: uint) => void;
    fn glClearDepth(depth: double) => void;
}

const GL_COLOR_BUFFER_BIT = 0x00004000;
const GL_DEPTH_BUFFER_BIT = 0x00000100;
const GL_PROJECTION = 0x1701;
const GL_MODELVIEW = 0x1700;
const GL_TRIANGLES = 0x0004;
const GL_DEPTH_TEST = 0x0B71;
const GL_LESS = 0x0201;

if (glfwInit() == 0) {
    printf("Failed to initialize GLFW\n");
    return;
}

let window = glfwCreateWindow(800, 600, "3D Gradient Pyramid", nullptr, nullptr);
if (window == nullptr) {
    printf("Failed to create window\n");
    glfwTerminate();
    return;
}

glfwMakeContextCurrent(window);

// Setup viewport and perspective
glViewport(0, 0, 800, 600);
glMatrixMode(GL_PROJECTION);
glLoadIdentity();
glOrtho(-2, 2, -2, 2, -5, 5);
glMatrixMode(GL_MODELVIEW);

// Enable depth buffer
glEnable(GL_DEPTH_TEST);
glClearDepth(1.0);
glDepthFunc(GL_LESS);

while (glfwWindowShouldClose(window) == 0) {
    let time = glfwGetTime();
    let angle = (time * 45.0) as float;

    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glTranslatef(0.0, 0.0, -2.0);
    glRotatef(angle, 0.3, 1.0, 0.0);

    glBegin(GL_TRIANGLES);

    // Front face
    glColor3f(1, 0, 0); glVertex3f( 0.0,  1.0,  0.0);
    glColor3f(0, 1, 0); glVertex3f(-1.0, -1.0,  1.0);
    glColor3f(0, 0, 1); glVertex3f( 1.0, -1.0,  1.0);

    // Right face
    glColor3f(1, 0, 0); glVertex3f( 0.0,  1.0,  0.0);
    glColor3f(0, 0, 1); glVertex3f( 1.0, -1.0,  1.0);
    glColor3f(0, 1, 0); glVertex3f( 1.0, -1.0, -1.0);

    // Back face
    glColor3f(1, 0, 0); glVertex3f( 0.0,  1.0,  0.0);
    glColor3f(0, 1, 0); glVertex3f( 1.0, -1.0, -1.0);
    glColor3f(0, 0, 1); glVertex3f(-1.0, -1.0, -1.0);

    // Left face
    glColor3f(1, 0, 0); glVertex3f( 0.0,  1.0,  0.0);
    glColor3f(0, 0, 1); glVertex3f(-1.0, -1.0, -1.0);
    glColor3f(0, 1, 0); glVertex3f(-1.0, -1.0,  1.0);

    glEnd();

    glfwSwapBuffers(window);
    glfwPollEvents();
}

glfwTerminate();
