module C = import "standard/1/Canvas.os";
module Console = import "standard/1/Console.os";

let window = {
    width: 800,
    height: 600
}

function main() => int {
    let app: C.Canvas;
    
    if (!app.init()) return 1;
    
    app.iconPath = "docs/examples/icon.png";

    let win = app.createWindow("omniscript window", window.width, window.height);

    if (win == nullptr) {app.quit(); return 2;}

    let pen = win.context2d();

    while (app.frame()) {
        if (win.keyPressed(256)) app.quit();
        pen.textScaled(window.width / 2 - 50, window.height / 2 - 50, "0m0g1", 0xFFFFFF, 3);
        app.present();
    }

    return 0;
}