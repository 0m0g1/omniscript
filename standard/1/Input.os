// standard/1/Input.os
// Tier 5: polled keyboard/mouse state via GLFW. Call update(win) once per frame.
module G = import "standard/1/c/glfw.os";
module W = import "standard/1/Window.os";

let KEY_SPACE: int = 32; let KEY_ESCAPE: int = 256; let KEY_ENTER: int = 257; let KEY_TAB: int = 258;
let KEY_RIGHT: int = 262; let KEY_LEFT: int = 263; let KEY_DOWN: int = 264; let KEY_UP: int = 265;
let KEY_LSHIFT: int = 340; let KEY_LCTRL: int = 341;
// letters/digits use their ASCII uppercase code: KEY_A = 65 ... KEY_Z = 90, '0'..'9' = 48..57
let MOUSE_LEFT: int = 0; let MOUSE_RIGHT: int = 1; let MOUSE_MIDDLE: int = 2;

struct Input {
    win: void* = nullptr;
    prev: [512]u8;
    cur: [512]u8;
    mousePrev: [8]u8;
    mouseCur: [8]u8;
    mx: double = 0.0;
    my: double = 0.0;

    bind(w: W.Window*) => void { this.win = w.h; }

    update() => void {
        let i: int = 0;
        while (i < 512) { this.prev[i] = this.cur[i]; i = i + 1; }
        i = 32;
        while (i < 349) { this.cur[i] = (G.glfwGetKey(this.win, i) == 1) as u8; i = i + 1; }
        i = 0;
        while (i < 8) { this.mousePrev[i] = this.mouseCur[i]; this.mouseCur[i] = (G.glfwGetMouseButton(this.win, i) == 1) as u8; i = i + 1; }
        G.glfwGetCursorPos(this.win, &this.mx, &this.my);
    }

    down(k: int) => bool { return this.cur[k] != 0; }
    pressed(k: int) => bool { return this.cur[k] != 0 && this.prev[k] == 0; }
    released(k: int) => bool { return this.cur[k] == 0 && this.prev[k] != 0; }
    mouseDown(b: int) => bool { return this.mouseCur[b] != 0; }
    mousePressed(b: int) => bool { return this.mouseCur[b] != 0 && this.mousePrev[b] == 0; }
}
