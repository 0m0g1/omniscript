extern "C" {
    fn printf(...fmt: char*) => int;
    fn malloc(size: int) => void*;
    fn free(ptr: void*) => void;
    fn strlen(str: char*) => int;
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
}

// OpenGL Constants
let GL_COLOR_BUFFER_BIT: uint = 0x00004000;
let GL_ARRAY_BUFFER: uint = 0x8892;
let GL_STATIC_DRAW: uint = 0x88E4;
let GL_TRIANGLES: uint = 0x0004;
let GL_VERTEX_SHADER: uint = 0x8B31;
let GL_FRAGMENT_SHADER: uint = 0x8B30;
let GL_COMPILE_STATUS: uint = 0x8B81;
let GL_LINK_STATUS: uint = 0x8B82;
let GL_INFO_LOG_LENGTH: uint = 0x8B84;
let GL_FLOAT: uint = 0x1406;
let GL_FALSE: uint = 0;
let GL_TRUE: uint = 1;

// OpenGL error constants
let GL_NO_ERROR: uint = 0;
let GL_INVALID_ENUM: uint = 0x0500;
let GL_INVALID_VALUE: uint = 0x0501;
let GL_INVALID_OPERATION: uint = 0x0502;
let GL_STACK_OVERFLOW: uint = 0x0503;
let GL_STACK_UNDERFLOW: uint = 0x0504;
let GL_OUT_OF_MEMORY: uint = 0x0505;

// GLFW constants
let GLFW_CONTEXT_VERSION_MAJOR: uint = 0x00022002;
let GLFW_CONTEXT_VERSION_MINOR: uint = 0x00022003;
let GLFW_OPENGL_PROFILE: uint = 0x00022008;
let GLFW_OPENGL_CORE_PROFILE: uint = 0x00032001;

// OpenGL function pointers
type GLGETERROR = fn() => uint;
type GLGETSTRING = fn(name: uint) => char*;
type GLCLEAR = fn(mask: uint) => void;
type GLCLEARCOLOR = fn(r: float, g: float, b: float, a: float) => void;
type GLVIEWPORT = fn(x: int, y: int, width: int, height: int) => void;
type GLGENBUFFERS = fn(n: int, buffers: uint*) => void;
type GLBINDBUFFER = fn(target: uint, buffer: uint) => void;
type GLBUFFERDATA = fn(target: uint, size: int, data: void*, usage: uint) => void;
type GLGENVERTEXARRAYS = fn(n: int, arrays: uint*) => void;
type GLBINDVERTEXARRAY = fn(array: uint) => void;
type GLVERTEXATTRIBPOINTER = fn(index: uint, size: int, type: uint, normalized: uint8, stride: int, pointer: void*) => void;
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

// Global function pointers
let glGetError: GLGETERROR;
let glGetString: GLGETSTRING;
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

// OpenGL constants for glGetString
const GL_VENDOR: uint = 0x1F00;
const GL_RENDERER: uint = 0x1F01;
const GL_VERSION: uint = 0x1F02;
const GL_SHADING_LANGUAGE_VERSION: uint = 0x8B8C;

// Improved shader sources with null terminators
let vertexShaderSource = "#version 120\nattribute vec3 aPos;\nattribute vec3 aColor;\nvarying vec3 ourColor;\nvoid main()\n{\n    gl_Position = vec4(aPos, 1.0);\n    ourColor = aColor;\n}\0";

let fragmentShaderSource = "#version 120\nvarying vec3 ourColor;\nvoid main()\n{\n    gl_FragColor = vec4(ourColor, 1.0);\n}\0";

function printOpenGLError(operation: char*) => void {
    let error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("OpenGL error after %s: 0x%x ", operation, error);
        if (error == GL_INVALID_ENUM) {
            printf("(GL_INVALID_ENUM)\n");
        } else if (error == GL_INVALID_VALUE) {
            printf("(GL_INVALID_VALUE)\n");
        } else if (error == GL_INVALID_OPERATION) {
            printf("(GL_INVALID_OPERATION)\n");
        } else if (error == GL_OUT_OF_MEMORY) {
            printf("(GL_OUT_OF_MEMORY)\n");
        } else {
            printf("(Unknown error)\n");
        }
    }
}

function loadOpenGLFunctions() => int {
    printf("Loading OpenGL functions...\n");
    
    // Load basic functions first
    glGetError = glfwGetProcAddress("glGetError") as GLGETERROR;
    glGetString = glfwGetProcAddress("glGetString") as GLGETSTRING;
    
    if (glGetError == nullptr || glGetString == nullptr) {
        printf("Failed to load basic OpenGL functions\n");
        return 0;
    }
    
    // Clear any existing errors
    while (glGetError() != GL_NO_ERROR) {
        // Clear error queue
    }
    
    // Load core functions
    glClear = glfwGetProcAddress("glClear") as GLCLEAR;
    glClearColor = glfwGetProcAddress("glClearColor") as GLCLEARCOLOR;
    glViewport = glfwGetProcAddress("glViewport") as GLVIEWPORT;
    
    // Load buffer functions
    glGenBuffers = glfwGetProcAddress("glGenBuffers") as GLGENBUFFERS;
    glBindBuffer = glfwGetProcAddress("glBindBuffer") as GLBINDBUFFER;
    glBufferData = glfwGetProcAddress("glBufferData") as GLBUFFERDATA;
    glDeleteBuffers = glfwGetProcAddress("glDeleteBuffers") as GLDELETEBUFFERS;
    
    // Load vertex array functions
    glGenVertexArrays = glfwGetProcAddress("glGenVertexArrays") as GLGENVERTEXARRAYS;
    glBindVertexArray = glfwGetProcAddress("glBindVertexArray") as GLBINDVERTEXARRAY;
    glDeleteVertexArrays = glfwGetProcAddress("glDeleteVertexArrays") as GLDELETEVERTEXARRAYS;
    
    // Load vertex attribute functions
    glVertexAttribPointer = glfwGetProcAddress("glVertexAttribPointer") as GLVERTEXATTRIBPOINTER;
    glEnableVertexAttribArray = glfwGetProcAddress("glEnableVertexAttribArray") as GLENABLEVERTEXATTRIBARRAY;
    
    // Load shader functions
    glCreateShader = glfwGetProcAddress("glCreateShader") as GLCREATESHADER;
    glShaderSource = glfwGetProcAddress("glShaderSource") as GLSHADERSOURCE;
    glCompileShader = glfwGetProcAddress("glCompileShader") as GLCOMPILESHADER;
    glGetShaderiv = glfwGetProcAddress("glGetShaderiv") as GLGETSHADERIV;
    glGetShaderInfoLog = glfwGetProcAddress("glGetShaderInfoLog") as GLGETSHADERINFOLOG;
    glDeleteShader = glfwGetProcAddress("glDeleteShader") as GLDELETESHADER;
    
    // Load program functions
    glCreateProgram = glfwGetProcAddress("glCreateProgram") as GLCREATEPROGRAM;
    glAttachShader = glfwGetProcAddress("glAttachShader") as GLATTACHSHADER;
    glLinkProgram = glfwGetProcAddress("glLinkProgram") as GLLINKPROGRAM;
    glGetProgramiv = glfwGetProcAddress("glGetProgramiv") as GLGETPROGRAMIV;
    glGetProgramInfoLog = glfwGetProcAddress("glGetProgramInfoLog") as GLGETPROGRAMINFOLOG;
    glUseProgram = glfwGetProcAddress("glUseProgram") as GLUSEPROGRAM;
    glDeleteProgram = glfwGetProcAddress("glDeleteProgram") as GLDELETEPROGRAM;
    
    // Load draw functions
    glDrawArrays = glfwGetProcAddress("glDrawArrays") as GLDRAWARRAYS;

    // Check if critical functions were loaded
    if (glClear == nullptr || glClearColor == nullptr || glViewport == nullptr) {
        printf("Failed to load core OpenGL functions\n");
        return 0;
    }
    
    if (glCreateShader == nullptr || glShaderSource == nullptr || glCompileShader == nullptr) {
        printf("Failed to load shader functions\n");
        return 0;
    }
    
    if (glCreateProgram == nullptr || glDrawArrays == nullptr) {
        printf("Failed to load program/draw functions\n");
        return 0;
    }
    
    printf("OpenGL functions loaded successfully\n");
    return 1;
}

function compileShader(source: char*, type: uint) => uint {
    printf("Compiling shader (type: 0x%x)...\n", type);
    
    if (glCreateShader == nullptr) {
        printf("glCreateShader function not loaded\n");
        return 0;
    }
    
    let shader: uint = glCreateShader(type);
    if (shader == 0) {
        printf("Failed to create shader - glCreateShader returned 0\n");
        printOpenGLError("glCreateShader");
        return 0;
    }
    
    printf("Created shader with ID: %u\n", shader);
    
    // Use null-terminated string without explicit length
    glShaderSource(shader, 1, &source, nullptr);
    printOpenGLError("glShaderSource");
    
    printf("Compiling shader...\n");
    glCompileShader(shader);
    printOpenGLError("glCompileShader");

    let success: int;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    printOpenGLError("glGetShaderiv");
    
    printf("Shader compilation status: %s\n", (success == GL_TRUE) ? "SUCCESS" : "FAILED");
    
    if (success != GL_TRUE) {
        let infoLogLength: int;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        
        if (infoLogLength > 0) {
            let infoLog = malloc(infoLogLength + 1) as char*;
            if (infoLog != nullptr) {
                glGetShaderInfoLog(shader, infoLogLength, nullptr, infoLog);
                printf("Shader compilation error: %s\n", infoLog);
                free(infoLog);
            }
        } else {
            printf("Shader compilation failed: No error log available\n");
        }
        
        glDeleteShader(shader);
        return 0;
    }
    
    printf("Shader compiled successfully\n");
    return shader;
}

function createShaderProgram() => uint {
    printf("Creating shader program...\n");
    
    let vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    if (vertexShader == 0) {
        printf("Failed to compile vertex shader\n");
        return 0;
    }
    
    let fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    if (fragmentShader == 0) {
        printf("Failed to compile fragment shader\n");
        glDeleteShader(vertexShader);
        return 0;
    }

    let program = glCreateProgram();
    if (program == 0) {
        printf("Failed to create shader program\n");
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }
    
    printf("Attaching shaders to program...\n");
    glAttachShader(program, vertexShader);
    printOpenGLError("glAttachShader (vertex)");
    
    glAttachShader(program, fragmentShader);
    printOpenGLError("glAttachShader (fragment)");
    
    printf("Linking program...\n");
    glLinkProgram(program);
    printOpenGLError("glLinkProgram");

    let success: int;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success != GL_TRUE) {
        let infoLogLength: int;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        
        if (infoLogLength > 0) {
            let infoLog = malloc(infoLogLength + 1) as char*;
            if (infoLog != nullptr) {
                glGetProgramInfoLog(program, infoLogLength, nullptr, infoLog);
                printf("Program linking failed: %s\n", infoLog);
                free(infoLog);
            }
        } else {
            printf("Program linking failed: No error log available\n");
        }
        
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    // Clean up shaders (they're now linked into the program)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    printf("Shader program created successfully\n");
    return program;
}

function main() => void {
    printf("=== OpenGL Triangle Renderer ===\n");
    
    // Initialize GLFW
    printf("Initializing GLFW...\n");
    if (glfwInit() == 0) {
        printf("Failed to initialize GLFW\n");
        return;
    }

    // Request OpenGL 3.3 Core Profile for better shader support
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    printf("Creating window...\n");
    let window = glfwCreateWindow(800, 600, "OpenGL Triangle", nullptr, nullptr);
    if (window == nullptr) {
        printf("Failed to create window\n");
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);

    // Load OpenGL functions
    if (loadOpenGLFunctions() == 0) {
        printf("Failed to load OpenGL functions\n");
        glfwTerminate();
        return;
    }

    // Print OpenGL context information
    printf("\n=== OpenGL Context Information ===\n");
    let vendor = glGetString(GL_VENDOR);
    let renderer = glGetString(GL_RENDERER);
    let version = glGetString(GL_VERSION);
    let glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("Vendor: %s\n", vendor != nullptr ? vendor : "Unknown");
    printf("Renderer: %s\n", renderer != nullptr ? renderer : "Unknown");
    printf("Version: %s\n", version != nullptr ? version : "Unknown");
    printf("GLSL Version: %s\n", glslVersion != nullptr ? glslVersion : "Unknown");

    // Clear any initial errors
    while (glGetError() != GL_NO_ERROR) {
        // Clear error queue
    }

    // Test basic OpenGL functionality
    printf("\n=== Setting up OpenGL ===\n");
    glViewport(0, 0, 800, 600);
    printOpenGLError("glViewport");

    glClearColor(0.2, 0.3, 0.3, 1.0);
    printOpenGLError("glClearColor");

    // Create shader program
    let shaderProgram = createShaderProgram();
    if (shaderProgram == 0) {
        printf("Failed to create shader program\n");
        glfwTerminate();
        return;
    }

    // Triangle vertices with position and color
    let vertices: float[18] = [
        // Position      // Color
         0.0,  0.5, 0.0,  1.0, 0.0, 0.0,  // Top vertex - Red
        -0.5, -0.5, 0.0,  0.0, 1.0, 0.0,  // Bottom-left vertex - Green
         0.5, -0.5, 0.0,  0.0, 0.0, 1.0   // Bottom-right vertex - Blue
    ];

    printf("\n=== Setting up vertex data ===\n");

    // Create and bind VAO
    let VAO: uint;
    glGenVertexArrays(1, &VAO);
    printOpenGLError("glGenVertexArrays");
    
    glBindVertexArray(VAO);
    printOpenGLError("glBindVertexArray");

    // Create VBO
    let VBO: uint;
    glGenBuffers(1, &VBO);
    printOpenGLError("glGenBuffers");

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    printOpenGLError("glBindBuffer");

    glBufferData(GL_ARRAY_BUFFER, 18 * 4, &vertices, GL_STATIC_DRAW);
    printOpenGLError("glBufferData");

    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * 4, nullptr);
    printOpenGLError("glVertexAttribPointer (position)");
    glEnableVertexAttribArray(0);
    printOpenGLError("glEnableVertexAttribArray (position)");
    
    // Color attribute (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * 4, (3 * 4) as void*);
    printOpenGLError("glVertexAttribPointer (color)");
    glEnableVertexAttribArray(1);
    printOpenGLError("glEnableVertexAttribArray (color)");

    // Unbind VAO
    glBindVertexArray(0);

    printf("\n=== Starting main loop ===\n");
    let frameCount: int = 0;

    // Main render loop
    while (glfwWindowShouldClose(window) == 0) {
        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Use shader program
        glUseProgram(shaderProgram);
        
        // Bind VAO and draw
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        frameCount++;
        if (frameCount == 1) {
            printf("First frame rendered successfully!\n");
        }
    }

    printf("\n=== Cleaning up ===\n");
    
    // Cleanup
    if (glDeleteVertexArrays != nullptr) {
        glDeleteVertexArrays(1, &VAO);
    }
    if (glDeleteBuffers != nullptr) {
        glDeleteBuffers(1, &VBO);
    }
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    
    printf("Program finished successfully\n");
}

// Entry point
main();