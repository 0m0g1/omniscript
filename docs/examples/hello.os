// examples/app/hello.os -- 2D app on Canvas + Gui, plus a 3D window with a 2D HUD and a 2D shader.
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
    let plasma: u32 = pen.createShader("void main() { vec2 p = vUV * 6.0; float v = sin(p.x + uTime) + sin(p.y * 1.3 - uTime) + sin((p.x + p.y) * 0.7 + uTime * 0.5); o = vec4(0.5 + 0.5 * sin(v), 0.5 + 0.5 * sin(v + 2.0), 0.5 + 0.5 * sin(v + 4.0), 1.0); }");

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
        app.present();
    }
    app.quit();
    return 0;
}

function W_ESC() => int { return 256; }
