// widgets_test.os
// Part 1: headless self-test of the pen widgets (no window): fake input frames -> assert widget state.
// Part 2: interactive window (Esc closes). Skipped if part 1 fails; pass any value of `interactive` below.
// Not compiled/run. No `return` inside loop bodies.
module C = import "standard/1/Canvas.os";
module CMem = import "standard/1/c/mem.os";
module Console = import "standard/1/Console.os";

let interactive: bool = true;

function streq(a: char*, b: char*) => bool {
    let i: int = 0;
    let same: bool = true;
    let more: bool = true;
    while (more) {
        let ca: int = a[i] as int;
        let cb: int = b[i] as int;
        if (ca != cb) { same = false; more = false; }
        else if (ca == 0) { more = false; }
        else { i = i + 1; }
    }
    return same;
}

function itoa(n: int, out: char*) => void {
    let v: int = n;
    let p: int = 0;
    if (v < 0) { out[p] = 45 as char; p = p + 1; v = 0 - v; }
    let digits: int = 0;
    let tmp: int = v;
    if (tmp == 0) digits = 1;
    while (tmp > 0) { digits = digits + 1; tmp = tmp / 10; }
    let k: int = digits - 1;
    while (k >= 0) { out[p + k] = (48 + v % 10) as char; v = v / 10; k = k - 1; }
    out[p + digits] = 0 as char;
}

// Feeds fake input frames to a pen, the same snapshot Win fills in beginFrame().
struct Sim {
    pen: C.Pen2D* = nullptr;
    mx: float = 0.0; my: float = 0.0;
    down: bool = false; prev: bool = false;
    bs: bool = false; ent: bool = false;
    buf: char* = nullptr;
    passed: int = 0; failed: int = 0;

    init(p: C.Pen2D*) => void {
        this.pen = p;
        this.buf = CMem.malloc(8) as char*;
        this.buf[0] = 0 as char;
    }

    frame() => void {
        this.pen.inp.mx = this.mx; this.pen.inp.my = this.my;
        this.pen.inp.down = this.down;
        this.pen.inp.pressed = this.down && !this.prev;
        this.pen.inp.released = !this.down && this.prev;
        this.prev = this.down;
        this.pen.inp.typed = this.buf;
        this.pen.inp.backspace = this.bs;
        this.pen.inp.enter = this.ent;
        this.bs = false; this.ent = false;
        this.pen.updateWidgets();
    }

    at(x: float, y: float, d: bool) => void { this.mx = x; this.my = y; this.down = d; this.frame(); }
    click(x: float, y: float) => void { this.at(x, y, true); this.at(x, y, false); }
    typeChar(c: int) => void { this.buf[0] = c as char; this.buf[1] = 0 as char; this.frame(); this.buf[0] = 0 as char; }
    pressBackspace() => void { this.bs = true; this.frame(); }
    pressEnter() => void { this.ent = true; this.frame(); }

    check(name: char*, cond: bool) => void {
        if (cond) { this.passed = this.passed + 1; }
        else { this.failed = this.failed + 1; Console.log("  FAIL: "); Console.log(name); }
    }
    free() => void { if (this.buf != nullptr) { CMem.free(this.buf as void*); this.buf = nullptr; } }
}

function absf(v: float) => float { if (v < 0.0) return 0.0 - v; return v; }

function selfTest() => bool {
    let pen: C.Pen2D;
    if (!pen.initSoftware(320, 240)) { Console.log("FAIL: initSoftware"); return false; }
    let s: Sim;
    s.init(&pen);

    let btn = pen.createButton("Go", 10.0, 10.0, 100.0, 40.0);
    let chk = pen.createCheckbox("Opt", 10.0, 60.0);
    let sld = pen.createSlider(10.0, 100.0, 200.0, 0.0, 1.0);
    let tb  = pen.createTextBox(10.0, 130.0, 200.0);
    s.check("created 4 widgets", pen.wcount == 4 && btn != nullptr && chk != nullptr && sld != nullptr && tb != nullptr);

    // --- button: hover, press, release over = click, flags last one frame ---
    s.at(50.0, 30.0, false);
    s.check("button hovered", btn.hovered && !btn.pressed && !btn.down);
    s.at(50.0, 30.0, true);
    s.check("button down on mouse down, not yet pressed", btn.down && !btn.pressed);
    s.at(50.0, 30.0, false);
    s.check("button pressed + released on release over it", btn.pressed && btn.released && !btn.down);
    s.frame();
    s.check("button pressed lasts one frame", !btn.pressed && !btn.released);

    // --- release outside = no click ---
    s.at(50.0, 30.0, true);
    s.at(300.0, 200.0, true);
    s.check("button still held when mouse leaves", btn.down && !btn.hovered);
    s.at(300.0, 200.0, false);
    s.check("no click when released outside", !btn.pressed && btn.released);

    // --- wantsMouse ---
    s.at(500.0, 500.0, false);
    s.check("wantsMouse false off widgets", !pen.wantsMouse);
    s.at(50.0, 30.0, false);
    s.check("wantsMouse true over widget", pen.wantsMouse);

    // --- checkbox toggles ---
    s.click(15.0, 65.0);
    s.check("checkbox checked + changed", chk.checked && chk.changed && chk.pressed);
    s.frame();
    s.check("checkbox changed lasts one frame", chk.checked && !chk.changed);
    s.click(15.0, 65.0);
    s.check("checkbox unchecked", !chk.checked);

    // --- slider drag, clamp ---
    s.at(110.0, 110.0, true);
    s.check("slider mid value", absf(sld.value - 0.5) < 0.001 && sld.changed);
    s.at(500.0, 110.0, true);
    s.check("slider clamps to max", absf(sld.value - 1.0) < 0.001);
    s.at(0.0 - 50.0, 110.0, true);
    s.check("slider clamps to min", absf(sld.value) < 0.001);
    s.at(0.0 - 50.0, 110.0, false);

    // --- textbox: focus, type, backspace, enter, blur ---
    s.click(20.0, 140.0);
    s.check("textbox focused on click", tb.focused);
    s.typeChar(97);
    s.typeChar(98);
    s.check("textbox typed 'ab'", streq(tb.text, "ab") && tb.len == 2 && tb.changed);
    s.pressBackspace();
    s.check("textbox backspace -> 'a'", streq(tb.text, "a") && tb.len == 1);
    s.pressEnter();
    s.check("textbox submitted", tb.submitted);
    s.frame();
    s.check("textbox submitted lasts one frame", !tb.submitted);
    s.click(300.0, 200.0);
    s.check("textbox blurs on outside click", !tb.focused);
    s.typeChar(122);
    s.check("unfocused textbox ignores typing", streq(tb.text, "a"));

    // --- enabled / visible ---
    btn.enabled = false;
    s.click(50.0, 30.0);
    s.check("disabled button ignores click", !btn.pressed);
    btn.enabled = true;
    btn.hide();
    s.click(50.0, 30.0);
    s.check("hidden button ignores click", !btn.pressed);
    btn.show();
    s.click(50.0, 30.0);
    s.check("shown button clicks again", btn.pressed);

    // --- overlap: last created wins, destroy removes it ---
    let top = pen.createButton("Top", 10.0, 10.0, 100.0, 40.0);
    s.click(50.0, 30.0);
    s.check("topmost overlapping widget wins", top.pressed && !btn.pressed);
    pen.destroyWidget(top);
    s.check("destroyWidget shrinks count", pen.wcount == 4);
    s.click(50.0, 30.0);
    s.check("widget below receives clicks after destroy", btn.pressed);

    // --- labels never take the mouse ---
    let lbl = pen.createLabel("hi", 10.0, 10.0);
    s.click(12.0, 12.0);
    s.check("label does not steal clicks", btn.pressed);
    pen.destroyWidget(lbl);

    // --- render smoke test: button body pixel is the idle colour ---
    pen.begin(320, 240);
    s.at(300.0, 200.0, false);
    pen.drawWidgets();
    pen.fillRect(100.0, 100.0, 20.0, 20.0, 0x3A3A46FF);   // raw primitive, clear of the button (10..110 x 10..50)
    pen.flush();
    let px: u8* = pen.pixels();
    let q: int = (110 * 320 + 110) * 4;
    let rawOk: bool = (px[q] as int) >= 0x39 && (px[q] as int) <= 0x3B && (px[q + 3] as int) != 0;
    s.check("render: raw fillRect literal colour at (110,110)", rawOk);
    let o: int = (20 * 320 + 20) * 4;               // inside the button body, clear of border and caption
    let pr: int = px[o] as int; let pg: int = px[o + 1] as int; let pb: int = px[o + 2] as int; let pa: int = px[o + 3] as int;
    let drawn: bool = pa != 0;
    let rOk: bool = pr >= 0x39 && pr <= 0x3B;
    let gOk: bool = pg >= 0x39 && pg <= 0x3B;
    let bOk: bool = pb >= 0x45 && pb <= 0x47;
    s.check("render: something was drawn at (20,20)", drawn);
    s.check("render: button idle colour at (20,20) (+-1)", rOk && gOk && bOk);
    if (!(drawn && rOk && gOk && bOk)) {
        let db: char* = CMem.malloc(16) as char*;
        itoa(pr, db); Console.log(db);
        itoa(pg, db); Console.log(db);
        itoa(pb, db); Console.log(db);
        itoa(pa, db); Console.log(db);
        CMem.free(db as void*);
    }

    let nb: char* = CMem.malloc(16) as char*;
    itoa(s.passed, nb); Console.log(nb);
    itoa(s.failed, nb); Console.log(nb);
    CMem.free(nb as void*);

    let ok: bool = s.failed == 0;
    s.free();
    pen.free();
    return ok;
}

function drawStat(pen: C.Pen2D*, x: float, y: float, label: char*, n: int, tmp: char*) => void {
    pen.text(x, y, label, 0xAAAAAAFF);
    itoa(n, tmp);
    pen.text(x + pen.textWidth(label, 1.0) + 6.0, y, tmp, 0xFFFFFFFF);
}

function main() => int {
    if (!selfTest()) return 3;
    if (!interactive) return 0;

    let app: C.Canvas;
    if (!app.init()) return 1;
    let win = app.createWindow("widgets test", 800, 600);
    if (win == nullptr) { app.quit(); return 2; }
    let pen = win.context2d();

    let go    = pen.createButton("Count", 20.0, 20.0, 120.0, 32.0);
    let off   = pen.createButton("Disable me", 160.0, 20.0, 140.0, 32.0);
    let red   = pen.createCheckbox("Red square", 20.0, 70.0);
    let sld   = pen.createSlider(20.0, 110.0, 280.0, 0.0, 1.0);
    let name  = pen.createTextBox(20.0, 150.0, 280.0);
    let title = pen.createLabel("pen widgets demo (Esc quits)", 20.0, 200.0);

    let clicks: int = 0;
    let bgClicks: int = 0;
    let submits: int = 0;
    let tmp: char* = CMem.malloc(24) as char*;

    while (app.frame()) {
        if (win.keyPressed(256)) win.close();
        if (go.pressed) clicks = clicks + 1;
        if (off.pressed) { off.enabled = false; off.setText("(disabled)"); }
        if (name.submitted) submits = submits + 1;
        if (win.mousePressed() && !pen.wantsMouse) bgClicks = bgClicks + 1;

        let sq: u32 = 0x5B7CFAFF;
        if (red.checked) sq = 0xE04040FF;
        pen.fillRect(20.0 + sld.value * 240.0, 240.0, 40.0, 40.0, sq);

        drawStat(pen, 20.0, 300.0, "clicks:", clicks, tmp);
        drawStat(pen, 20.0, 320.0, "empty-space clicks:", bgClicks, tmp);
        drawStat(pen, 20.0, 340.0, "slider %:", (sld.value * 100.0) as int, tmp);
        drawStat(pen, 20.0, 360.0, "enter pressed:", submits, tmp);
        pen.text(20.0, 380.0, "typed:", 0xAAAAAAFF);
        pen.text(80.0, 380.0, name.text, 0xFFFFFFFF);
        app.present();
    }
    CMem.free(tmp as void*);
    app.quit();
    return 0;
}
