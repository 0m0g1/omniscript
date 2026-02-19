extern "C" {
    fn printf(fmt: char*, ...) => int;
}

extern 
"dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dll", 
"dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/libglfw3.a"
{
    fn glfwInit() => int;
    fn glfwCreateWindow(w: int, h: int, title: char*, monitor: void*, share: void*) => void*;
    fn glfwMakeContextCurrent(win: void*) => void;
    fn glfwWindowShouldClose(win: void*) => int;
    fn glfwPollEvents() => void;
    fn glfwSwapBuffers(win: void*) => void;
    fn glfwTerminate() => void;
    fn glfwGetCursorPos(win: void*, x_out: double*, y_out: double*) => void;
    fn glfwGetMouseButton(win: void*, button: int) => int;
}

extern "C:/Windows/System32/opengl32.dll" {
    fn glClearColor(r: float, g: float, b: float, a: float) => void;
    fn glClear(mask: uint) => void;
    fn glBegin(mode: uint) => void;
    fn glEnd() => void;
    fn glVertex2f(x: float, y: float) => void;
    fn glColor3f(r: float, g: float, b: float) => void;
    fn glLoadIdentity() => void;
    fn glViewport(x: int, y: int, w: int, h: int) => void;
    fn glMatrixMode(mode: uint) => void;
    fn glOrtho(left: double, right: double, bottom: double, top: double, near: double, far: double) => void;
}

const GL_COLOR_BUFFER_BIT = 0x00004000;
const GL_PROJECTION = 0x1701;
const GL_MODELVIEW = 0x1700;
const GL_QUADS = 0x0007;

const BUTTON_LEFT = -0.3f;
const BUTTON_RIGHT = 0.3f;
const BUTTON_TOP = 0.2f;
const BUTTON_BOTTOM = -0.2f;

if (glfwInit() == 0) {
    printf("GLFW failed\n");
    return;
}

let window = glfwCreateWindow(800, 600, "3D Button", nullptr, nullptr);
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
    glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    // Mouse input
    let mx: double = 0.0;
    let my: double = 0.0;
    glfwGetCursorPos(window, &mx, &my);
    let norm_x = ((mx / 800.0) * 2.0 - 1.0) as float;
    let norm_y = (-((my / 600.0) * 2.0 - 1.0)) as float;

    let hovering = norm_x >= BUTTON_LEFT && norm_x <= BUTTON_RIGHT &&
                   norm_y >= BUTTON_BOTTOM && norm_y <= BUTTON_TOP;

    let clicked = glfwGetMouseButton(window, 0) == 1 && hovering;

    // Color scheme
    let top_r = clicked ? 0.2f : 0.4f;
    let top_g = clicked ? 0.2f : 0.6f;
    let top_b = clicked ? 0.2f : 0.8f;

    let bottom_r = clicked ? 0.1f : 0.2f;
    let bottom_g = clicked ? 0.1f : 0.3f;
    let bottom_b = clicked ? 0.2f : 0.4f;

    // 3D-like button quad with shaded top and bottom
    glBegin(GL_QUADS);
        glColor3f(top_r, top_g, top_b);
        glVertex2f(BUTTON_LEFT, BUTTON_TOP);
        glVertex2f(BUTTON_RIGHT, BUTTON_TOP);

        glColor3f(bottom_r, bottom_g, bottom_b);
        glVertex2f(BUTTON_RIGHT, BUTTON_BOTTOM);
        glVertex2f(BUTTON_LEFT, BUTTON_BOTTOM);
    glEnd();

    // Click action
    if (clicked) {
        printf("Button clicked! Hello, World!\n");
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
}

glfwTerminate();