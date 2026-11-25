// standard/1/Ui/Root.os
// The `Ui` struct (Ui.Ui): owns the root node, metrics, style sheet, layout engine, event state, display list and backend.
//
//   let ui: Ui.Ui;
//   ui.init(win);                 // GUI on a Canvas window;  ui.initTui(80, 24) for a terminal
//   let root = ui.root();
//   let bar = root.row();  bar.gap = 8.0;  bar.padding = 8.0;
//   let ok = bar.button("OK");  let name = bar.input();  name.grow = 1.0;
//   ui.addCss("button:hover { background-color: #444; }");
//   while (app.frame()) { ui.frame(); app.present(); }     // events -> style -> layout -> paint -> backend
//   if (ok.pressed) { ... }
//
// frame() order: widget results -> node flags + events, mouse/keys, cascade (if dirty), layout (if dirty),
// widget rectangles, display list (if dirty), backend. Call it between win.beginFrame() and win.endFrame()
// (Pen2D widgets are drawn by endFrame, on top of the display list).
// fillWindow = true (default): the root fills the window (apps). Set false for documents that grow and scroll.

struct Ui {
    win: C.Win* = nullptr; pen: C.Pen2D* = nullptr;
    rootNode: Node* = nullptr;
    m: Metrics; meas: Measure; ctx: CssCtx; sheet: Sheet; base: Style;
    lay: Layout; ev: Events; dl: DisplayList; painter: Painter;
    gui: GuiBackend; tui: TuiBackend;
    backend: int = 0;
    vw: float = 0.0; vh: float = 0.0;
    fillWindow: bool = true;
    themeFg: u32 = 0; themeBg: u32 = 0;
    ready: bool = false;
    recordStyles: bool = false;        // inspector: the cascade fills node.styleNotes while this is on

    // ---- setup ----
    wire() => void {
        this.meas.m = &this.m;
        this.lay.meas = &this.meas; this.lay.m = &this.m;
        this.painter.dl = &this.dl; this.painter.m = &this.m;
        this.painter.tui = this.ctx.tui;
        this.themeFg = DEF_TEXT; this.themeBg = DEF_BG;
        this.base.reset();
        this.base.color = this.themeFg;
        this.base.fontPx = this.m.cellH;
        this.sheet.head = nullptr; this.sheet.seq = 0; this.sheet.usesPseudo = false;
        this.rootNode = createByTag("html");
        this.ready = true;
    }

    init(win: C.Win*) => bool {
        this.win = win;
        this.pen = win.context2d();
        this.backend = BK_GUI;
        this.m.cellW = 8.0; this.m.cellH = 16.0; this.m.lineH = 18.0; this.m.unit = 1.0;
        this.ctx.unit = 1.0; this.ctx.curPx = 16.0; this.ctx.rootPx = 16.0; this.ctx.tui = false;
        this.meas.pen = this.pen;
        this.wire();
        this.gui.init(this.pen);
        this.ev.init(win, this.pen);
        this.painter.pop = &this.ev.pop;
        this.painter.sel = &this.ev.sel;
        this.painter.ctx = &this.ev.ctx;
        return true;
    }

    initTui(cols: int, rows: int) => bool {
        this.win = nullptr; this.pen = nullptr;
        this.backend = BK_TUI;
        this.m.cellW = 1.0; this.m.cellH = 1.0; this.m.lineH = 1.0; this.m.unit = 0.125;
        this.ctx.unit = 0.125; this.ctx.curPx = 1.0; this.ctx.rootPx = 1.0; this.ctx.tui = true;
        this.meas.pen = nullptr;
        this.wire();
        this.tui.init(cols, rows);
        return true;
    }

    root() => Node* { return this.rootNode; }

    // ---- style ----
    // turn declaration recording on/off and restyle so every node's styleNotes are rebuilt
    setRecordStyles(on: bool) => void { this.recordStyles = on; this.rootNode.markStyle(); }
    addCss(css: char*) => void { sheetParse(&this.sheet, css); this.rootNode.markStyle(); }
    clearCss() => void { sheetClear(&this.sheet); this.rootNode.markStyle(); }
    setTheme(fg: u32, bg: u32) => void {
        this.themeFg = fg; this.themeBg = bg;
        this.base.color = fg;
        this.rootNode.markStyle();
    }

    // ---- events ----
    poll(out: Event*) => bool { return this.ev.poll(out); }
    dispatch() => void { this.ev.dispatch(); }       // run per-node handlers for queued events (frame() calls this too)
    wheel(dy: float) => bool { return this.ev.wheel(dy); }
    wheelX(dx: float) => bool { return this.ev.wheelX(dx); }
    focused() => Node* { return this.ev.focus; }
    hovered() => Node* { return this.ev.hot; }

    // ---- the frame ----
    frame() => void {
        if (!this.ready) return;
        let root: Node* = this.rootNode;
        let w: float = 0.0; let h: float = 0.0;
        if (this.backend == BK_GUI) { w = this.win.width() as float; h = this.win.height() as float; }
        else { w = this.tui.cols as float; h = this.tui.rows as float; }
        if (w != this.vw || h != this.vh) {
            this.vw = w; this.vh = h; this.ctx.vw = w; this.ctx.vh = h;
            root.markStyle();
        }

        if (this.backend == BK_GUI) {
            this.ev.viewW = w; this.ev.viewH = h;
            this.ev.begin(root);
            this.ev.scan(root);
            this.ev.update(root, this.sheet.usesPseudo);
            this.ev.dispatch();
            this.win.setCursor(this.ev.cursor);
        }

        if (root.dirtyStyle || root.subStyle) {
            cssSetRecord(this.recordStyles);
            cascadeNode(root, &this.base, &this.sheet, &this.ctx, false);
            if ((root.st.bg & 0xFF) == 0) root.st.bg = this.themeBg;
            root.st.display = DP_BLOCK;
        }
        if (root.dirtyLayout) {
            this.lay.layoutRoot(root, w, h, this.fillWindow);
        }
        if (this.backend == BK_GUI) {
            ctlGeom(root, 0.0, 0.0, 0.0, 0.0, w, h, false);
            if (this.ev.ctx.isOpen()) ctlHideUnder(root, nullptr, this.ev.ctx.x, this.ev.ctx.y, this.ev.ctx.w, this.ev.ctx.h);
            if (this.ev.pop.isOpen()) ctlHideUnder(root, this.ev.pop.owner, this.ev.pop.x, this.ev.pop.y, this.ev.pop.w, this.ev.pop.h);
        }

        if (root.dirtyPaint) {
            this.painter.paint(root);
            root.dirtyPaint = false;
        }
        if (this.backend == BK_GUI) {
            this.gui.run(&this.dl, &this.m);
        } else {
            this.tui.clear(this.themeBg);
            this.tui.run(&this.dl, &this.m);
        }
    }

    // terminal output of the last frame (ANSI string); print it with your console
    tuiText() => char* { return this.tui.frameText(); }

    destroy() => void {
        if (this.rootNode != nullptr) { this.rootNode.destroy(); this.rootNode = nullptr; }
        sheetClear(&this.sheet);
        this.dl.free();
        if (this.backend == BK_TUI) this.tui.destroy();
        this.ready = false;
    }
}
