// examples/gl/triangle.os: OpenGL 3.3 core, rotating coloured triangle (Window + OpenGL modules).
module W = import "standard/1/Window.os";
module OGL = import "standard/1/OpenGL.os";
module CMem = import "standard/1/c/mem.os";

let VS: char* = "#version 330 core\nlayout(location=0) in vec2 p; layout(location=1) in vec3 c; out vec3 vc; uniform float t;\nvoid main(){ float s=sin(t), k=cos(t); gl_Position=vec4(p.x*k-p.y*s, p.x*s+p.y*k, 0.0, 1.0); vc=c; }";
let FS: char* = "#version 330 core\nin vec3 vc; out vec4 o; void main(){ o=vec4(vc,1.0); }";

function main() => int {
    if (!W.init()) return 1;
    let win: W.Window;
    if (!win.openGL(800, 600, "OmniScript OpenGL")) { W.terminate(); return 2; }
    win.vsync(true);
    OGL.load();
    if (!OGL.isLoaded()) { win.close(); W.terminate(); return 3; }

    let prog: OGL.Program;
    if (!prog.build(VS, FS)) { win.close(); W.terminate(); return 4; }   // OGL.lastError() has the log

    let verts = CMem.malloc(5 * 3 * 4) as float*;
    verts[0] = -0.6; verts[1] = -0.5; verts[2] = 1.0; verts[3] = 0.0; verts[4] = 0.0;
    verts[5] =  0.6; verts[6] = -0.5; verts[7] = 0.0; verts[8] = 1.0; verts[9] = 0.0;
    verts[10] = 0.0; verts[11] = 0.6; verts[12] = 0.0; verts[13] = 0.0; verts[14] = 1.0;
    let layout = CMem.malloc(8) as int*;
    layout[0] = 2; layout[1] = 3;
    let mesh: OGL.Mesh;
    mesh.uploadLayout(verts, 15, layout, 2);

    while (!win.shouldClose()) {
        W.pollEvents();
        if (win.keyDown(W.KEY_ESCAPE)) win.requestClose();
        let w: int = 0; let h: int = 0;
        win.framebufferSize(&w, &h);
        OGL.viewport(w, h);
        OGL.clear(0.05, 0.05, 0.08, 1.0);
        prog.use();
        prog.set1f("t", W.time() as float);
        mesh.draw();
        win.swap();
    }
    mesh.free(); prog.free();
    win.close();
    W.terminate();
    return 0;
}
