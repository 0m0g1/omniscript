// standard/1/Canvas/Win.os
// ---------------------------------------------------------------------------------------------
// Win: native window + backend + pens + input
// ---------------------------------------------------------------------------------------------

// ASCII for a GLFW printable key code (0 if none), honouring shift on a US layout.
function keyToChar(k: int, shift: bool) => int {
    let r: int = 0;
    if (k >= 65 && k <= 90) { r = k + 32; if (shift) r = k; }
    else if (k >= 48 && k <= 57) {
        r = k;
        if (shift) {
            if (k == 48) r = 41; if (k == 49) r = 33; if (k == 50) r = 64; if (k == 51) r = 35; if (k == 52) r = 36;
            if (k == 53) r = 37; if (k == 54) r = 94; if (k == 55) r = 38; if (k == 56) r = 42; if (k == 57) r = 40;
        }
    }
    else if (k == 32) r = 32;
    else if (k == 39) { r = 39; if (shift) r = 34; }
    else if (k == 44) { r = 44; if (shift) r = 60; }
    else if (k == 45) { r = 45; if (shift) r = 95; }
    else if (k == 46) { r = 46; if (shift) r = 62; }
    else if (k == 47) { r = 47; if (shift) r = 63; }
    else if (k == 59) { r = 59; if (shift) r = 58; }
    else if (k == 61) { r = 61; if (shift) r = 43; }
    else if (k == 91) { r = 91; if (shift) r = 123; }
    else if (k == 92) { r = 92; if (shift) r = 124; }
    else if (k == 93) { r = 93; if (shift) r = 125; }
    else if (k == 96) { r = 96; if (shift) r = 126; }
    return r;
}

struct Win {
    win: W.Window;
    pen: Pen2D;
    scene: Pen3D;
    api: int = 0;
    vk: VK.Vulkan;                      // Vulkan windows: this window's presentation target (use it for your own 3D)
    blit: Pen2D;                        // software windows: GL pen that shows the CPU buffer
    blitTex: OGL.Texture;
    blitW: int = 0; blitH: int = 0;
    isOpen: bool = false;
    active: bool = false;               // false while minimised: skip heavy work
    fbW: int = 0; fbH: int = 0;
    // mouse (framebuffer pixel coordinates; same space the pens draw in)
    mx: float = 0.0; my: float = 0.0;
    ldown: bool = false; lprev: bool = false;
    rdown: bool = false; rprev: bool = false;
    // keyboard
    keyNow: u8* = nullptr; keyPrev: u8* = nullptr;
    typed: char* = nullptr; ntyped: int = 0;     // characters typed this frame (NUL-terminated)
    keyEv: u8* = nullptr;               // per key: 1 on the frame it is pressed and on each auto-repeat while held
    keyNext: double* = nullptr;         // per key: time of the next auto-repeat
    lastClickT: double = 0.0 - 1.0; dblClick: bool = false;
    lastClickX: float = 0.0; lastClickY: float = 0.0;
    curKind: int = 0;                   // cursor shape currently set (W.CURSOR_*)
    wheelH: float = 0.0;                // horizontal wheel notches this frame (+ = content moves left)
    wheel: float = 0.0;                 // scroll-wheel notches this frame (+ = rolled up / away from you); 0 when idle
    keepFrame: bool = false;            // false (default): the window is cleared to pen.clear's colour at the start of every frame

    // core: Canvas-owned Vulkan device holder (zeroed; first Vulkan window fills it). share: window to share GL
    // objects with (or nullptr). dir: SPIR-V folder for the Vulkan backend.
    init(title: char*, w: int, hh: int, apiWanted: int, core: VK.Vulkan*, share: W.Window*, dir: char*, validate: bool) => bool {
        this.api = apiWanted;
        let ok: bool = false;
        if (apiWanted == API_VULKAN) { ok = this.initVulkanWin(title, w, hh, core, dir, validate); }
        else { ok = this.initGLWin(title, w, hh, share, apiWanted == API_SOFTWARE); }
        if (!ok) return false;
        this.keyNow = CMem.malloc(KEY_COUNT as u64) as u8*;
        this.keyPrev = CMem.malloc(KEY_COUNT as u64) as u8*;
        CMem.memset(this.keyNow as void*, 0, KEY_COUNT as u64);
        CMem.memset(this.keyPrev as void*, 0, KEY_COUNT as u64);
        this.keyEv = CMem.malloc(KEY_COUNT as u64) as u8*;
        this.keyNext = CMem.malloc((KEY_COUNT * 8) as u64) as double*;
        CMem.memset(this.keyEv as void*, 0, KEY_COUNT as u64);
        CMem.memset(this.keyNext as void*, 0, (KEY_COUNT * 8) as u64);
        this.lastClickT = 0.0 - 1.0; this.dblClick = false;
        this.win.hookWheel();
        this.typed = CMem.malloc(32) as char*;
        this.typed[0] = 0 as char;
        this.isOpen = true;
        return true;
    }

    initGLWin(title: char*, w: int, hh: int, share: W.Window*, software: bool) => bool {
        let opened: bool = false;
        if (share != nullptr) { opened = this.win.openGLShared(w, hh, title, share); }
        else { opened = this.win.openGL(w, hh, title); }
        if (!opened) return false;
        this.win.vsync(true);
        if (!OGL.isLoaded()) OGL.load();
        if (!OGL.isLoaded()) { this.win.close(); return false; }
        if (software) {
            if (!this.blit.init(this.win.h)) { this.win.close(); return false; }
            this.pen.initSoftware(w, hh);
            return true;
        }
        if (!this.pen.init(this.win.h)) { this.win.close(); return false; }
        if (!this.scene.init(this.win.h)) { this.pen.free(); this.win.close(); return false; }
        return true;
    }

    initVulkanWin(title: char*, w: int, hh: int, core: VK.Vulkan*, dir: char*, validate: bool) => bool {
        if (!this.win.openVulkan(w, hh, title)) return false;
        if (core.device == nullptr) {
            core.owner = true; core.vsync = true;
            if (!core.initCore(this.win.h, "OmniScript", validate)) { this.win.close(); return false; }
        }
        this.vk.vsync = true;
        if (!this.vk.attach(core, this.win.h)) { this.vk.free(); this.win.close(); return false; }
        if (!this.pen.initVulkan(&this.vk, dir)) { this.pen.free(); this.vk.free(); this.win.close(); return false; }
        return true;
    }

    context2d() => Pen2D* { return &this.pen; }
    context3d() => Pen3D* { return &this.scene; }

    setTitle(t: char*) => void { this.win.setTitle(t); }
    close() => void { this.win.requestClose(); }

    // ---- input queries (valid after frame() started) ----
    mouseDown() => bool { return this.ldown; }
    mousePressed() => bool { return this.ldown && !this.lprev; }
    mouseReleased() => bool { return !this.ldown && this.lprev; }
    rightDown() => bool { return this.rdown; }
    rightPressed() => bool { return this.rdown && !this.rprev; }
    keyDown(k: int) => bool { return this.keyNow[k] != 0; }
    keyPressed(k: int) => bool { return this.keyNow[k] != 0 && this.keyPrev[k] == 0; }
    shiftDown() => bool { return this.keyNow[W.KEY_LEFT_SHIFT] != 0 || this.keyNow[344] != 0; }
    ctrlDown() => bool { return this.keyNow[W.KEY_LEFT_CONTROL] != 0 || this.keyNow[345] != 0; }
    // True on the frame a key goes down and on every auto-repeat while it is held.
    keyRepeat(k: int) => bool { return this.keyEv[k] != 0; }
    // Backspace with key repeat (fires on press, then repeats while held).
    backspace() => bool { return this.keyEv[W.KEY_BACKSPACE] != 0; }
    altDown() => bool { return this.keyNow[GK_LALT] != 0 || this.keyNow[GK_RALT] != 0; }
    // True on the frame the left button went down for the second time within DBLCLICK_TIME.
    doubleClicked() => bool { return this.dblClick; }
    // Characters typed this frame, NUL-terminated (ASCII only; poll-based, no callback).
    typedText() => char* { return this.typed; }
    // Mouse cursor shape (W.CURSOR_ARROW / IBEAM / HAND / ...). Cheap to call every frame: only a change reaches GLFW.
    setCursor(kind: int) => void {
        if (kind == this.curKind) return;
        this.curKind = kind;
        this.win.setCursor(kind);
    }
    // Wheel notches this frame: positive = scroll up, negative = down. Touchpads give fractions.
    wheelY() => float { return this.wheel; }
    wheelX() => float { return this.wheelH; }
    width() => int { return this.fbW; }
    height() => int { return this.fbH; }

    pollInput() => void {
        let now: double = W.time();
        this.ntyped = 0;
        this.wheel = this.win.takeWheel() as float;
        this.wheelH = this.win.takeWheelX() as float;
        let k: int = KEY_FIRST;
        while (k <= KEY_LAST) {
            this.keyPrev[k] = this.keyNow[k];
            this.keyEv[k] = 0 as u8;
            let d: bool = this.win.keyDown(k);
            if (d) {
                this.keyNow[k] = 1 as u8;
                if (this.keyPrev[k] == 0) { this.keyEv[k] = 1 as u8; this.keyNext[k] = now + REPEAT_DELAY; }
                else if (now >= this.keyNext[k]) { this.keyEv[k] = 1 as u8; this.keyNext[k] = now + REPEAT_RATE; }
            } else { this.keyNow[k] = 0 as u8; }
            k = k + 1;
        }
        // printable keys (with auto-repeat); Ctrl/Alt chords are shortcuts, not text
        let shift: bool = this.shiftDown();
        let chord: bool = this.ctrlDown() || this.altDown();
        k = 32;
        while (k <= 96) {
            if (!chord && this.keyEv[k] != 0 && this.ntyped < 30) {
                let c: int = keyToChar(k, shift);
                if (c != 0) { this.typed[this.ntyped] = c as char; this.ntyped = this.ntyped + 1; }
            }
            k = k + 1;
        }
        this.typed[this.ntyped] = 0 as char;

        this.lprev = this.ldown; this.rprev = this.rdown;
        this.ldown = this.win.mouseDown(W.MOUSE_LEFT);
        this.rdown = this.win.mouseDown(W.MOUSE_RIGHT);
        let cx: double = 0.0; let cy: double = 0.0;
        this.win.cursor(&cx, &cy);
        let ww: int = 0; let wh: int = 0;
        this.win.size(&ww, &wh);
        let sx: float = 1.0; let sy: float = 1.0;
        if (ww > 0) sx = (this.fbW as float) / (ww as float);
        if (wh > 0) sy = (this.fbH as float) / (wh as float);
        this.mx = (cx as float) * sx;
        this.my = (cy as float) * sy;

        // double click: two presses within DBLCLICK_TIME and a few pixels of each other
        this.dblClick = false;
        if (this.mousePressed()) {
            let ddx: float = this.mx - this.lastClickX; if (ddx < 0.0) ddx = 0.0 - ddx;
            let ddy: float = this.my - this.lastClickY; if (ddy < 0.0) ddy = 0.0 - ddy;
            if (now - this.lastClickT < DBLCLICK_TIME && ddx < 6.0 && ddy < 6.0) {
                this.dblClick = true;
                this.lastClickT = 0.0 - 1.0;          // a third click starts a new sequence
            } else {
                this.lastClickT = now; this.lastClickX = this.mx; this.lastClickY = this.my;
            }
        }
    }

    beginFrame() => void {
        if (this.api != API_VULKAN) this.win.makeCurrent();
        let w: int = 0; let hh: int = 0;
        this.win.framebufferSize(&w, &hh);
        this.fbW = w; this.fbH = hh;
        this.active = w > 0 && hh > 0;
        this.pollInput();
        this.pen.inp.mx = this.mx; this.pen.inp.my = this.my;
        this.pen.inp.down = this.ldown;
        this.pen.inp.pressed = this.mousePressed();
        this.pen.inp.released = this.mouseReleased();
        this.pen.inp.typed = this.typed;
        let ctrl: bool = this.ctrlDown();
        this.pen.inp.shift = this.shiftDown();
        this.pen.inp.ctrl = ctrl;
        this.pen.inp.backspace = this.keyRepeat(W.KEY_BACKSPACE);
        this.pen.inp.kDel = this.keyRepeat(GK_DELETE);
        this.pen.inp.left = this.keyRepeat(GK_LEFT);
        this.pen.inp.right = this.keyRepeat(GK_RIGHT);
        this.pen.inp.up = this.keyRepeat(GK_UP);
        this.pen.inp.kDown = this.keyRepeat(GK_DOWN);
        this.pen.inp.kHome = this.keyRepeat(GK_HOME);
        this.pen.inp.kEnd = this.keyRepeat(GK_END);
        this.pen.inp.tab = this.keyRepeat(GK_TAB);
        this.pen.inp.esc = this.keyPressed(GK_ESC);
        this.pen.inp.enter = this.keyPressed(W.KEY_ENTER) || this.keyPressed(GK_KPENTER);
        this.pen.inp.space = this.keyPressed(32);
        this.pen.inp.selAll = ctrl && this.keyPressed(GK_A);
        this.pen.inp.copy = ctrl && this.keyPressed(GK_C);
        this.pen.inp.cut = ctrl && this.keyRepeat(GK_X);
        this.pen.inp.paste = ctrl && this.keyRepeat(GK_V);
        this.pen.inp.dbl = this.dblClick;
        this.pen.inp.time = W.time();
        this.pen.updateWidgets();
        this.pen.begin(w, hh);
        // keepFrame == false: start every frame from the clear colour. Without this the GL back buffers keep
        // whatever was drawn two frames ago and everything redrawn at a new spot looks like it is vibrating.
        if (!this.keepFrame && this.active) this.pen.clear(this.pen.clearR, this.pen.clearG, this.pen.clearB, this.pen.clearA);
        this.scene.size(w, hh);
    }

    endFrame() => void {
        this.pen.drawWidgets();                 // widgets land on top of everything drawn this frame
        if (this.api == API_VULKAN) { this.pen.flush(); return; }       // flush records, submits and presents
        this.win.makeCurrent();
        this.pen.flush();
        if (this.api == API_SOFTWARE) this.blitSoftware();
        this.win.swap();
    }

    // Uploads the CPU framebuffer to a GL texture and draws it 1:1 through the blit pen.
    blitSoftware() => void {
        let w: int = this.pen.pixelWidth(); let hh: int = this.pen.pixelHeight();
        if (w <= 0 || hh <= 0 || this.fbW != w || this.fbH != hh) return;
        if (this.blitW != w || this.blitH != hh) {
            if (this.blitTex.id != 0) this.blitTex.free();
            this.blitTex.fromRGBA(this.pen.pixels(), w, hh, false);
            this.blitW = w; this.blitH = hh;
        } else {
            GL.glBindTexture(OGL.TEXTURE_2D, this.blitTex.id);
            GL.glTexSubImage2D(OGL.TEXTURE_2D, 0, 0, 0, w, hh, OGL.RGBA, OGL.UNSIGNED_BYTE, this.pen.pixels() as void*);
        }
        this.blit.begin(w, hh);
        this.blit.image(this.blitTex.id, 0.0, 0.0, w as float, hh as float);
        this.blit.flush();
    }

    destroy() => void {
        if (!this.isOpen) return;
        if (this.api == API_VULKAN) {
            this.pen.free();              // waits for the device, destroys buffers/pipelines/textures
            this.vk.free();               // this window's swapchain, surface, sync (device stays with the core)
        } else {
            this.win.makeCurrent();
            if (this.api == API_SOFTWARE) { if (this.blitTex.id != 0) this.blitTex.free(); this.blit.free(); }
            this.pen.free();
            this.scene.free();
        }
        this.win.close();
        if (this.keyNow != nullptr) { CMem.free(this.keyNow as void*); this.keyNow = nullptr; }
        if (this.keyPrev != nullptr) { CMem.free(this.keyPrev as void*); this.keyPrev = nullptr; }
        if (this.keyEv != nullptr) { CMem.free(this.keyEv as void*); this.keyEv = nullptr; }
        if (this.keyNext != nullptr) { CMem.free(this.keyNext as void*); this.keyNext = nullptr; }
        if (this.typed != nullptr) { CMem.free(this.typed as void*); this.typed = nullptr; }
        this.isOpen = false;
    }
}
