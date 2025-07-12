/*
C++ equivalent without standard library
Compile with:
g++ -o starfield.exe starfield.cpp -lglfw3 -lglad -lgdi32 -lopengl32 -lkernel32
*/

// Basic C library functions
extern "C" {
    int printf(const char* fmt, ...);
    void* malloc(unsigned long size);
    void free(void* ptr);
    int strcmp(const char* str1, const char* str2);
    unsigned long strlen(const char* str);
    void* memcpy(void* dest, const void* src, unsigned long n);
    void* memset(void* ptr, int value, unsigned long num);
}

// Threading support
extern "C" {
    void* CreateThread(void* lpThreadAttributes, unsigned int dwStackSize, 
                      void* lpStartAddress, void* lpParameter, 
                      unsigned int dwCreationFlags, unsigned int* lpThreadId);
    unsigned int WaitForSingleObject(void* hHandle, unsigned int dwMilliseconds);
    int CloseHandle(void* hObject);
    void Sleep(unsigned int dwMilliseconds);
    unsigned int GetCurrentThreadId();
}

// GLFW declarations
extern "C" {
    int glfwInit();
    void* glfwCreateWindow(int width, int height, const char* title, void* monitor, void* share);
    void glfwMakeContextCurrent(void* window);
    int glfwWindowShouldClose(void* window);
    void glfwPollEvents();
    void glfwSwapBuffers(void* window);
    void glfwTerminate();
    void* glfwGetProcAddress(const char* name);
    void glfwWindowHint(int hint, int value);
    void* glfwSetErrorCallback(void* callback);
    double glfwGetTime();
    void glfwGetFramebufferSize(void* window, int* width, int* height);
}

// GLAD declarations
extern "C" {
    int gladLoadGL(void* loader);
    
    // OpenGL function pointers
    extern unsigned int (*glad_glGetError)();
    extern void (*glad_glClear)(unsigned int mask);
    extern void (*glad_glClearColor)(float r, float g, float b, float a);
    extern void (*glad_glViewport)(int x, int y, int width, int height);
    extern void (*glad_glGenBuffers)(int n, unsigned int* buffers);
    extern void (*glad_glBindBuffer)(unsigned int target, unsigned int buffer);
    extern void (*glad_glBufferData)(unsigned int target, int size, const void* data, unsigned int usage);
    extern void (*glad_glGenVertexArrays)(int n, unsigned int* arrays);
    extern void (*glad_glBindVertexArray)(unsigned int array);
    extern void (*glad_glVertexAttribPointer)(unsigned int index, int size, unsigned int type, unsigned int normalized, int stride, const void* pointer);
    extern void (*glad_glEnableVertexAttribArray)(unsigned int index);
    extern unsigned int (*glad_glCreateShader)(unsigned int type);
    extern void (*glad_glShaderSource)(unsigned int shader, int count, const char** string, const int* length);
    extern void (*glad_glCompileShader)(unsigned int shader);
    extern void (*glad_glGetShaderiv)(unsigned int shader, unsigned int pname, int* params);
    extern void (*glad_glGetShaderInfoLog)(unsigned int shader, int bufSize, int* length, char* infoLog);
    extern void (*glad_glDeleteShader)(unsigned int shader);
    extern unsigned int (*glad_glCreateProgram)();
    extern void (*glad_glAttachShader)(unsigned int program, unsigned int shader);
    extern void (*glad_glLinkProgram)(unsigned int program);
    extern void (*glad_glGetProgramiv)(unsigned int program, unsigned int pname, int* params);
    extern void (*glad_glGetProgramInfoLog)(unsigned int program, int bufSize, int* length, char* infoLog);
    extern void (*glad_glUseProgram)(unsigned int program);
    extern void (*glad_glDeleteProgram)(unsigned int program);
    extern void (*glad_glDrawArrays)(unsigned int mode, int first, int count);
    extern void (*glad_glDeleteVertexArrays)(int n, const unsigned int* arrays);
    extern void (*glad_glDeleteBuffers)(int n, const unsigned int* buffers);
    extern int (*glad_glGetUniformLocation)(unsigned int program, const char* name);
    extern void (*glad_glUniform1f)(int location, float v0);
    extern void (*glad_glUniform2f)(int location, float v0, float v1);
}

// OpenGL Constants
const unsigned int GL_COLOR_BUFFER_BIT = 0x00004000;
const unsigned int GL_ARRAY_BUFFER = 0x8892;
const unsigned int GL_STATIC_DRAW = 0x88E4;
const unsigned int GL_TRIANGLES = 0x0004;
const unsigned int GL_VERTEX_SHADER = 0x8B31;
const unsigned int GL_FRAGMENT_SHADER = 0x8B30;
const unsigned int GL_COMPILE_STATUS = 0x8B81;
const unsigned int GL_LINK_STATUS = 0x8B82;
const unsigned int GL_INFO_LOG_LENGTH = 0x8B84;
const unsigned int GL_FLOAT = 0x1406;
const unsigned int GL_FALSE = 0;
const int GL_TRUE = 1;

// GLFW constants
const unsigned int GLFW_CONTEXT_VERSION_MAJOR = 0x00022002;
const unsigned int GLFW_CONTEXT_VERSION_MINOR = 0x00022003;
const unsigned int GLFW_OPENGL_PROFILE = 0x00022008;
const unsigned int GLFW_OPENGL_CORE_PROFILE = 0x00032001;

// Threading constants
const unsigned int INFINITE = 0xFFFFFFFF;

// Global state for thread communication
int g_shouldExit = 0;
void* g_window = nullptr;
void* g_renderThread = nullptr;

// Shader sources
const char* vertexShaderSource = R"(#version 330 core
layout(location = 0) in vec3 aPos;

void main() {
    gl_Position = vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(#version 330 core
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
)";

// Error callback for GLFW
void glfwErrorCallback(int error, const char* description) {
    printf("GLFW Error %d: %s\n", error, description);
}

unsigned int compileShader(const char* source, unsigned int type) {
    unsigned int shader = glad_glCreateShader(type);
    if (shader == 0) {
        printf("Failed to create shader\n");
        return 0;
    }

    glad_glShaderSource(shader, 1, &source, nullptr);
    glad_glCompileShader(shader);

    int success;
    glad_glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glad_glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        printf("Shader compilation error:\n%s\n", infoLog);
        glad_glDeleteShader(shader);
        return 0;
    }

    return shader;
}

unsigned int createShaderProgram() {
    printf("Creating the shader program\n");
    unsigned int vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    printf("Compiled vertex shader\n");
    if (vertexShader == 0) return 0;

    unsigned int fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    printf("Compiled fragment shader\n");
    if (fragmentShader == 0) {
        glad_glDeleteShader(vertexShader);
        return 0;
    }

    unsigned int program = glad_glCreateProgram();
    glad_glAttachShader(program, vertexShader);
    glad_glAttachShader(program, fragmentShader);
    glad_glLinkProgram(program);

    int success;
    glad_glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glad_glGetProgramInfoLog(program, 512, nullptr, infoLog);
        printf("Shader linking error:\n%s\n", infoLog);
        glad_glDeleteProgram(program);
        program = 0;
    }

    glad_glDeleteShader(vertexShader);
    glad_glDeleteShader(fragmentShader);

    return program;
}

// Thread function for rendering
unsigned int __stdcall renderThreadFunction(void* param) {
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
    unsigned int shaderProgram = createShaderProgram();
    if (shaderProgram == 0) {
        printf("Failed to create shader program on render thread\n");
        return 1;
    }
    printf("Created shader program on render thread\n");

    // Get uniform locations
    int timeLocation = glad_glGetUniformLocation(shaderProgram, "u_time");
    int resolutionLocation = glad_glGetUniformLocation(shaderProgram, "u_resolution");
    printf("Time location: %d, Resolution location: %d\n", timeLocation, resolutionLocation);

    // Set up fullscreen quad vertices (two triangles)
    float vertices[18] = {
        // First triangle
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
        // Second triangle
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f
    };

    unsigned int VAO, VBO;
    glad_glGenVertexArrays(1, &VAO);
    glad_glGenBuffers(1, &VBO);

    glad_glBindVertexArray(VAO);
    glad_glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glad_glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), vertices, GL_STATIC_DRAW);

    glad_glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glad_glEnableVertexAttribArray(0);

    glad_glBindBuffer(GL_ARRAY_BUFFER, 0);
    glad_glBindVertexArray(0);

    printf("Starting render loop on separate thread\n");
    
    // Render loop
    while (!g_shouldExit) {
        // Get current time
        float currentTime = static_cast<float>(glfwGetTime());
        
        // Get window size
        int width, height;
        glfwGetFramebufferSize(g_window, &width, &height);
        glad_glViewport(0, 0, width, height);

        // Clear screen
        glad_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glad_glClear(GL_COLOR_BUFFER_BIT);

        // Use shader program
        glad_glUseProgram(shaderProgram);
        
        // Set uniforms
        if (timeLocation != -1) {
            glad_glUniform1f(timeLocation, currentTime);
        }
        if (resolutionLocation != -1) {
            glad_glUniform2f(resolutionLocation, static_cast<float>(width), static_cast<float>(height));
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

int main() {
    printf("Main thread started (Thread ID: %d)\n", GetCurrentThreadId());
    
    // Initialize GLFW
    glfwSetErrorCallback(reinterpret_cast<void*>(glfwErrorCallback));
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
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
        return -1;
    }
    printf("Successfully created the window\n");

    // Don't make context current on main thread - render thread will do it
    
    // Create render thread
    unsigned int threadId;
    g_renderThread = CreateThread(nullptr, 0, reinterpret_cast<void*>(renderThreadFunction), nullptr, 0, &threadId);
    if (g_renderThread == nullptr) {
        printf("Failed to create render thread\n");
        glfwTerminate();
        return -1;
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
    return 0;
}