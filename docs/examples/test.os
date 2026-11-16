module C = import "standard/1/Canvas.os";
module Console = import "standard/1/Console.os";

let window = {
    width: 800,
    height: 600
}

function main() => int {
    let app: C.Canvas;
    if (!app.init()) return 1;

    let win = app.createWindow("Omniscript Window", 800, 600);

    if (win == nullptr) {app.quit(); return 2; }

    let pen: C.Pen2D* = win.context2d();

    while (app.frame()) {
        if (win.keyPressed(256)) win.close();

        pen.text(window.width/2, window.height/2, "Hello, Canvas!", 0xFFFFFFFF);
        Console.log(window.width/2)
        Console.log(window.height/2)
        app.present();
    }

    return 0;
}