// browser/browser.os  -- module root: HTML/JS/network on top of the standard Ui module.
//   loader -> html parser -> Ui tree -> (Ui: cascade, layout, paint, backend)      js binds to Ui nodes
// Loading runs one stage per frame (see step()) so the window can show status + progress between stages and
// each stage's time is logged to the Dbg console. Ui does style+layout inside ui.frame().
// NEXT: real async fetch (Thread/Channel); image loading; scoping page CSS away from the browser chrome.
module UI = import "standard/1/Ui.os";
module U = import "browser/util.os";
module A = import "browser/uiapi.os";
module Dbg = import "browser/dbg.os";
module Net = import "browser/net/loader.os";
module H = import "browser/html/html.os";
module JS = import "browser/js/js.os";

let JS_REMOTE: bool = false;     // run scripts on http(s) pages? real-world JS is far beyond this engine; off by default
let JS_MAX: int = 150000;        // skip scripts bigger than this many bytes

let S_IDLE: int = 0;
let S_FETCH: int = 1;
let S_PARSE: int = 2;
let S_CSS_INLINE: int = 3;
let S_CSS_LINK: int = 4;
let S_JS_LINK: int = 5;
let S_JS_RUN: int = 6;
let S_ATTACH: int = 7;
let S_DONE: int = 8;

// first <a href> in the subtree that was clicked this frame (Ui flags `clicked` on ancestors too)
function clickedLink(n: UI.Node*) => UI.Node* {
    let found: UI.Node* = nullptr;
    if (n.clicked && U.streq(A.tagOf(n), "a") && n.hasAttr("href")) found = n;
    let c: UI.Node* = n.firstEl();
    while (c != nullptr && found == nullptr) { found = clickedLink(c); c = c.nextEl(); }
    return found;
}

struct Browser {
    page: UI.Node* = nullptr;    // container in the Ui tree that holds the document (scrolls)
    url: char* = nullptr;
    doc: H.Doc;
    gen: int = 0;                // increments on every new document (inspector drops stale selections)
    hist: [32]char*;
    histN: int = 0;
    histPos: int = 0 - 1;

    loading: bool = false;
    status: char* = "";
    state: int = 0;
    stageI: int = 0;
    stageT: double = 0.0;
    stageName: char* = nullptr;
    pendingUrl: char* = nullptr;
    navKind: int = 0;            // 0 push history, 1 history move (navPos), 2 reload
    navPos: int = 0;
    htmlSrc: char* = nullptr;
    jsAcc: char* = nullptr;

    // CSS handed to main.os, which owns the UI.Ui and applies it (ui.clearCss / ui.addCss) once per frame
    cssReset: bool = false;
    cssQ: [8]char*;
    qN: int = 0;
    queueCss(text: char*) => void { if (this.qN < 8) { this.cssQ[this.qN] = text; this.qN = this.qN + 1; } }

    startLoad(url: char*, kind: int, pos: int) => void {
        this.pendingUrl = url;
        this.navKind = kind;
        this.navPos = pos;
        this.state = S_FETCH;
        this.stageI = 0;
        this.loading = true;
        this.status = U.cat("Fetching ", url);
    }

    // user-initiated load (address bar). Queued: the work happens in step().
    navigate(raw: char*) => void { this.startLoad(Net.normalize(raw), 0, 0); }
    canBack() => bool { return this.histPos > 0; }
    canForward() => bool { return this.histPos + 1 < this.histN; }
    back() => void { if (this.canBack() && !this.loading) this.startLoad(this.hist[this.histPos - 1], 1, this.histPos - 1); }
    forward() => void { if (this.canForward() && !this.loading) this.startLoad(this.hist[this.histPos + 1], 1, this.histPos + 1); }
    reload() => void { if (this.url != nullptr && !this.loading) this.startLoad(this.url, 2, 0); }

    progress() => float { return (this.state as float) / 9.0; }

    pushHistory(u: char*) => void {
        this.histN = this.histPos + 1;                           // drop forward history
        if (this.histN >= 32) {
            let i: int = 1;
            while (i < 32) { this.hist[i - 1] = this.hist[i]; i = i + 1; }
            this.histN = 31;
        }
        this.hist[this.histN] = u;
        this.histPos = this.histN;
        this.histN = this.histN + 1;
    }

    fail(msg: char*) => void {
        Dbg.error(msg);
        this.status = msg;
        this.loading = false;
        this.state = S_IDLE;
    }

    finishStage(name: char*, next: int, nextStatus: char*) => void {
        this.stageName = name;
        this.state = next;
        this.status = nextStatus;
    }

    // one stage of the load; each blocks only for its own work
    runStage() => void {
        let st: int = this.state;
        if (st == S_FETCH) {
            let html: char* = Net.load(this.pendingUrl);
            if (html == nullptr) { this.fail(U.cat("Cannot load ", this.pendingUrl)); }
            else {
                this.htmlSrc = html;
                this.finishStage("fetch html", S_PARSE, U.cat("Parsing HTML (", U.cat(U.fromInt(U.slen(html) / 1024), " KB)")));
            }
        }
        else if (st == S_PARSE) {
            this.url = this.pendingUrl;
            if (this.navKind == 0) this.pushHistory(this.url);
            else if (this.navKind == 1) this.histPos = this.navPos;
            if (this.doc.root != nullptr) { this.doc.root.remove(); this.doc.root.destroy(); this.doc.root = nullptr; }
            JS.reset();
            this.cssReset = true;
            this.qN = 0;
            this.doc.cssN = 0;
            this.doc.jsN = 0;
            this.gen = this.gen + 1;
            H.parseHtml(this.htmlSrc, &this.doc);                // builds Ui nodes, not attached yet (no style cost)
            this.finishStage("parse html", S_CSS_INLINE, "Parsing CSS");
        }
        else if (st == S_CSS_INLINE) {
            this.queueCss(this.doc.css);
            this.stageI = 0;
            if (this.doc.cssN > 0) this.finishStage("inline css", S_CSS_LINK, "Fetching stylesheet 1");
            else { this.jsAcc = U.dup(""); this.finishStage("inline css", S_JS_LINK, "Loading scripts"); }
        }
        else if (st == S_CSS_LINK) {
            let sheet: char* = Net.load(Net.resolve(this.url, this.doc.cssLinks[this.stageI]));
            if (sheet != nullptr) this.queueCss(sheet);
            this.stageI = this.stageI + 1;
            if (this.stageI < this.doc.cssN) this.finishStage("stylesheet", S_CSS_LINK, U.cat("Fetching stylesheet ", U.fromInt(this.stageI + 1)));
            else { this.stageI = 0; this.jsAcc = U.dup(""); this.finishStage("stylesheet", S_JS_LINK, "Loading scripts"); }
        }
        else if (st == S_JS_LINK) {
            let remote: bool = Net.isHttp(this.url);
            if (remote && !JS_REMOTE) {
                if (this.doc.jsN > 0 || U.slen(this.doc.js) > 0) Dbg.log("scripts skipped on remote page (JS_REMOTE = false)");
                this.finishStage("scripts", S_ATTACH, "Styling & layout");
            }
            else if (this.stageI < this.doc.jsN) {
                let code: char* = Net.load(Net.resolve(this.url, this.doc.jsLinks[this.stageI]));
                if (code != nullptr) this.jsAcc = U.cat(this.jsAcc, U.cat(code, "\n"));
                this.stageI = this.stageI + 1;
                this.finishStage("script fetch", S_JS_LINK, "Loading scripts");
            }
            else { this.finishStage("scripts", S_JS_RUN, "Running scripts"); }
        }
        else if (st == S_JS_RUN) {
            let js: char* = U.cat(this.doc.js, this.jsAcc);
            if (U.slen(js) > JS_MAX) Dbg.log("scripts skipped: larger than JS_MAX");
            else if (U.slen(js) > 0) JS.run(this.doc.root, js);
            this.finishStage("run js", S_ATTACH, "Styling & layout");
        }
        else if (st == S_ATTACH) {
            // the heavy cascade + layout happens inside the next ui.frame(); the status above is already on screen
            this.page.append(this.doc.root);
            this.page.scrollTo(0.0, 0.0);
            this.finishStage("attach", S_DONE, "Styling & layout");
        }
        else if (st == S_DONE) {
            this.loading = false;
            this.status = "";
            this.stageName = "style + layout";
            this.state = S_IDLE;
        }
    }

    // call once per frame AFTER presenting, so the status of the coming stage is already visible
    step(now: double) => void {
        if (this.stageName != nullptr) {
            Dbg.log(U.cat(this.stageName, U.cat(": ", U.cat(U.fromInt((now - this.stageT) as int), " ms"))));
            this.stageName = nullptr;
        }
        if (this.state != S_IDLE) {
            this.stageT = now;
            this.runStage();
        }
    }

    // after ui.frame(), when a click event was seen: link navigation + JS click listeners
    handleClicks(ms: double) => void {
        if (this.doc.root != nullptr && !this.loading) {
            let link: UI.Node* = clickedLink(this.doc.root);
            if (link != nullptr) this.navigate(Net.resolve(this.url, link.getAttr("href")));
            else JS.pumpClicks(ms);
        }
    }

    title() => char* {
        let out: char* = "";
        if (this.url != nullptr) out = this.url;
        if (this.doc.root != nullptr) {
            let t: UI.Node* = this.doc.root.findTag("title");
            if (t != nullptr && U.slen(A.textOf(t)) > 0) out = A.textOf(t);
        }
        return out;
    }

    tick(ms: double) => void { if (!this.loading) JS.tick(ms); }
}
