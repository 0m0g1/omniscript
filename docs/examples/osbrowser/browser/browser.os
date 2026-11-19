// browser/browser.os  -- module root: wires the layers + navigation history.
//   loader -> html -> dom -> css -> layout -> paint -> backend      (js hooks in via DOM)
module U = import "browser/util.os";
module Dbg = import "browser/dbg.os";
module Net = import "browser/net/loader.os";
module D = import "browser/dom/dom.os";
module H = import "browser/html/html.os";
module CSS = import "browser/css/css.os";
module L = import "browser/layout/layout.os";
module P = import "browser/paint/paint.os";
module JS = import "browser/js/js.os";

struct Browser {
    url: char* = nullptr;
    doc: H.Doc;
    list: P.Cmd* = nullptr;
    pageH: float = 0.0;
    viewW: float = 0.0;
    scrollY: float = 0.0;
    needLayout: bool = true;
    hist: [32]char*;
    histN: int = 0;
    histPos: int = 0 - 1;

    // fetch + parse + run scripts. Keeps the old page if the load fails.
    // NEXT: async loading, subresource cache, error page.
    open(url: char*) => bool {
        let html: char* = Net.load(url);
        if (html == nullptr) { Dbg.error(U.cat("cannot load ", url)); return false; }
        this.url = url;
        this.scrollY = 0.0;
        this.doc.cssN = 0;
        this.doc.jsN = 0;
        H.parseHtml(html, &this.doc);

        CSS.rules = nullptr;
        CSS.parseSheet(this.doc.css);
        let i: int = 0;
        while (i < this.doc.cssN) {
            let sheet: char* = Net.load(Net.resolve(url, this.doc.cssLinks[i]));
            if (sheet != nullptr) CSS.parseSheet(sheet);
            i = i + 1;
        }

        let js: char* = this.doc.js;
        i = 0;
        while (i < this.doc.jsN) {
            let code: char* = Net.load(Net.resolve(url, this.doc.jsLinks[i]));
            if (code != nullptr) js = U.cat(js, U.cat(code, "\n"));
            i = i + 1;
        }
        if (U.slen(js) > 0) JS.run(this.doc.root, js);
        this.needLayout = true;
        return true;
    }

    // user-initiated load (address bar): normalises the text and records history
    navigate(raw: char*) => bool {
        let u: char* = Net.normalize(raw);
        let ok: bool = this.open(u);
        if (ok) {
            this.histN = this.histPos + 1;                       // drop forward history
            if (this.histN >= 32) {
                let i: int = 1;
                while (i < 32) { this.hist[i - 1] = this.hist[i]; i = i + 1; }
                this.histN = 31;
            }
            this.hist[this.histN] = u;
            this.histPos = this.histN;
            this.histN = this.histN + 1;
        }
        return ok;
    }

    canBack() => bool { return this.histPos > 0; }
    canForward() => bool { return this.histPos + 1 < this.histN; }
    back() => void { if (this.canBack()) { this.histPos = this.histPos - 1; this.open(this.hist[this.histPos]); } }
    forward() => void { if (this.canForward()) { this.histPos = this.histPos + 1; this.open(this.hist[this.histPos]); } }
    reload() => void { if (this.url != nullptr) this.open(this.url); }

    title() => char* {
        let out: char* = "";
        if (this.url != nullptr) out = this.url;
        if (this.doc.root != nullptr) {
            let t: D.Node* = D.findTag(this.doc.root, "title");
            if (t != nullptr && t.first != nullptr && t.first.kind == 2) out = t.first.text;
        }
        return out;
    }

    // style -> layout -> paint (only when something changed)
    refresh(viewW: float) => void {
        if (this.doc.root != nullptr) {
            if (viewW != this.viewW) { this.viewW = viewW; this.needLayout = true; }
            if (JS.takeDirty()) this.needLayout = true;
            if (this.needLayout) {
                CSS.styleTree(this.doc.root, nullptr);
                this.pageH = L.layoutDocument(this.doc.root, viewW);
                this.list = P.buildDisplayList(this.doc.root, viewW, this.pageH);
                this.needLayout = false;
            }
        }
    }

    // x,y are relative to the top-left of the page viewport (below the chrome)
    pick(x: float, y: float) => D.Node* {
        let hit: D.Node* = nullptr;
        if (this.doc.root != nullptr) hit = P.hitTest(this.doc.root, x, y + this.scrollY);
        return hit;
    }

    click(x: float, y: float, ms: double) => void {
        let hit: D.Node* = this.pick(x, y);
        if (hit != nullptr) JS.fireClick(hit, ms);
    }

    tick(ms: double) => void { JS.tick(ms); }
}
