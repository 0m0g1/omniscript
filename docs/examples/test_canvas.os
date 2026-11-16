// test_window.os
module C = import "standard/1/Canvas.os";

function main() => int {
    let app: C.Canvas;
    if (!app.init()) return 1;

    let win: C.Win* = app.createWindow("Omniscript window test", 800, 600);
    if (win == nullptr) { app.quit(); return 2; }

    let pen: C.Pen2D* = win.context2d();

    while (app.frame()) {
        if (win.keyPressed(256)) win.close();      // Esc closes

        pen.clear(0.1, 0.1, 0.12, 1.0);
        pen.fillRect(50.0, 50.0, 200.0, 100.0, 0x3399FFFF);
        pen.circle(win.mx, win.my, 20.0, 0xFF6633FF);   // follows the mouse
        pen.text(60.0, 70.0, "Hello, Canvas!", 0xFFFFFFFF);

        app.present();
    }

    app.quit();
    return 0;
}