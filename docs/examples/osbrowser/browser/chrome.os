// browser/chrome.os  -- browser UI built from Ui nodes: toolbar (back/forward/reload, address bar, Inspect),
// progress bar, the scrolling page container, and a status line.
//   Layout: root (flex column) = [ toolbar | progress | page (grows, scrolls) | status ]
// Note: page stylesheets are global in Ui, so a page rule like `button {...}` can also restyle the toolbar
// (inline styles here win for the properties they set). NEXT: scope page CSS under the page container.
module UI = import "standard/1/Ui.os";
module C = import "standard/1/Canvas.os";
module U = import "util.os";
module B = import "browser.os";

struct Chrome {
    bar: UI.Node* = nullptr;
    back: UI.Node* = nullptr;
    fwd: UI.Node* = nullptr;
    reload: UI.Node* = nullptr;
    addr: UI.Node* = nullptr;
    inspect: UI.Node* = nullptr;
    progress: UI.Node* = nullptr;
    page: UI.Node* = nullptr;
    statusLbl: UI.Node* = nullptr;
    shown: char* = nullptr;
    lastStatus: char* = nullptr;
    progShown: bool = false;
    lastFrac: float = 0.0;
    backOn: bool = true;
    fwdOn: bool = true;
    lastH: float = 0.0;

    init(root: UI.Node*) => void {
        root.style("display: flex; flex-direction: column;");          // ADAPT if the root should be wrapped in a panel instead
        this.bar = root.row();
        this.bar.style("padding: 6px; gap: 6px; background-color: #2a2a33;");
        this.back = this.bar.button("<");
        this.fwd = this.bar.button(">");
        this.reload = this.bar.button("R");
        this.addr = this.bar.input();
        this.addr.grow = 1.0;
        this.inspect = this.bar.button("Inspect");
        this.progress = root.progress(0.0);
        this.progress.setAttr("hidden", "hidden");
        this.page = root.box();
        this.page.style("overflow: auto;");               // height is set by fit() every frame
        this.statusLbl = root.label("");
        this.statusLbl.style("padding: 2px 8px; background-color: #2a2a33;");
    }

    // Page height = window height - toolbar - progress (when shown) - status line. Call every frame BEFORE ui.frame()
    // (and from the resize redraw) so a resize lays out at once. Bar/status heights come from the previous layout.
    fit(win: C.Win*) => void {
        let wh: float = win.height() as float;
        let used: float = this.bar.box.h + this.statusLbl.box.h;
        if (this.progShown) used = used + this.progress.box.h;
        let h: float = wh - used;
        if (h < 60.0) h = 60.0;
        if (h != this.lastH) { this.lastH = h; this.page.height = h; this.page.dirty(); }
    }

    // call right after ui.frame(): flags like `pressed` / `submitted` are true for exactly this frame
    update(win: C.Win*, b: B.Browser*) => void {
        if (this.back.pressed) b.back();
        if (this.fwd.pressed) b.forward();
        if (this.reload.pressed || win.keyPressed(294)) b.reload();      // R button or F5
        if (this.addr.submitted) b.navigate(this.addr.getValue());

        let cb: bool = b.canBack();
        if (cb != this.backOn) { this.backOn = cb; this.back.setDisabled(!cb); }
        let cf: bool = b.canForward();
        if (cf != this.fwdOn) { this.fwdOn = cf; this.fwd.setDisabled(!cf); }

        if (b.url != nullptr && this.shown != b.url) {
            this.shown = b.url;
            this.addr.setValue(b.url);
            win.setTitle(U.cat("OmniBrowser - ", b.title()));
        }
        if (this.lastStatus != b.status) { this.lastStatus = b.status; this.statusLbl.setText(b.status); }

        if (b.loading) {
            if (!this.progShown) { this.progShown = true; this.progress.removeAttr("hidden"); }
            let f: float = b.progress();
            if (f != this.lastFrac) { this.lastFrac = f; this.progress.setRange(0.0, 1.0, f); }
        } else if (this.progShown) {
            this.progShown = false;
            this.progress.setAttr("hidden", "hidden");
        }
    }
}
