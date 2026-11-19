// browser/main.os  -- shell: window + input loop.   Run:  Osengine.exe browser/main.os
// Keys: Enter in address bar = go, Ctrl+L focus bar, F5 reload, F12 inspector, Up/Down scroll, Esc quit.
module C = import "standard/1/Canvas.os";
module B = import "browser/browser.os";
module R = import "browser/paint/backend.os";
module CH = import "browser/chrome.os";
module INS = import "browser/inspector.os";

let START: char* = "browser/test/index.html";

function main() => int {
    let app: C.Canvas;
    if (!app.init()) return 2;
    let win = app.createWindow("OmniBrowser", 1000, 700);
    if (win == nullptr) { app.quit(); return 3; }
    let pen = win.context2d();

    let b: B.Browser;
    let chrome: CH.Chrome;
    chrome.init(pen);
    let ins: INS.Inspector;
    b.navigate(START);                                          // on failure the page stays blank; see console

    let top: float = CH.height();
    while (app.frame()) {
        let ms: double = app.time() * 1000.0;
        let w: float = win.width() as float;
        let h: float = win.height() as float;
        let viewH: float = h - top;

        if (win.keyPressed(256)) win.close();                   // Esc
        if (win.keyPressed(301)) { if (ins.isOpen()) ins.close(); else ins.open(&app); }   // F12

        chrome.update(win, pen, &b, w);

        if (!chrome.addr.focused) {
            if (win.keyDown(264)) b.scrollY = b.scrollY + 8.0;  // Down
            if (win.keyDown(265)) b.scrollY = b.scrollY - 8.0;  // Up
        }
        let maxS: float = b.pageH - viewH;
        if (maxS < 0.0) maxS = 0.0;
        if (b.scrollY > maxS) b.scrollY = maxS;
        if (b.scrollY < 0.0) b.scrollY = 0.0;

        if (win.mousePressed() && !pen.wantsMouse && win.my >= top) {
            let hit = b.pick(win.mx, win.my - top);
            if (hit != nullptr && ins.isOpen()) ins.selected = hit;
            b.click(win.mx, win.my - top, ms);
        }

        b.tick(ms);
        b.refresh(w);
        R.render(pen, b.list, b.scrollY, top, w, viewH);
        ins.highlight(pen, top, b.scrollY);
        chrome.paintBackground(pen, w);
        ins.update(&b);
        app.present();
    }
    app.quit();
    return 0;
}
