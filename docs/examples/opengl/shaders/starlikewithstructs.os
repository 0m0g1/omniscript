// Structs to organize your data better

struct Vec2 {
    x: float = 0.0;
    y: float = 0.0;
    
    constructor(x: float, y: float) => void {
        this.x = x;
        this.y = y;
    }
}

struct Vec3 {
    x: float = 0.0;
    y: float = 0.0;
    z: float = 0.0;
    
    constructor(x: float, y: float, z: float) => void {
        this.x = x;
        this.y = y;
        this.z = z;
    }
}

struct WindowConfig {
    width: int = 800;
    height: int = 600;
    title: char* = "Threaded Star Field";
    contextMajor: int = 3;
    contextMinor: int = 3;
    
    constructor(width: int, height: int, title: char*) => void {
        this.width = width;
        this.height = height;
        this.title = title;
        this.contextMajor = 3;
        this.contextMinor = 3;
    }
}

struct RenderState {
    shaderProgram: uint = 0;
    VAO: uint = 0;
    VBO: uint = 0;
    timeLocation: int = -1;
    resolutionLocation: int = -1;
    shouldExit: int = 0;
    
    constructor() => void {
        this.shaderProgram = 0;
        this.VAO = 0;
        this.VBO = 0;
        this.timeLocation = -1;
        this.resolutionLocation = -1;
        this.shouldExit = 0;
    }
}

struct ThreadData {
    window: void* = nullptr;
    renderThread: void* = nullptr;
    mainThreadId: uint = 0;
    renderThreadId: uint = 0;
    
    constructor() => void {
        this.window = nullptr;
        this.renderThread = nullptr;
        this.mainThreadId = 0;
        this.renderThreadId = 0;
    }
}

struct StarLayer {
    density: float = 10.0;
    brightness: float = 1.0;
    twinkle: float = 0.5;
    color: Vec3 = Vec3{1.0, 1.0, 1.0};
    scale: float = 1.0;
    
    constructor(density: float, brightness: float, twinkle: float, r: float, g: float, b: float, scale: float) => void {
        this.density = density;
        this.brightness = brightness;
        this.twinkle = twinkle;
        this.color = Vec3{r, g, b};
        this.scale = scale;
    }
}

struct ShaderUniforms {
    timeLocation: int = -1;
    resolutionLocation: int = -1;
    
    constructor(program: uint) => void {
        this.timeLocation = glad_glGetUniformLocation(program, "u_time");
        this.resolutionLocation = glad_glGetUniformLocation(program, "u_resolution");
    }
    
    updateTime(time: float) => void {
        if (this.timeLocation != -1) {
            glad_glUniform1f(this.timeLocation, time);
        }
    }
    
    updateResolution(width: float, height: float) => void {
        if (this.resolutionLocation != -1) {
            glad_glUniform2f(this.resolutionLocation, width, height);
        }
    }
}

struct FullscreenQuad {
    vertices: float[18] = [
        // First triangle
        -1.0, -1.0, 0.0,
         1.0, -1.0, 0.0,
        -1.0,  1.0, 0.0,
        // Second triangle
         1.0, -1.0, 0.0,
         1.0,  1.0, 0.0,
        -1.0,  1.0, 0.0
    ];
    VAO: uint = 0;
    VBO: uint = 0;
    
    constructor() => void {
        glad_glGenVertexArrays(1, &this.VAO);
        glad_glGenBuffers(1, &this.VBO);

        glad_glBindVertexArray(this.VAO);
        glad_glBindBuffer(GL_ARRAY_BUFFER, this.VBO);
        glad_glBufferData(GL_ARRAY_BUFFER, 18 * 4, &this.vertices, GL_STATIC_DRAW);

        glad_glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * 4, nullptr);
        glad_glEnableVertexAttribArray(0);

        glad_glBindBuffer(GL_ARRAY_BUFFER, 0);
        glad_glBindVertexArray(0);
    }
    
    render() => void {
        glad_glBindVertexArray(this.VAO);
        glad_glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    cleanup() => void {
        glad_glDeleteVertexArrays(1, &this.VAO);
        glad_glDeleteBuffers(1, &this.VBO);
    }
}

struct PerformanceConfig {
    renderFPS: uint = 120;  // Target render FPS
    eventFPS: uint = 60;    // Target event polling FPS
    
    constructor(renderFPS: uint, eventFPS: uint) => void {
        this.renderFPS = renderFPS;
        this.eventFPS = eventFPS;
    }
    
    getRenderSleepTime() => uint {
        return 1000 / this.renderFPS;
    }
    
    getEventSleepTime() => uint {
        return 1000 / this.eventFPS;
    }
}

// Global state using structs
let g_threadData = ThreadData{};
let g_renderState = RenderState{};
let g_perfConfig = PerformanceConfig{120, 60};

// Example of how you could use these structs in your functions:

function initializeWindow(config: WindowConfig*) => void* {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, config.contextMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, config.contextMinor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    return glfwCreateWindow(config.width, config.height, config.title, nullptr, nullptr);
}

function updateUniforms(uniforms: ShaderUniforms*, window: void*) => void {
    let currentTime = glfwGetTime() as float;
    uniforms.updateTime(currentTime);
    
    let width: int, height: int;
    glfwGetFramebufferSize(window, &width, &height);
    uniforms.updateResolution(width as float, height as float);
}

// Modified render thread function using structs
function renderThreadFunctionWithStructs(param: void*) => uint {
    printf("Render thread started (Thread ID: %d)\n", GetCurrentThreadId());
    
    glfwMakeContextCurrent(g_threadData.window);
    
    if (!gladLoadGL(glfwGetProcAddress)) {
        printf("Failed to initialize GLAD on render thread\n");
        return 1;
    }

    g_renderState.shaderProgram = createShaderProgram();
    if (g_renderState.shaderProgram == 0) {
        return 1;
    }

    let uniforms = ShaderUniforms{g_renderState.shaderProgram};
    let quad = FullscreenQuad{};
    
    while (!g_renderState.shouldExit) {
        let width: int, height: int;
        glfwGetFramebufferSize(g_threadData.window, &width, &height);
        glad_glViewport(0, 0, width, height);

        glad_glClearColor(0.0, 0.0, 0.0, 1.0);
        glad_glClear(GL_COLOR_BUFFER_BIT);

        glad_glUseProgram(g_renderState.shaderProgram);
        
        updateUniforms(&uniforms, g_threadData.window);
        quad.render();

        glfwSwapBuffers(g_threadData.window);
        Sleep(g_perfConfig.getRenderSleepTime());
    }

    quad.cleanup();
    glad_glDeleteProgram(g_renderState.shaderProgram);
    
    return 0;
}

// Example usage in main function
function mainWithStructs() => void {
    let windowConfig = WindowConfig{800, 600, "Structured Star Field"};
    
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return;
    }

    g_threadData.window = initializeWindow(&windowConfig);
    if (g_threadData.window == nullptr) {
        printf("Failed to create window\n");
        glfwTerminate();
        return;
    }

    // Create render thread
    g_threadData.renderThread = CreateThread(nullptr, 0, renderThreadFunctionWithStructs, nullptr, 0, &g_threadData.renderThreadId);
    
    // Main event loop
    while (!glfwWindowShouldClose(g_threadData.window)) {
        glfwPollEvents();
        Sleep(g_perfConfig.getEventSleepTime());
    }

    g_renderState.shouldExit = 1;
    WaitForSingleObject(g_threadData.renderThread, INFINITE);
    CloseHandle(g_threadData.renderThread);
    
    glfwTerminate();
}