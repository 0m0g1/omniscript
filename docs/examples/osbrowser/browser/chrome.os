// browser/chrome.os  -- browser UI around the page: back / forward / reload + address bar.
// Uses Pen2D widgets for now (so typing, selection, Enter already work).
// NEXT: tab strip (Tab struct per page), bookmarks bar, loading indicator, rebuild on Ui (row/button/input).
//   Shortcuts: Enter in the bar = go, Ctrl+L = focus bar, F5 = reload.
module C = import "standard/1/Canvas.os";
module U = import "browser/util.os";
module B = import "browser/browser.os";

function height() => float { return 44.0; }

struct Chrome {
    back: C.Widget* = nullptr;
    fwd: C.Widget* = nullptr;
    reload: C.Widget* = nullptr;
    addr: C.Widget* = nullptr;
    inspect: C.Widget* = nullptr;       // opens/closes the inspector window (same as F12)
    shown: char* = nullptr;

    init(pen: C.Pen2D*) => void {
        this.back = pen.createButton("<", 8.0, 8.0, 30.0, 28.0);
        this.fwd = pen.createButton(">", 42.0, 8.0, 30.0, 28.0);
        this.reload = pen.createButton("R", 76.0, 8.0, 30.0, 28.0);
        this.addr = pen.createTextBox(114.0, 8.0, 400.0);
        this.inspect = pen.createButton("Inspect", 520.0, 8.0, 64.0, 28.0);
    }

    update(win: C.Win*, pen: C.Pen2D*, b: B.Browser*, winW: float) => void {
        this.addr.w = winW - 122.0 - 72.0;
        this.inspect.x = winW - 72.0;
        if (this.back.pressed) b.back();
        if (this.fwd.pressed) b.forward();
        if (this.reload.pressed) b.reload();
        if (win.keyPressed(294)) b.reload();                                          // F5
        if (win.ctrlDown() && win.keyPressed(76)) { pen.setFocus(this.addr); this.addr.selectAll(); }   // Ctrl+L
        if (this.addr.submitted) b.navigate(this.addr.text);
        this.back.enabled = b.canBack();
        this.fwd.enabled = b.canForward();
        if (b.url != nullptr && this.shown != b.url) {
            this.shown = b.url;
            this.addr.setText(b.url);
            win.setTitle(U.cat("OmniBrowser - ", b.title()));
        }
    }

    paintBackground(pen: C.Pen2D*, winW: float) => void {
        pen.fillRect(0.0, 0.0, winW, height(), 0x2A2A33FF);
        pen.fillRect(0.0, height() - 1.0, winW, 1.0, 0x55556AFF);
    }

    // progress bar under the toolbar + status line at the bottom while loading (or after a failure)
    // NEXT: it only animates between stages; a worker thread is needed to animate during a download.
    paintStatus(pen: C.Pen2D*, b: B.Browser*, winW: float, winH: float, ms: double) => void {
        if (b.loading) {
            let span: int = (winW as int) + 160;
            let x: int = ((ms as int) / 3) % span - 160;
            pen.fillRect(0.0, height() - 3.0, winW, 3.0, 0x3A3A46FF);
            pen.fillRect(x as float, height() - 3.0, 160.0, 3.0, 0x5B7CFAFF);
        }
        if (U.slen(b.status) > 0) {
            pen.fillRect(0.0, winH - 22.0, winW, 22.0, 0x2A2A33FF);
            pen.text(8.0, winH - 19.0, b.status, 0xE8E8F0FF);
        }
    }
}
