// browser/main.os  -- shell: window + Ui + frame loop.   Run:  Osengine.exe browser/main.os
// Keys: Enter in the address bar = go, F5 reload, F12 or the Inspect button = inspector, Up/Down/PgUp/PgDn scroll, Esc quit.
module C = import "standard/1/Canvas.os";
module UI = import "standard/1/Ui.os";
module B = import "browser/browser.os";
module CH = import "browser/chrome.os";
module INS = import "browser/inspector.os";

let START: char* = "browser/test/index.html";
let WHEEL_STEP: float = 60.0;                                   // pixels per wheel notch

function main() => int {
    let app: C.Canvas;
    if (!app.init()) return 2;
    let win = app.createWindow("OmniBrowser", 1000, 700);
    if (win == nullptr) { app.quit(); return 3; }

    let ui: UI.Ui;
    ui.init(win);
    let chrome: CH.Chrome;
    let root: UI.Node* = ui.root();
    chrome.init(root);
    let b: B.Browser;
    b.page = chrome.page;
    let ins: INS.Inspector;
    b.navigate(START);                                          // queued; loads stage by stage in b.step()

    while (app.frame()) {
        let ms: double = app.time() * 1000.0;
        if (win.keyPressed(256)) win.close();                   // Esc

        chrome.fit(win);                                        // page height = window - toolbar - status line (before the frame, so a resize lays out at once)
        ui.frame();                                             // events -> cascade -> layout -> paint -> backend

        let wy: float = win.wheelY();                           // notches this frame, + = rolled up
        let wx: float = win.wheelX();                           // sideways wheel / touchpad swipe, + = content moves left
        if (win.shiftDown() && wy != 0.0 && wx == 0.0) { wx = 0.0 - wy; wy = 0.0; }   // Shift+wheel scrolls sideways
        if (wx != 0.0) {
            let movedX: bool = ui.wheelX(wx * WHEEL_STEP);
            if (!movedX) chrome.page.scrollByX(wx * WHEEL_STEP);
        }
        if (wy != 0.0) {
            let moved: bool = ui.wheel(0.0 - wy * WHEEL_STEP);
            if (!moved) chrome.page.scrollBy(0.0 - wy * WHEEL_STEP);   // nothing under the mouse scrolls: scroll the page itself
        }

        let ev: UI.Event;
        let clicked: bool = false;
        while (ui.poll(&ev)) {
            if (ev.kind == UI.EV_CLICK) {
                clicked = true;
                if (ins.isOpen()) ins.select(ev.node);
            }
        }
        chrome.update(win, &b);
        if (clicked) b.handleClicks(ms);

        if (!chrome.addr.focus) {
            if (win.keyDown(264)) chrome.page.scrollBy(8.0);    // Down
            if (win.keyDown(265)) chrome.page.scrollBy(0.0 - 8.0);   // Up
            if (win.keyRepeat(267)) chrome.page.scrollBy(300.0);     // PgDn
            if (win.keyRepeat(266)) chrome.page.scrollBy(0.0 - 300.0);   // PgUp
        }
        if (win.keyPressed(301) || chrome.inspect.pressed) { if (ins.isOpen()) ins.close(); else ins.open(&app); }

        b.tick(ms);
        ins.update(&b);
        app.present();
        b.step(app.time() * 1000.0);                            // after present: the status of the coming stage is already visible

        // CSS queued by the loader is applied here, where the UI.Ui lives
        if (b.cssReset) { ui.clearCss(); b.cssReset = false; }
        let q: int = 0;
        while (q < b.qN) { ui.addCss(b.cssQ[q]); q = q + 1; }
        b.qN = 0;
    }
    ui.destroy();
    app.quit();
    return 0;
}
