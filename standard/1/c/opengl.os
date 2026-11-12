// standard/1/c/opengl.os  (OpenGL 1.1 symbols from opengl32; newer via glfwGetProcAddress, see OpenGL.os)
extern "C", "opengl32.dll" fn glClearColor(r: float, g: float, b: float, a: float) => void;
extern "C", "opengl32.dll" fn glClear(mask: u32) => void;
extern "C", "opengl32.dll" fn glViewport(x: int, y: int, w: int, h: int) => void;
extern "C", "opengl32.dll" fn glScissor(x: int, y: int, w: int, h: int) => void;
extern "C", "opengl32.dll" fn glEnable(cap: u32) => void;
extern "C", "opengl32.dll" fn glDisable(cap: u32) => void;
extern "C", "opengl32.dll" fn glBlendFunc(s: u32, d: u32) => void;
extern "C", "opengl32.dll" fn glCullFace(mode: u32) => void;
extern "C", "opengl32.dll" fn glFrontFace(mode: u32) => void;
extern "C", "opengl32.dll" fn glDepthFunc(f: u32) => void;
extern "C", "opengl32.dll" fn glDepthMask(on: u8) => void;
extern "C", "opengl32.dll" fn glPixelStorei(p: u32, v: int) => void;
extern "C", "opengl32.dll" fn glReadPixels(x: int, y: int, w: int, h: int, fmt: u32, kind: u32, px: void*) => void;
extern "C", "opengl32.dll" fn glDrawArrays(mode: u32, first: int, count: int) => void;
extern "C", "opengl32.dll" fn glDrawElements(mode: u32, count: int, kind: u32, idx: void*) => void;
extern "C", "opengl32.dll" fn glGenTextures(n: int, ids: u32*) => void;
extern "C", "opengl32.dll" fn glDeleteTextures(n: int, ids: u32*) => void;
extern "C", "opengl32.dll" fn glBindTexture(t: u32, id: u32) => void;
extern "C", "opengl32.dll" fn glTexImage2D(t: u32, lvl: int, ifmt: int, w: int, h: int, border: int, fmt: u32, kind: u32, px: void*) => void;
extern "C", "opengl32.dll" fn glTexSubImage2D(t: u32, lvl: int, x: int, y: int, w: int, h: int, fmt: u32, kind: u32, px: void*) => void;
extern "C", "opengl32.dll" fn glTexParameteri(t: u32, p: u32, v: int) => void;
extern "C", "opengl32.dll" fn glGetString(name: u32) => char*;
extern "C", "opengl32.dll" fn glGetError() => u32;
extern "C", "opengl32.dll" fn glFlush() => void;
// GL 2.0+ (shaders, buffers, VAOs, FBOs, glUniform*, glActiveTexture...): loaded in OpenGL.os via glfwGetProcAddress.
