// ---------------------------------------------------------------------------------------------
// Canvas: app root
// ---------------------------------------------------------------------------------------------
struct Canvas {
    wins: Win** = nullptr;
    count: int = 0;
    started: bool = false;
    vkCore: VK.Vulkan* = nullptr;       // shared Vulkan device, created by the first Vulkan window
    firstGL: Win* = nullptr;            // GL windows share textures/buffers with this one
    shaderDir: char* = "shaders/";      // where canvas2d.{vert,frag}.spv live (Vulkan windows)
    validation: bool = false;           // set before the first Vulkan window to enable the validation layer
    iconPath: char* = nullptr;          // optional: png used as the window/taskbar icon of every window opened afterwards

    init() => bool {
        if (!W.init()) return false;
        this.wins = CMem.malloc((MAX_WINDOWS * 8) as u64) as Win**;
        this.count = 0;
        this.started = true;
        return true;
    }

    // api: API_AUTO (GL, falling back to Vulkan), API_GL, API_VULKAN or API_SOFTWARE. Windows of different
    // backends can coexist. Returns nullptr on failure.
    createWindow(title: char*, w: int, hh: int) => Win* { return this.createWindowEx(title, w, hh, API_AUTO); }

    createWindowEx(title: char*, w: int, hh: int, api: int) => Win* {
        if (!this.started || this.count >= MAX_WINDOWS) return nullptr;
        let want: int = api;
        if (want == API_AUTO) want = API_GL;
        let win: Win* = this.makeWin(title, w, hh, want);
        if (win == nullptr && api == API_AUTO) win = this.makeWin(title, w, hh, API_VULKAN);   // no GL: try Vulkan
        if (win == nullptr) return nullptr;
        if (this.iconPath != nullptr) { if (!win.win.setIconFile(this.iconPath)) Console.log("[icon] could not load icon file (path is relative to the folder you run from)"); }
        this.wins[this.count] = win;
        this.count = this.count + 1;
        if (this.firstGL == nullptr && win.api != API_VULKAN) this.firstGL = win;
        return win;
    }

    makeWin(title: char*, w: int, hh: int, api: int) => Win* {
        if (api == API_VULKAN && this.vkCore == nullptr) {
            this.vkCore = CMem.malloc(sizeof(VK.Vulkan)) as VK.Vulkan*;
            CMem.memset(this.vkCore as void*, 0, sizeof(VK.Vulkan));
        }
        let share: W.Window* = nullptr;
        if (api != API_VULKAN && this.firstGL != nullptr && this.firstGL.isOpen) share = &this.firstGL.win;
        let win = CMem.malloc(sizeof(Win)) as Win*;
        CMem.memset(win as void*, 0, sizeof(Win));
        if (!win.init(title, w, hh, api, this.vkCore, share, this.shaderDir, this.validation)) { CMem.free(win as void*); return nullptr; }
        return win;
    }

    // Polls events, drops windows the user closed, starts a frame on every open window.
    // Returns false once no window is left open.
    frame() => bool {
        W.pollEvents();
        let i: int = 0;
        while (i < this.count) {
            let w: Win* = this.wins[i];
            if (w.isOpen && w.win.shouldClose()) { w.destroy(); }
            i = i + 1;
        }
        let alive: int = 0;
        i = 0;
        while (i < this.count) {
            let w: Win* = this.wins[i];
            if (w.isOpen) { w.beginFrame(); alive = alive + 1; }
            i = i + 1;
        }
        return alive > 0;
    }

    // Flushes every 2D pen and presents every open window.
    present() => void {
        let i: int = 0;
        while (i < this.count) {
            let w: Win* = this.wins[i];
            if (w.isOpen) w.endFrame();
            i = i + 1;
        }
    }

    time() => double { return W.time(); }

    quit() => void {
        let i: int = 0;
        while (i < this.count) {
            let w: Win* = this.wins[i];
            w.destroy();
            CMem.free(w as void*);
            i = i + 1;
        }
        this.count = 0;
        if (this.vkCore != nullptr) {
            if (this.vkCore.device != nullptr) this.vkCore.free();
            CMem.free(this.vkCore as void*);
            this.vkCore = nullptr;
        }
        if (this.wins != nullptr) { CMem.free(this.wins as void*); this.wins = nullptr; }
        if (this.started) { W.terminate(); this.started = false; }
    }
}
