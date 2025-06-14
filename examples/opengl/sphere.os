extern "C" {
    fn sin(x: double) => double;
    fn cos(x: double) => double;
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
const GL_TRIANGLE_STRIP = 0x0005;
const GL_DEPTH_TEST = 0x0B71;
const GL_LESS = 0x0201;

fn drawSphere(radius: float, stacks: int, slices: int) => void {
    let PI = 3.1415926;
    let i = 0;
    while (i < stacks) {
        let lat0 = PI * (-0.5 + (i as float) / stacks);
        let z0 = radius * sin(lat0);
        let zr0 = radius * cos(lat0);

        let lat1 = PI * (-0.5 + ((i + 1) as float) / stacks);
        let z1 = radius * sin(lat1);
        let zr1 = radius * cos(lat1);

        let j = 0;
        glBegin(GL_TRIANGLE_STRIP);
        while (j <= slices) {
            let lng = 2 * PI * (j as float) / slices;
            let x = cos(lng) as float;
            let y = sin(lng) as float;

            // Color gradient by position
            glColor3f((x + 1) as float / 2, (y + 1) as float / 2, (z0 as float + radius) as float / (2 * radius));
            glVertex3f(x * zr0 as float, y * zr0 as float, z0);

            glColor3f((x + 1) as float / 2, (y + 1) as float / 2, (z1 as float + radius) as float / (2 * radius));
            glVertex3f(x * zr1 as float, y * zr1 as float, z1 as float);

            j += 1;
        }
        glEnd();
        i += 1;
    }
}

if (glfwInit() == 0) {
    printf("Failed to initialize GLFW\n");
    return;
}

let window = glfwCreateWindow(800, 600, "Gradient Sphere", nullptr, nullptr);
if (window == nullptr) {
    printf("Failed to create window\n");
    glfwTerminate();
    return;
}

glfwMakeContextCurrent(window);

glViewport(0, 0, 800, 600);
glMatrixMode(GL_PROJECTION);
glLoadIdentity();
glOrtho(-2, 2, -2, 2, -4, 4);
glMatrixMode(GL_MODELVIEW);

glEnable(GL_DEPTH_TEST);
glClearDepth(1.0);
glDepthFunc(GL_LESS);

while (glfwWindowShouldClose(window) == 0) {
    let time = glfwGetTime();
    let angle = (time * 20.0) as float;

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glTranslatef(0.0, 0.0, -1.0);
    glRotatef(angle, 0.2, 1.0, 0.0);

    drawSphere(1.0, 30, 30);

    glfwSwapBuffers(window);
    glfwPollEvents();
}

glfwTerminate();
