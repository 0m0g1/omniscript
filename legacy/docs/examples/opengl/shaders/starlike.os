/*
This code is complete as is, linker flags will be generated automatically based of the provided library paths in the ffi and any available linkers in your system
Compile with:
./path/to/Osengine.exe docs/examples/opengl/shaders/starlikeglad.os // For JIT mode
./path/to/Osengine.exe docs/examples/opengl/shaders/starlikeglad.os --make -o app.exe // For AOT mode
*/
extern "C" fn printf(fmt: char*, ...) => int;

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
type GLGETUNIFORMLOCATION = fn(program: uint, name: char*) => int;
type GLUNIFORM1F = fn(location: int, v0: float) => void;
type GLUNIFORM2F = fn(location: int, v0: float, v1: float) => void;

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
let glGetUniformLocation: GLGETUNIFORMLOCATION;
let glUniform1f: GLUNIFORM1F;
let glUniform2f: GLUNIFORM2F;

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
    glGetUniformLocation = loadGLFunction("glGetUniformLocation", 1) as GLGETUNIFORMLOCATION;
    glUniform1f = loadGLFunction("glUniform1f", 1) as GLUNIFORM1F;
    glUniform2f = loadGLFunction("glUniform2f", 1) as GLUNIFORM2F;
}

function compileShader(source: char*, type: uint) => uint {
    let shader = glCreateShader(type);
    if (shader == 0) {
        printf("Failed to create shader\n");
        return 0;
    }

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    let success: int;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        let infoLog: char[512];
        glGetShaderInfoLog(shader, 512, nullptr, &infoLog);
        printf("Shader compilation error:\n%s\n", &infoLog);
        glDeleteShader(shader);
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
        let infoLog: char[512];
        glGetProgramInfoLog(program, 512, nullptr, &infoLog);
        printf("Shader linking error:\n%s\n", &infoLog);
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
    let window = glfwCreateWindow(800, 600, "Star Field", nullptr, nullptr);
    if (window == nullptr) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return;
    }
    printf("Successfully created the window\n");
    
    glfwMakeContextCurrent(window);
    printf("Successfully made the context current\n");
    
    // Load OpenGL functions
    loadOpenGLFunctions();
    printf("Successfully loaded OpenGL functions\n");

    // Create shader program
    let shaderProgram = createShaderProgram();
    printf("Checking if the shader program is valid\n");
    if (shaderProgram == 0) {
        glfwTerminate();
        return;
    }
    printf("Successfully created the shader program\n");

    // Get uniform locations
    let timeLocation = glGetUniformLocation(shaderProgram, "u_time");
    let resolutionLocation = glGetUniformLocation(shaderProgram, "u_resolution");
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
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 18 * 4 /*sizeof(vertices)*/, &vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * 4/*sizeof(float)*/, nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    printf("Starting main loop\n");
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Get current time
        let currentTime = glfwGetTime() as float;
        
        // Get window size
        let width: int, height: int;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        // Clear screen
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        // Use shader program
        glUseProgram(shaderProgram);
        
        // Set uniforms
        if (timeLocation != -1) {
            glUniform1f(timeLocation, currentTime);
        }
        if (resolutionLocation != -1) {
            glUniform2f(resolutionLocation, width as float, height as float);
        }

        // Draw fullscreen quad
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    printf("Done\n");
}