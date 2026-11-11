// standard/1/c/opengl.os  (OpenGL 1.1 symbols from opengl32; newer via glfwGetProcAddress)
extern "C" fn glClearColor(r: float, g: float, b: float, a: float) => void;
extern "C" fn glClear(mask: u32) => void;
extern "C" fn glViewport(x: int, y: int, w: int, h: int) => void;
extern "C" fn glEnable(cap: u32) => void;
extern "C" fn glDisable(cap: u32) => void;
extern "C" fn glBlendFunc(s: u32, d: u32) => void;
extern "C" fn glDrawArrays(mode: u32, first: int, count: int) => void;
extern "C" fn glDrawElements(mode: u32, count: int, kind: u32, idx: void*) => void;
extern "C" fn glGenTextures(n: int, ids: u32*) => void;
extern "C" fn glBindTexture(t: u32, id: u32) => void;
extern "C" fn glTexImage2D(t: u32, lvl: int, ifmt: int, w: int, h: int, border: int, fmt: u32, kind: u32, px: void*) => void;
extern "C" fn glTexParameteri(t: u32, p: u32, v: int) => void;
extern "C" fn glGetError() => u32;
// glCreateShader, glShaderSource, glCompileShader, glCreateProgram, glLinkProgram, glUseProgram,
// glGenBuffers, glBindBuffer, glBufferData, glVertexAttribPointer, glEnableVertexAttribArray,
// glGenVertexArrays, glBindVertexArray, glUniform*: load with glfwGetProcAddress (see OpenGL.os).
