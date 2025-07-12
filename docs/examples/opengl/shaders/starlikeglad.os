extern "C" fn printf(fmt: char*, ...) => int;

// Threading support
extern "C" {
    fn CreateThread(lpThreadAttributes: void*, dwStackSize: uint, 
                   lpStartAddress: void*, lpParameter: void*, 
                   dwCreationFlags: uint, lpThreadId: uint*) => void*;
    fn WaitForSingleObject(hHandle: void*, dwMilliseconds: uint) => uint;
    fn CloseHandle(hObject: void*) => int;
    fn Sleep(dwMilliseconds: uint) => void;
    fn GetCurrentThreadId() => uint;
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
    fn glfwSetErrorCallback(callback: void*) => void*;
    fn glfwGetTime() => double;
    fn glfwGetFramebufferSize(window: void*, width: int*, height: int*) => void;
}

// GLAD
extern 
"dependencies/glad/gl/bin/glad_gl.dll",
"dependencies/glad/gl/bin/libglad_gl.a" {
    fn gladLoadGL(loader: void*) => int;

    const glad_glGetError: fn() => uint;
    const glad_glClear: fn(mask: uint) => void;
    const glad_glClearColor: fn(r: float, g: float, b: float, a: float) => void;
    const glad_glViewport: fn(x: int, y: int, width: int, height: int) => void;
    const glad_glGenBuffers: fn(n: int, buffers: uint*) => void;
    const glad_glBindBuffer: fn(target: uint, buffer: uint) => void;
    const glad_glBufferData: fn(target: uint, size: int, data: void*, usage: uint) => void;
    const glad_glGenVertexArrays: fn(n: int, arrays: uint*) => void;
    const glad_glBindVertexArray: fn(array: uint) => void;
    const glad_glVertexAttribPointer: fn(index: uint, size: int, type: uint, normalized: uint, stride: int, pointer: void*) => void;
    const glad_glEnableVertexAttribArray: fn(index: uint) => void;
    const glad_glCreateShader: fn(type: uint) => uint;
    const glad_glShaderSource: fn(shader: uint, count: int, string: char**, length: int*) => void;
    const glad_glCompileShader: fn(shader: uint) => void;
    const glad_glGetShaderiv: fn(shader: uint, pname: uint, params: int*) => void;
    const glad_glGetShaderInfoLog: fn(shader: uint, bufSize: int, length: int*, infoLog: char*) => void;
    const glad_glDeleteShader: fn(shader: uint) => void;
    const glad_glCreateProgram: fn() => uint;
    const glad_glAttachShader: fn(program: uint, shader: uint) => void;
    const glad_glLinkProgram: fn(program: uint) => void;
    const glad_glGetProgramiv: fn(program: uint, pname: uint, params: int*) => void;
    const glad_glGetProgramInfoLog: fn(program: uint, bufSize: int, length: int*, infoLog: char*) => void;
    const glad_glUseProgram: fn(program: uint) => void;
    const glad_glDeleteProgram: fn(program: uint) => void;
    const glad_glDrawArrays: fn(mode: uint, first: int, count: int) => void;
    const glad_glDeleteVertexArrays: fn(n: int, arrays: uint*) => void;
    const glad_glDeleteBuffers: fn(n: int, buffers: uint*) => void;
    const glad_glGetUniformLocation: fn(program: uint, name: char*) => int;
    const glad_glUniform1f: fn(location: int, v0: float) => void;
    const glad_glUniform2f: fn(location: int, v0: float, v1: float) => void;
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

// Threading constants
const INFINITE: uint = 0xFFFFFFFF;

// Global state for thread communication
let g_shouldExit: int = 0;
let g_window: void* = nullptr;
let g_renderThread: void* = nullptr;

// Shader sources
const vertexShaderSource = r"#version 330 core
layout(location = 0) in vec3 aPos;

void main() {
    gl_Position = vec4(aPos, 1.0);
}
";

const fragmentShaderSource = r"#version 330 core
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_resolution;

// Hash function for pseudo-random numbers
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

// Multi-hash for different random values
float hash2(vec2 p) {
    return fract(sin(dot(p, vec2(41.9898, 289.233))) * 85734.3751);
}

// Star field function with color variation
float starField(vec2 uv, float density, float brightness, float twinkle) {
    vec2 p = floor(uv * density);
    float r = hash(p);
    vec2 center = p + vec2(0.5);
    float star = smoothstep(0.95, 1.0, 
        pow(1.0 - distance(fract(uv * density), vec2(0.5)), 20.0));
    
    // Twinkle effect with time
    float twinkleFactor = (sin(u_time * (r * 2.0 + 1.0) + r * 10.0) * 0.5 + 0.5) * twinkle;
    return star * brightness * (0.8 + twinkleFactor * 0.2) * r;
}

// HSV to RGB conversion
vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    // Normalize coordinates to [-1, 1] range
    vec2 uv = (gl_FragCoord.xy * 2.0 - u_resolution) / u_resolution.y;
    
    // Colorful nebula background
    vec3 color = vec3(0.0);
    
    // Multi-colored nebula layers
    float nebula1 = sin(uv.x * 3.0 + u_time * 0.5) * cos(uv.y * 2.0 + u_time * 0.3);
    float nebula2 = sin(uv.x * 5.0 - u_time * 0.7) * cos(uv.y * 4.0 - u_time * 0.4);
    float nebula3 = sin(uv.x * 2.0 + uv.y * 3.0 + u_time * 0.6);
    
    // Create colorful nebula
    color += vec3(0.1, 0.02, 0.2) * (nebula1 * 0.5 + 0.5);  // Purple
    color += vec3(0.05, 0.1, 0.3) * (nebula2 * 0.3 + 0.3);  // Blue
    color += vec3(0.2, 0.05, 0.1) * (nebula3 * 0.2 + 0.2);  // Red
    
    // Add base dark space color
    color += vec3(0.02, 0.03, 0.1);
    
    // Convert back to [0, 1] range for star field functions
    vec2 starUV = (uv + 1.0) * 0.5;
    starUV.x *= u_resolution.x / u_resolution.y;
    
    // Multiple layers of colored stars
    float stars1 = starField(starUV, 15.0, 1.2, 0.8);  // Tiny stars
    float stars2 = starField(starUV, 8.0, 1.0, 0.6);   // Small stars
    float stars3 = starField(starUV, 4.0, 1.5, 0.9);   // Medium stars
    float stars4 = starField(starUV, 2.0, 2.0, 1.2);   // Large stars
    
    // Colored star layers with different hues
    float greenStars = starField(starUV * 1.3, 6.0, 0.8, 0.5);
    float orangeStars = starField(starUV * 0.7, 3.5, 0.9, 0.6);
    float purpleStars = starField(starUV * 1.1, 5.5, 0.7, 0.4);
    float cyanStars = starField(starUV * 0.9, 4.5, 0.6, 0.7);
    
    // Add rainbow-colored stars
    color += stars1 * vec3(1.0, 0.9, 0.8);        // Warm white
    color += stars2 * vec3(0.9, 0.9, 1.0);        // Cool white
    color += stars3 * vec3(1.0, 1.0, 0.7);        // Yellow
    color += stars4 * vec3(1.0, 0.8, 0.6);        // Orange
    
    color += greenStars * vec3(0.3, 1.0, 0.4);     // Green
    color += orangeStars * vec3(1.0, 0.5, 0.1);    // Orange
    color += purpleStars * vec3(0.8, 0.3, 1.0);    // Purple
    color += cyanStars * vec3(0.2, 0.8, 1.0);      // Cyan
    
    // Add more exotic colored stars
    float pinkStars = starField(starUV * 1.4, 7.0, 0.5, 0.3);
    float yellowStars = starField(starUV * 0.6, 3.0, 0.7, 0.5);
    
    color += pinkStars * vec3(1.0, 0.4, 0.8);      // Pink
    color += yellowStars * vec3(1.0, 1.0, 0.2);    // Bright yellow
    
    // Colorful gradient from center
    float dist = length(uv);
    vec3 gradientColor = hsv2rgb(vec3(dist * 0.3 + u_time * 0.1, 0.4, 0.8));
    color *= 1.0 - 0.2 * dist + 0.1 * gradientColor;
    
    // Rainbow central star pattern
    float angle = atan(uv.y, uv.x);
    float radius = length(uv);
    float star = abs(sin(6.0 * angle + u_time));
    float intensity = smoothstep(0.4, 0.0, abs(radius - star * 0.3));
    
    // Create rainbow effect for central pattern
    vec3 rainbowColor = hsv2rgb(vec3(angle / 6.28318 + u_time * 0.2, 0.8, 1.0));
    color += intensity * rainbowColor * 2;
    
    // Add some sparkle effects
    float sparkle1 = sin(uv.x * 20.0 + u_time * 3.0) * cos(uv.y * 20.0 + u_time * 2.0);
    float sparkle2 = sin(uv.x * 15.0 - u_time * 2.5) * cos(uv.y * 15.0 - u_time * 3.5);
    
    color += vec3(0.02, 0.01, 0.03) * sparkle1 * sparkle1;
    color += vec3(0.01, 0.03, 0.02) * sparkle2 * sparkle2;
    
    // Final color boost
    color *= 0.5;
    
    FragColor = vec4(color, 1.0);
}
";

// Error callback for GLFW
fn glfwErrorCallback(error: int, description: char*) => void {
    printf("GLFW Error %d: %s\n", error, description);
}

function compileShader(source: char*, type: uint) => uint {
    let shader = glad_glCreateShader(type);
    if (shader == 0) {
        printf("Failed to create shader\n");
        return 0;
    }

    glad_glShaderSource(shader, 1, &source, nullptr);
    glad_glCompileShader(shader);

    let success: int;
    glad_glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        let infoLog: char[512];
        glad_glGetShaderInfoLog(shader, 512, nullptr, &infoLog);
        printf("Shader compilation error:\n%s\n", &infoLog);
        glad_glDeleteShader(shader);
        return 0;
    }

    return shader;
}

function createShaderProgram() => uint {
    printf("Creating the shader program\n");
    let vertexShader = compileShader(&vertexShaderSource, GL_VERTEX_SHADER);
    printf("Compiled vertex shader\n");
    if (vertexShader == 0) return 0;

    let fragmentShader = compileShader(&fragmentShaderSource, GL_FRAGMENT_SHADER);
    printf("Compiled fragment shader\n");
    if (fragmentShader == 0) {
        glad_glDeleteShader(vertexShader);
        return 0;
    }

    let program = glad_glCreateProgram();
    glad_glAttachShader(program, vertexShader);
    glad_glAttachShader(program, fragmentShader);
    glad_glLinkProgram(program);

    let success: int;
    glad_glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        let infoLog: char[512];
        glad_glGetProgramInfoLog(program, 512, nullptr, &infoLog);
        printf("Shader linking error:\n%s\n", &infoLog);
        glad_glDeleteProgram(program);
        program = 0;
    }

    glad_glDeleteShader(vertexShader);
    glad_glDeleteShader(fragmentShader);

    return program;
}

// Thread function for rendering
fn renderThreadFunction(param: void*) => uint {
    printf("Render thread started (Thread ID: %d)\n", GetCurrentThreadId());
    
    // Make context current on this thread
    glfwMakeContextCurrent(g_window);
    printf("Made context current on render thread\n");
    
    // Initialize GLAD on this thread
    if (!gladLoadGL(glfwGetProcAddress)) {
        printf("Failed to initialize GLAD on render thread\n");
        return 1;
    }

    printf("Successfully initialized GLAD on render thread\n");

    // Create shader program
    let shaderProgram = createShaderProgram();
    if (shaderProgram == 0) {
        printf("Failed to create shader program on render thread\n");
        return 1;
    }
    printf("Created shader program on render thread\n");

    // Get uniform locations
    let timeLocation = glad_glGetUniformLocation(shaderProgram, "u_time");
    let resolutionLocation = glad_glGetUniformLocation(shaderProgram, "u_resolution");
    printf("Time location: %d, Resolution location: %d\n", timeLocation, resolutionLocation);

    // Set up fullscreen quad vertices (two triangles)
    let vertices: float[18] = [
        // First triangle
        -1.0, -1.0, 0.0,
         1.0, -1.0, 0.0,
        -1.0,  1.0, 0.0,
        // Second triangle
         1.0, -1.0, 0.0,
         1.0,  1.0, 0.0,
        -1.0,  1.0, 0.0
    ];

    let VAO: uint, VBO: uint;
    glad_glGenVertexArrays(1, &VAO);
    glad_glGenBuffers(1, &VBO);

    glad_glBindVertexArray(VAO);
    glad_glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glad_glBufferData(GL_ARRAY_BUFFER, 18 * 4, &vertices, GL_STATIC_DRAW);

    glad_glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * 4, nullptr);
    glad_glEnableVertexAttribArray(0);

    glad_glBindBuffer(GL_ARRAY_BUFFER, 0);
    glad_glBindVertexArray(0);

    printf("Starting render loop on separate thread\n");
    
    // Render loop
    while (!g_shouldExit) {
        // Get current time
        let currentTime = glfwGetTime() as float;
        
        // Get window size
        let width: int, height: int;
        glfwGetFramebufferSize(g_window, &width, &height);
        glad_glViewport(0, 0, width, height);

        // Clear screen
        glad_glClearColor(0.0, 0.0, 0.0, 1.0);
        glad_glClear(GL_COLOR_BUFFER_BIT);

        // Use shader program
        glad_glUseProgram(shaderProgram);
        
        // Set uniforms
        if (timeLocation != -1) {
            glad_glUniform1f(timeLocation, currentTime);
        }
        if (resolutionLocation != -1) {
            glad_glUniform2f(resolutionLocation, width as float, height as float);
        }

        // Draw fullscreen quad
        glad_glBindVertexArray(VAO);
        glad_glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(g_window);
        
        // Small sleep to prevent excessive CPU usage
        Sleep(8); // ~120 FPS target
    }

    printf("Render thread cleaning up\n");
    
    // Cleanup
    glad_glDeleteVertexArrays(1, &VAO);
    glad_glDeleteBuffers(1, &VBO);
    glad_glDeleteProgram(shaderProgram);

    printf("Render thread exiting\n");
    return 0;
}

function main() => void {
    printf("Main thread started (Thread ID: %d)\n", GetCurrentThreadId());
    
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
    g_window = glfwCreateWindow(800, 600, "Threaded Star Field", nullptr, nullptr);
    if (g_window == nullptr) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return;
    }
    printf("Successfully created the window\n");

    // Don't make context current on main thread - render thread will do it
    
    // Create render thread
    let threadId: uint;
    g_renderThread = CreateThread(nullptr, 0, renderThreadFunction, nullptr, 0, &threadId);
    if (g_renderThread == nullptr) {
        printf("Failed to create render thread\n");
        glfwTerminate();
        return;
    }
    printf("Created render thread with ID: %d\n", threadId);

    // Main thread handles events
    printf("Main thread entering event loop\n");
    while (!glfwWindowShouldClose(g_window)) {
        glfwPollEvents();
        
        // Sleep to prevent excessive CPU usage in main thread
        Sleep(16); // ~60 FPS for event polling
    }

    printf("Main thread signaling render thread to exit\n");
    g_shouldExit = 1;

    // Wait for render thread to finish
    printf("Waiting for render thread to finish\n");
    WaitForSingleObject(g_renderThread, INFINITE);
    CloseHandle(g_renderThread);
    printf("Render thread finished\n");

    // Cleanup
    glfwTerminate();
    printf("Application exiting\n");
}

// main();
// printf("Done\n");