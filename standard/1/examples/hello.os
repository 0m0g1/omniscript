// examples/app/hello.os -- one Canvas, four windows on three backends: GL 2D app (Gui + 2D shader),
// GL 3D scene with a 2D HUD, a Vulkan 2D window and a software-rasterised window. The same Pen2D/Gui code
// runs on all of them. The Vulkan window needs shaders/*.spv (run from the folder that contains shaders/).
// Run from the standard/1 folder. Not compiled yet.
module C = import "standard/1/Canvas.os";
module Ui = import "standard/1/Gui.os";
module OGL = import "standard/1/OpenGL.os";

function main() => int {
    let app: C.Canvas;
    if (!app.init()) return 1;

    // window A: plain 2D app
    let win: C.Win* = app.createWindow("My App", 800, 600);
    if (win == nullptr) return 2;
    let pen: C.Pen2D* = win.context2d();
    let ui: Ui.Gui; ui.init(pen, win);
    ui.setPanel(16.0, 16.0, 300.0);

    // a 2D fragment shader: plasma
    let plasma: u32 = pen.createShaderBoth("void main() { vec2 p = vUV * 6.0; float v = sin(p.x + uTime) + sin(p.y * 1.3 - uTime) + sin((p.x + p.y) * 0.7 + uTime * 0.5); o = vec4(0.5 + 0.5 * sin(v), 0.5 + 0.5 * sin(v + 2.0), 0.5 + 0.5 * sin(v + 4.0), 1.0); }", "shaders/plasma.frag.spv");

    // window B: 3D scene + HUD
    let view: C.Win* = app.createWindow("3D view", 640, 480);
    let scene: C.Pen3D* = view.context3d();
    let hud: C.Pen2D* = view.context2d();
    let cube: OGL.Mesh;
    scene.createCube(&cube);
    let model = C.CMem.malloc(64) as float*;
    let rot = C.CMem.malloc(64) as float*;
    let hudUi: Ui.Gui; hudUi.init(hud, view);
    hudUi.setPanel(12.0, 12.0, 200.0);

    // window C: Vulkan (skipped if Vulkan is unavailable). Gui.os/Canvas calls are identical to the GL window.
    let vkw: C.Win* = app.createWindowEx("Vulkan 2D", 480, 320, C.API_VULKAN);
    let vkPen: C.Pen2D* = nullptr;
    let vkUi: Ui.Gui; 
    let vkPlasma: u32 = 0;
    if (vkw != nullptr) {
        vkPen = vkw.context2d();
        vkUi.init(vkPen, vkw); vkUi.setPanel(12.0, 12.0, 220.0);
        vkPlasma = vkPen.createShaderBoth("", "shaders/plasma.frag.spv");
    }
    // window D: software rasteriser shown through a GL blit (custom shaders are skipped here)
    let sww: C.Win* = app.createWindowEx("Software 2D", 480, 320, C.API_SOFTWARE);
    let swPen: C.Pen2D* = nullptr;
    let swUi: Ui.Gui;
    if (sww != nullptr) { swPen = sww.context2d(); swUi.init(swPen, sww); swUi.setPanel(12.0, 12.0, 220.0); }

    let clicks: int = 0;
    let volume: float = 0.5;
    let speed: float = 1.0;
    let fancy: bool = true;
    let name = C.CMem.malloc(64) as char*;
    name[0] = 0 as char;
    let angle: float = 0.0;

    while (app.frame()) {
        if (win.isOpen) {
            pen.clear(0.1, 0.1, 0.12, 1.0);
            if (fancy) pen.shaderRect(plasma, 330.0, 16.0, 450.0, 220.0);
            pen.circle(560.0, 400.0, 60.0, 0x5B7CFAFF);
            pen.line(330.0, 300.0, 780.0, 560.0, 3.0, 0xFFB454FF);
            ui.begin();
            ui.header("Controls");
            ui.label("Hello from Canvas");
            if (ui.button("Click me")) clicks = clicks + 1;
            ui.slider("Volume", &volume, 0.0, 1.0);
            ui.slider("Speed", &speed, 0.0, 4.0);
            ui.checkbox("Plasma", &fancy);
            ui.textInput("Name", name, 64);
            ui.separator();
            ui.dim("clicks / name:");
            ui.label(name);
            ui.end();
            if (win.keyPressed(W_ESC())) win.close();
        }

        if (view.isOpen) {
            angle = angle + 0.016 * speed;
            scene.clear(0.05, 0.06, 0.09, 1.0);
            scene.camera(2.5, 2.0, 3.5, 0.0, 0.0, 0.0, 60.0);
            C.matRotateY(model, angle);
            C.matRotateX(rot, angle * 0.5);
            let m2 = C.CMem.malloc(64) as float*;
            C.matMul(m2, model, rot);
            scene.draw(&cube, m2, 0x5B9CFAFF);
            C.CMem.free(m2 as void*);
            hudUi.begin();
            hudUi.label("3D HUD");
            hudUi.slider("Speed", &speed, 0.0, 4.0);
            hudUi.end();
        }

        if (vkw != nullptr && vkw.isOpen) {
            vkPen.clear(0.08, 0.1, 0.16, 1.0);
            vkPen.shaderRect(vkPlasma, 240.0, 12.0, 220.0, 150.0);
            vkUi.begin();
            vkUi.header("Vulkan");
            if (vkUi.button("Click me")) clicks = clicks + 1;
            vkUi.slider("Volume", &volume, 0.0, 1.0);
            vkUi.end();
        }
        if (sww != nullptr && sww.isOpen) {
            swPen.clear(0.14, 0.1, 0.1, 1.0);
            swPen.circle(380.0, 200.0, 50.0, 0xFFB454FF);
            swUi.begin();
            swUi.header("Software");
            if (swUi.button("Click me")) clicks = clicks + 1;
            swUi.slider("Volume", &volume, 0.0, 1.0);
            swUi.end();
        }
        app.present();
    }
    app.quit();
    return 0;
}

function W_ESC() => int { return 256; }
