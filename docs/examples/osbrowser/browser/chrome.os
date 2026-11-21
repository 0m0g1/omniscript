// browser/chrome.os  -- browser UI built from Ui nodes: toolbar (back/forward/reload, address bar, Inspect),
// progress bar, the scrolling page container, and a status line.
//   Layout: root (flex column) = [ toolbar | progress | page (grows, scrolls) | status ]
// Note: page stylesheets are global in Ui, so a page rule like `button {...}` can also restyle the toolbar
// (inline styles here win for the properties they set). NEXT: scope page CSS under the page container.
module UI = import "standard/1/Ui.os";
module C = import "standard/1/Canvas.os";
module U = import "browser/util.os";
module B = import "browser/browser.os";

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
    lastFit: float = 0.0 - 1.0;         // page height set by fit()

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
        this.page.style("overflow: auto;");                          // its height is set every frame by fit(): the window minus toolbar and status line
        this.statusLbl = root.label("");
        this.statusLbl.style("padding: 2px 8px; background-color: #2a2a33;");
    }

    // Gives the page a definite height (window height minus what sits above and below it), so that content taller
    // than that scrolls inside the page instead of stretching it past the window. Call every frame after update():
    // it re-fits after a resize, a hidden/shown progress bar, or a status change. Uses last frame's layout boxes.
    fit(win: C.Win*) => void {
        let st: UI.Node* = this.statusLbl;
        let below: float = st.box.h + st.box.mt + st.box.mb;
        let avail: float = (win.height() as float) - this.page.box.y - below;
        if (avail < 40.0) avail = 40.0;
        let d: float = avail - this.lastFit;
        if (d < 0.0) d = 0.0 - d;
        if (d > 0.5) {
            this.lastFit = avail;
            this.page.height = avail;
            this.page.markLayout();
        }
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
