extern "C" {
    fn printf(fmt: char*, ...) => int;
    // fn sin(x: double) => double;
    // fn cos(x: double) => double;
    // fn GetTickCount() => uint;
}

// extern "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dll" {
//     fn glfwInit() => int;
//     fn glfwCreateWindow(width: int, height: int, title: char*, monitor: void*, share: void*) => void*;
//     fn glfwMakeContextCurrent(window: void*) => void;
//     fn glfwWindowShouldClose(window: void*) => int;
//     fn glfwPollEvents() => void;
//     fn glfwSwapBuffers(window: void*) => void;
//     fn glfwTerminate() => void;
//     fn glfwGetTime() => double;
// }

// extern "opengl32.dll" {
//     fn glClearColor(r: float, g: float, b: float, a: float) => void;
//     fn glClear(mask: uint) => void;
//     fn glBegin(mode: uint) => void;
//     fn glEnd() => void;
//     fn glVertex2f(x: float, y: float) => void;
//     fn glColor3f(r: float, g: float, b: float) => void;
//     fn glLoadIdentity() => void;
//     fn glViewport(x: int, y: int, width: int, height: int) => void;
//     fn glMatrixMode(mode: uint) => void;
//     fn glOrtho(left: double, right: double, bottom: double, top: double, near: double, far: double) => void;
//     fn glPushMatrix() => void;
//     fn glPopMatrix() => void;
//     fn glTranslatef(x: float, y: float, z: float) => void;
//     fn glRotatef(angle: float, x: float, y: float, z: float) => void;
//     fn glScalef(x: float, y: float, z: float) => void;
// }

// const GL_COLOR_BUFFER_BIT = 0x00004000;
// const GL_PROJECTION = 0x1701;
// const GL_MODELVIEW = 0x1700;
// const GL_TRIANGLES = 0x0004;
// const GL_QUADS = 0x0007;
// const PI = 3.14159265359;

// Shader-like color functions
function wave_color(time: double, phase: double) => float {
    return (sin(time * 2.0 + phase) + 1.0) as float * 0.5;
}

printf("%.2f", wave_color(30.0, 5.0))

// function pulse_color(time: double, frequency: double) => float {
//     return (sin(time * frequency) * sin(time * frequency)) as float;
// }

// function gradient_color(pos: float, time: double) => float {
//     return (sin(pos * PI + time) + 1.0) as float * 0.5;
// }

// // Render a color-cycling triangle
// function render_triangle(time: double, x_offset: float, y_offset: float) => void {
//     glPushMatrix();
//     glTranslatef(x_offset, y_offset, 0.0);
//     glRotatef((time * 30.0) as float, 0.0, 0.0, 1.0);
    
//     glBegin(GL_TRIANGLES);
    
//     // Top vertex - Cycling red component
//     glColor3f(wave_color(time, 0.0), wave_color(time, 2.0), wave_color(time, 4.0));
//     glVertex2f(0.0, 0.3);
    
//     // // Bottom-left vertex - Pulsing green
//     // glColor3f(pulse_color(time, 3.0), wave_color(time, 1.0), pulse_color(time, 2.0));
//     // glVertex2f(-0.3, -0.3);
    
//     // // Bottom-right vertex - Gradient blue
//     // glColor3f(gradient_color(-0.3, time), gradient_color(0.3, time), wave_color(time, 3.0));
//     // glVertex2f(0.3, -0.3);
    
//     // glEnd();
//     // glPopMatrix();
// }

// // Render a color-morphing quad
// function render_quad(time: double, x_offset: float, y_offset: float) => void {
//     // glPushMatrix();
//     // glTranslatef(x_offset, y_offset, 0.0);
//     // glScalef(sin(time * 0.5) * 0.3 + 0.7, cos(time * 0.7) * 0.3 + 0.7, 1.0);
    
//     // glBegin(GL_QUADS);
    
//     // // Each corner gets a different color pattern
//     // glColor3f(wave_color(time, 0.0), 0.0, wave_color(time, PI));
//     // glVertex2f(-0.2, 0.2);
    
//     // glColor3f(0.0, wave_color(time, PI/2), wave_color(time, 3*PI/2));
//     // glVertex2f(0.2, 0.2);
    
//     // glColor3f(wave_color(time, PI), wave_color(time, PI), 0.0);
//     // glVertex2f(0.2, -0.2);
    
//     // glColor3f(wave_color(time, 3*PI/2), 0.0, wave_color(time, PI/2));
//     // glVertex2f(-0.2, -0.2);
    
//     // glEnd();
//     // glPopMatrix();
// }

// // Main shader test
// if (glfwInit() == 0) {
//     printf("Failed to initialize GLFW\n");
//     return;
// }

// let window = glfwCreateWindow(1200, 800, "Shader Test - Color Morphing", nullptr, nullptr);
// if (window == nullptr) {
//     printf("Failed to create window\n");
//     glfwTerminate();
//     return;
// }

// glfwMakeContextCurrent(window);

// glViewport(0, 0, 1200, 800);
// glMatrixMode(GL_PROJECTION);
// glLoadIdentity();
// glOrtho(-2, 2, -1.5, 1.5, -1, 1);
// glMatrixMode(GL_MODELVIEW);

// printf("Shader Test Started - Press ESC or close window to exit\n");
// printf("Watch the color morphing and geometric transformations!\n");

// while (glfwWindowShouldClose(window) == 0) {
//     // let time = glfwGetTime();
    
//     // // Dynamic background color
//     // let bg_r = wave_color(time * 0.3, 0.0) * 0.1;
//     // let bg_g = wave_color(time * 0.2, PI/3) * 0.1;
//     // let bg_b = wave_color(time * 0.4, 2*PI/3) * 0.1;
//     // glClearColor(bg_r, bg_g, bg_b, 1.0);
//     // glClear(GL_COLOR_BUFFER_BIT);
    
//     // glLoadIdentity();
    
//     // // Render multiple triangles with different phases
//     // render_triangle(time, -1.0, 0.5);
//     // render_triangle(time + PI/2, 0.0, 0.5);
//     // render_triangle(time + PI, 1.0, 0.5);
    
//     // // Render morphing quads
//     // render_quad(time, -1.0, -0.5);
//     // render_quad(time * 1.3, 0.0, -0.5);
//     // render_quad(time * 0.7, 1.0, -0.5);
    
//     // // Central complex shape - combining multiple primitives
//     // glPushMatrix();
//     // glTranslatef(0.0, 0.0, 0.0);
//     // glRotatef((time * 45.0) as float, 0.0, 0.0, 1.0);
    
//     // // Outer ring of triangles
//     // for (let i = 0; i < 8; i++) {
//     //     let angle = (i * 2.0 * PI) / 8.0;
//     //     let x = cos(angle) * 0.8;
//     //     let y = sin(angle) * 0.8;
        
//     //     glPushMatrix();
//     //     glTranslatef(x, y, 0.0);
//     //     glRotatef((angle * 180.0 / PI + time * 60.0) as float, 0.0, 0.0, 1.0);
//     //     glScalef(0.3, 0.3, 1.0);
        
//     //     glBegin(GL_TRIANGLES);
//     //     // glColor3f(wave_color(time + i, 0.0), wave_color(time + i, 2.0), wave_color(time + i, 4.0));
//     //     glVertex2f(0.0, 0.5);
//     //     glVertex2f(-0.3, -0.3);
//     //     glVertex2f(0.3, -0.3);
//     //     glEnd();
        
//     //     glPopMatrix();
//     // }
    
//     // glPopMatrix();
    
//     glfwSwapBuffers(window);
//     glfwPollEvents();
// }

// printf("Shader Test Completed\n");
// glfwTerminate();