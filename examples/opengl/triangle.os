extern "C" {
    fn printf(...fmt: char*) => int;
}

extern "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dll",
        "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/libglfw3.a" {
        fn glfwInit() => int;
        fn glfwCreateWindow(width: int, height: int, title: char*, monitor: void*, share: void*) => void*;
        fn glfwMakeContextCurrent(window: void*) => void;
        fn glfwWindowShouldClose(window: void*) => int;
        fn glfwPollEvents() => void;
        fn glfwSwapBuffers(window: void*) => void;
        fn glfwTerminate() => void;
}

extern "C:/Windows/System32/opengl32.dll" {
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

const GL_COLOR_BUFFER_BIT = 0x00004000;
const GL_PROJECTION = 0x1701;
const GL_MODELVIEW = 0x1700;
const GL_TRIANGLES = 0x0004;

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

glViewport(0, 0, 800, 600);
glMatrixMode(GL_PROJECTION);
glLoadIdentity();
glOrtho(-1, 1, -1, 1, -1, 1);
glMatrixMode(GL_MODELVIEW);

while (glfwWindowShouldClose(window) == 0) {
    glClearColor(0.1, 0.1, 0.1, 0.1);
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    glBegin(GL_TRIANGLES);

    // Top vertex - Red
    glColor3f(1.0, 0.0, 0.0);
    glVertex2f(0.0, 0.5);

    // Bottom-left vertex - Green
    glColor3f(0.0, 1.0, 0.0);
    glVertex2f(-0.5, -0.5);

    // Bottom-right vertex - Blue
    glColor3f(0.0, 0.0, 1.0);
    glVertex2f(0.5, -0.5);

    glEnd();

    glfwSwapBuffers(window);
    glfwPollEvents();
}

glfwTerminate();