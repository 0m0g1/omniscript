// browser/main.os  -- shell: window + Ui + frame loop.   Run:  Osengine.exe browser/main.os
// Keys: Enter in the address bar = go, F5 reload, F12 or the Inspect button = inspector, Up/Down/PgUp/PgDn scroll, Esc quit.
module C = import "standard/1/Canvas.os";
module UI = import "standard/1/Ui.os";
module B = import "browser.os";
module CH = import "chrome.os";
module INS = import "inspector.os";
module G = import "standard/1/c/glfw.os";

let START: char* = "browser/test/index.html";
// Windows runs its own message loop while you drag a window edge, so glfwPollEvents() does not return and the main
// loop below stalls: the newly exposed area stays black until you let go. GLFW still calls these two callbacks during
// the drag, so each draws one full frame. (Same pattern as Http.onWrite: plain functions passed as `fn as void*`.)
let g_win: C.Win* = nullptr;
let g_ui: UI.Ui* = nullptr;
let g_chrome: CH.Chrome* = nullptr;
let g_busy: bool = false;

function redrawNow() => void {
    if (g_win == nullptr || g_ui == nullptr || g_chrome == nullptr || g_busy) return;
    if (!g_win.isOpen) return;
    g_busy = true;
    g_win.beginFrame();
    g_chrome.fit(g_win);
    g_ui.frame();
    g_win.endFrame();
    g_busy = false;
}
function onRefresh(w: void*) => void { redrawNow(); }
function onFbSize(w: void*, width: int, height: int) => void { redrawNow(); }

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
    g_win = win; g_ui = &ui; g_chrome = &chrome;
    G.glfwSetWindowRefreshCallback(win.win.h, onRefresh as void*);
    G.glfwSetFramebufferSizeCallback(win.win.h, onFbSize as void*);
    let b: B.Browser;
    b.page = chrome.page;
    b.pen = ui.pen;                                             // without this the browser never fetches <img>/<svg> (S_IMG is skipped)
    b.startWorkers();                                           // image fetch + decode run on worker threads (before the first navigate)
    let ins: INS.Inspector;
    let recording: bool = false;
    b.viewW = win.width() as int; b.viewH = win.height() as int;   // @media is evaluated against the window
    let lastW: int = b.viewW; let lastH: int = b.viewH;
    let settle: int = 0;                                        // frames the size has been stable but not yet applied
    b.navigate(START);                                          // queued; loads stage by stage in b.step()

    while (app.frame()) {
        let ms: double = app.time() * 1000.0;
        if (win.keyPressed(256)) win.close();                   // Esc

        let insOpen: bool = ins.isOpen();
        if (insOpen != recording) {                              // CSS diagnostics only cost anything while the inspector is open
            recording = insOpen;
            ui.recordStyles = insOpen;
            root.markStyle();                                    // re-cascade so every node gets its notes
        }
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
        let subForm: UI.Node* = nullptr;                        // form to submit this frame (one at most)
        while (ui.poll(&ev)) {
            if (ev.kind == UI.EV_CLICK) {
                clicked = true;
                if (ins.isOpen()) ins.select(ev.node);
                let sf: UI.Node* = B.submitterForm(ev.node);    // <button> inside a <form>
                if (sf != nullptr) subForm = sf;
            }
            if (ev.kind == UI.EV_SUBMIT) {                      // Enter in a field, or <input type=submit>
                let f2: UI.Node* = B.formAncestor(ev.node);
                if (f2 != nullptr) subForm = f2;
            }
        }
        chrome.update(win, &b);
        if (clicked) b.handleClicks(ms);
        if (subForm != nullptr) b.submitForm(subForm);

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

        // re-evaluate @media once the window size has stopped changing for a few frames
        let nowW: int = win.width() as int;
        let nowH: int = win.height() as int;
        if (nowW != lastW || nowH != lastH) { lastW = nowW; lastH = nowH; settle = 0; }
        else if (nowW > 0 && nowH > 0 && (nowW != b.viewW || nowH != b.viewH)) {   // 0 x 0 = minimized
            settle = settle + 1;
            if (settle >= 8) { if (b.resizeView(nowW, nowH)) settle = 0; }
        }

        // CSS queued by the loader is applied here, where the UI.Ui lives
        if (b.cssReset) { ui.clearCss(); b.cssReset = false; }
        let q: int = 0;
        while (q < b.qN) { ui.addCss(b.cssQ[q]); q = q + 1; }
        b.qN = 0;
    }
    g_win = nullptr; g_ui = nullptr; g_chrome = nullptr;           // no callback may touch them past this point
    b.shutdown();                                               // stop the image workers before the Ui goes away
    ui.destroy();
    app.quit();
    return 0;
}
