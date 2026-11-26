// browser/browser.os  -- module root: HTML/JS/network on top of the standard Ui module.
//   loader -> html parser -> Ui tree -> (Ui: cascade, layout, paint, backend)      js binds to Ui nodes
// Loading runs one stage per frame (see step()) so the window can show status + progress between stages and
// each stage's time is logged to the Dbg console. Ui does style+layout inside ui.frame().
// Images are fetched + decoded on a small worker pool (Thread/Channel); only the GPU upload and the Ui node update
// happen on the main thread, a few per frame (see submitImages / consumeImages).
// NEXT: async fetch for html/css/js too; scoping page CSS away from the browser chrome.
module UI = import "standard/1/Ui.os";
module U = import "util.os";
module A = import "uiapi.os";
module Dbg = import "dbg.os";
module Net = import "net/loader.os";
module H = import "html/html.os";
module JS = import "js/js.os";
module Str = import "standard/1/String.os";
module Text = import "standard/1/Text.os";
module CF = import "css/flatten.os";          // CSS nesting + @media + var() -> flat CSS for the Ui (ADAPT: path as for html/html.os)
module V = import "viewers/registry.os";      // file viewers: text, Markdown, CSV, JSON (ADAPT: path as for html/html.os)

module C = import "standard/1/Canvas.os";      // Pen2D: registers decoded images
module Img = import "standard/1/Image.os";      // decoder: Image.load_memory(bytes, len)
module Th = import "standard/1/Thread.os";      // Mutex, Atomic
module Ch = import "standard/1/Channel.os";     // ThreadPool

let JS_REMOTE: bool = true;     // run scripts on http(s) pages? real-world JS is far beyond this engine; off by default
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
let S_IMG: int = 9;              // after the page is shown: one <img> per frame
let IMG_MAX: int = 48;           // images fetched per page
let IMG_WORKERS: u64 = 2;        // worker threads for image fetch + decode
let IMG_PER_FRAME: int = 3;      // GPU uploads / node updates per frame on the main thread
let BG_MAX: int = 64;            // CSS background images per page
let BGC_MAX: int = 48;           // entries in the background image cache
let CSS_MAX: int = 20;           // stylesheets queued per page (Browser.cssQ size)

// image slot states (Atomic i64, written by a worker, read by the main thread)
let K_IDLE: i64 = 0;
let K_QUEUED: i64 = 1;           // job submitted, worker has not finished
let K_RASTER: i64 = 2;           // decoded: slot.im holds pixels, main thread uploads
let K_BYTES: i64 = 3;            // not a raster format: slot.buf holds bytes, main thread tries SVG
let K_SKIP: i64 = 4;             // stale (page changed) - nothing to do
let K_FAIL: i64 = 5;             // fetch failed
let K_DONE: i64 = 6;             // consumed by the main thread
let K_SVG: i64 = 7;              // inline <svg>: rasterized by the main thread

// first <a href> in the subtree that was clicked this frame (Ui flags `clicked` on ancestors too)
function clickedLink(n: UI.Node*) => UI.Node* {
    let found: UI.Node* = nullptr;
    if (n.clicked && U.streq(A.tagOf(n), "a") && n.hasAttr("href")) found = n;
    let c: UI.Node* = n.firstEl();
    while (c != nullptr && found == nullptr) { found = clickedLink(c); c = c.nextEl(); }
    return found;
}

// ---- text downloads: shown as a page with a Download link ----
function escInto(out: Str.String*, s: char*) => void {
    let i: u64 = 0;
    while ((s[i] as int) != 0) {
        let c: int = s[i] as int;
        if (c == 38) out.append("&amp;");
        else if (c == 60) out.append("&lt;");
        else if (c == 62) out.append("&gt;");
        else out.appendChar(s[i]);
        i = i + 1;
    }
}

function viewPage(text: char*, name: char*) => char* {
    let o: Str.String;
    o.append("<html><head><title>");
    escInto(&o, name);
    o.append("</title></head><body style=\"padding: 12px;\"><p><a href=\"#save-download\">Download ");
    escInto(&o, name);
    o.append("</a></p><pre>");
    escInto(&o, text);
    o.append("</pre></body></html>");
    let r: char* = U.dup(o.cstr());
    o.free();
    return r;
}

// ---- forms ----
function addField(body: Str.String*, name: char*, value: char*) => void {
    if (body.len > 0) body.append("&");
    Text.urlEncode(name, body);
    body.append("=");
    Text.urlEncode(value, body);
}

function optionValue(o: UI.Node*) => char* {
    let v: char* = o.getAttr("value");
    if (v == nullptr) {
        v = "";
        let t: UI.Node* = o.first;
        if (t != nullptr) v = A.textOf(t);
    }
    return v;
}

// successful controls of a form: named, enabled inputs / selects / textareas (submit buttons are not sent)
function collect(n: UI.Node*, body: Str.String*) => void {
    let c: UI.Node* = n.firstEl();
    while (c != nullptr) {
        let tag: char* = A.tagOf(c);
        let nm: char* = c.getAttr("name");
        if (nm != nullptr && !c.disabled) {
            if (U.streq(tag, "input")) {
                let ty: char* = U.lower(A.attr(c, "type"));
                if (U.streq(ty, "checkbox") || U.streq(ty, "radio")) {
                    if (c.checked) {
                        let v: char* = c.getAttr("value");
                        if (v == nullptr) v = "on";
                        addField(body, nm, v);
                    }
                }
                else if (U.streq(ty, "submit") || U.streq(ty, "button") || U.streq(ty, "reset") || U.streq(ty, "image") || U.streq(ty, "file")) { }
                else if (U.streq(ty, "hidden")) addField(body, nm, A.attr(c, "value"));
                else addField(body, nm, A.valueOf(c));
            }
            else if (U.streq(tag, "textarea")) {
                let v2: char* = A.valueOf(c);
                if (U.slen(v2) == 0 && c.first != nullptr) v2 = A.textOf(c.first);
                addField(body, nm, v2);
            }
            else if (U.streq(tag, "select")) {
                let k: int = 0;
                let o: UI.Node* = c.firstEl();
                while (o != nullptr) {
                    if (k == c.selIndex) addField(body, nm, optionValue(o));
                    k = k + 1;
                    o = o.nextEl();
                }
            }
        }
        collect(c, body);
        c = c.nextEl();
    }
}

function formAncestor(n: UI.Node*) => UI.Node* {
    let a: UI.Node* = n;
    let found: UI.Node* = nullptr;
    while (a != nullptr && found == nullptr) {
        if (U.streq(A.tagOf(a), "form")) found = a;
        a = a.parent;
    }
    return found;
}

// The form to submit when `n` was clicked: a <button> without type="button"/"reset" inside a <form> (the Ui only
// treats <input type=submit> as a submit control). nullptr otherwise.
function submitterForm(n: UI.Node*) => UI.Node* {
    let a: UI.Node* = n;
    let f: UI.Node* = nullptr;
    let hit: bool = false;
    while (a != nullptr && !hit) {
        if (U.streq(A.tagOf(a), "button")) {
            hit = true;
            let ty: char* = U.lower(A.attr(a, "type"));
            if (!U.streq(ty, "button") && !U.streq(ty, "reset")) f = formAncestor(a);
        }
        a = a.parent;
    }
    return f;
}

// first URL of a srcset ("a.png 1x, b.png 2x" -> "a.png")
function firstSrcset(v: char*) => char* {
    let n: int = U.slen(v);
    let a: int = 0;
    while (a < n && U.isSpace(v[a] as int)) { a = a + 1; }
    let e: int = a;
    while (e < n && !U.isSpace(v[e] as int) && (v[e] as int) != 44) { e = e + 1; }
    return U.sub(v, a, e);
}

// the URL an <img> should load: src, else the usual lazy-load attributes, else the first srcset candidate.
// data: URIs are skipped (Net cannot fetch them). "" when there is nothing usable.
function imgSrc(n: UI.Node*) => char* {
    let r: char* = "";
    let s0: char* = n.getAttr("src");
    if (s0 != nullptr && U.slen(s0) > 0 && !U.startsAt(s0, 0, "data:")) r = s0;
    if (U.slen(r) == 0) { let s1: char* = n.getAttr("data-src"); if (s1 != nullptr && U.slen(s1) > 0) r = s1; }
    if (U.slen(r) == 0) { let s2: char* = n.getAttr("data-lazy-src"); if (s2 != nullptr && U.slen(s2) > 0) r = s2; }
    if (U.slen(r) == 0) { let s3: char* = n.getAttr("srcset"); if (s3 != nullptr && U.slen(s3) > 0) r = firstSrcset(s3); }
    if (U.slen(r) == 0) { let s4: char* = n.getAttr("data-srcset"); if (s4 != nullptr && U.slen(s4) > 0) r = firstSrcset(s4); }
    return r;
}

// One per queued image (index = position in Browser.imgQ). The worker only touches url/buf/im/st; the Ui node stays
// on the main thread.
struct ImgSlot {
    st: Th.Atomic;
    epoch: i64 = 0;
    epochP: Th.Atomic* = nullptr;    // Browser.epoch: bumped on every navigation, so queued jobs of an old page skip
    netM: Th.Mutex* = nullptr;       // Browser.netM: Net keeps global state (lastFinal...), so one Net call at a time
    url: char* = nullptr;
    buf: Str.String;
    im: Img.Image;
}

// runs on a worker thread: no Ui, no Pen2D (GPU), no Dbg here
function imgJob(p: void*) => void {
    let s: ImgSlot* = p as ImgSlot*;
    if (s.epochP.load() != s.epoch) { s.st.store(K_SKIP); return; }
    let ok: bool = false;
    s.netM.lock();
    if (s.epochP.load() == s.epoch) ok = Net.loadBytes(s.url, &s.buf);
    s.netM.unlock();
    if (s.epochP.load() != s.epoch) { s.st.store(K_SKIP); return; }
    if (!ok) { s.st.store(K_FAIL); return; }
    if (s.im.load_memory(s.buf.data as u8*, s.buf.len as int)) {
        s.buf.free();
        s.st.store(K_RASTER);
    }
    else s.st.store(K_BYTES);                                    // maybe an SVG: the main thread rasterizes it
}

function hasText(s: char*, needle: char*) => bool {
    let n: int = U.slen(s);
    let i: int = 0;
    while (i < n) {
        if (U.startsAt(s, i, needle)) return true;
        i = i + 1;
    }
    return false;
}

// ---- CSS url() handling ----
// Every url(...) in a stylesheet is relative to THAT stylesheet, not to the page. Rewrite them to resolved URLs when
// the sheet is queued (base = the stylesheet's own URL, or the page URL for <style> blocks). data: and #fragment stay.
function absUrls(css: char*, base: char*) => char* {
    let o: Str.String;
    let n: int = U.slen(css);
    let i: int = 0;
    while (i < n) {
        if (U.startsAt(css, i, "url(")) {
            let j: int = i + 4;
            while (j < n && U.isSpace(css[j] as int)) { j = j + 1; }
            let q: int = css[j] as int;
            let quoted: bool = (q == 34 || q == 39);
            if (quoted) j = j + 1;
            let s: int = j;
            let done: bool = false;
            while (j < n && !done) {
                let ch: int = css[j] as int;
                if (quoted) { if (ch == q) done = true; else j = j + 1; }
                else { if (ch == 41 || U.isSpace(ch)) done = true; else j = j + 1; }
            }
            let raw: char* = U.sub(css, s, j);
            if (quoted) j = j + 1;
            while (j < n && U.isSpace(css[j] as int)) { j = j + 1; }
            if (j < n && (css[j] as int) == 41) j = j + 1;
            o.append("url(\"");
            if (U.startsAt(raw, 0, "data:") || U.startsAt(raw, 0, "#") || U.slen(raw) == 0) o.append(raw);
            else o.append(Net.resolve(base, raw));
            o.append("\")");
            i = j;
        }
        else { o.appendChar(css[i]); i = i + 1; }
    }
    let r: char* = U.dup(o.cstr());
    o.free();
    return r;
}

function b64v(c: int) => int {
    let r: int = 0 - 1;
    if (c >= 65 && c <= 90) r = c - 65;
    else if (c >= 97 && c <= 122) r = c - 97 + 26;
    else if (c >= 48 && c <= 57) r = c - 48 + 52;
    else if (c == 43 || c == 45) r = 62;
    else if (c == 47 || c == 95) r = 63;
    return r;
}

function hexv(c: int) => int {
    let r: int = 0 - 1;
    if (c >= 48 && c <= 57) r = c - 48;
    else if (c >= 97 && c <= 102) r = c - 97 + 10;
    else if (c >= 65 && c <= 70) r = c - 65 + 10;
    return r;
}

// data:[mime][;base64],payload  ->  raw bytes in `out` (base64 or percent-encoded). false when malformed.
function decodeDataUri(uri: char*, out: Str.String*) => bool {
    let n: int = U.slen(uri);
    let comma: int = 0 - 1;
    let i: int = 5;
    while (i < n && comma < 0) { if ((uri[i] as int) == 44) comma = i; i = i + 1; }
    if (comma < 0) return false;
    let isB64: bool = hasText(U.sub(uri, 5, comma), ";base64");
    let k: int = comma + 1;
    let acc: int = 0;                                           // no shift operators: keep a 16-bit window with * and %
    let bits: int = 0;
    while (k < n) {
        let c: int = uri[k] as int;
        if (c == 37 && k + 2 < n && hexv(uri[k + 1] as int) >= 0 && hexv(uri[k + 2] as int) >= 0) {
            let b: int = hexv(uri[k + 1] as int) * 16 + hexv(uri[k + 2] as int);
            if (isB64) {
                if (b == 61) { }                                 // %3D padding
            }
            else out.appendChar(b as char);
            k = k + 3;
        }
        else {
            if (isB64) {
                let v: int = b64v(c);
                if (v >= 0) {
                    acc = (acc * 64 + v) % 65536;
                    bits = bits + 6;
                    if (bits >= 8) {
                        bits = bits - 8;
                        let d: int = 1;
                        let t: int = 0;
                        while (t < bits) { d = d * 2; t = t + 1; }
                        out.appendChar(((acc / d) % 256) as char);
                    }
                }
            }
            else out.appendChar(c as char);
            k = k + 1;
        }
    }
    return out.len > 0;
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
    dlData: char* = nullptr;         // text of the shown download page (saved when its Download link is clicked)
    dlName: char* = nullptr;
    pendingBody: char* = nullptr;    // POST body for the pending load (nullptr = GET)
    navKind: int = 0;            // 0 push history, 1 history move (navPos), 2 reload
    navPos: int = 0;
    htmlSrc: char* = nullptr;
    jsAcc: char* = nullptr;

    // images: main.os sets `pen` (br.pen = ui.pen); without it <img> keeps its placeholder box
    pen: C.Pen2D* = nullptr;
    imgQ: [48]UI.Node*;
    bgQ: [64]UI.Node*;            // nodes whose CSS background-image url() still has to load (found after the cascade)
    bgN: int = 0;
    bgI: int = 0;
    bgEmpty: int = 0;             // empty url() backgrounds logged for this page (capped)
    // url -> decoded image, kept across pages (Pen2D images stay valid): the same sprite is fetched once
    bgcUrl: [48]char*;
    bgcId: [48]u32;
    bgcW: [48]float;
    bgcH: [48]float;
    bgcN: int = 0;
    imgN: int = 0;
    imgI: int = 0;
    imgDone: int = 0;
    slots: [48]ImgSlot;
    slotsUsed: int = 0;
    pool: Ch.ThreadPool;
    netM: Th.Mutex;              // serializes Net.* between the main thread and the image workers
    epoch: Th.Atomic;
    netOn: bool = false;
    workersOn: bool = false;

    // CSS handed to main.os, which owns the UI.Ui and applies it (ui.clearCss / ui.addCss) once per frame
    cssReset: bool = false;
    cssQ: [20]char*;
    qN: int = 0;
    viewW: int = 1000;           // viewport the @media conditions are evaluated against (set from the window size if you like)
    viewH: int = 700;
    // base = URL the sheet came from (its own URL for <link>, the page URL for <style>): url() is resolved against it
    queueCss(text: char*, base: char*) => void {
        if (this.qN < CSS_MAX) {
            let flat: char* = CF.flatten(absUrls(text, base), this.viewW, this.viewH);
            this.cssQ[this.qN] = flat;
            this.qN = this.qN + 1;
        }
        else Dbg.log(U.cat("CSS dropped: queue full: ", base));
    }

    // call once, after the Browser has its final address (main.os: right after `let b`)
    startWorkers() => void {
        if (this.netOn) return;
        this.netM.init();
        this.netOn = true;
        this.workersOn = this.pool.start(IMG_WORKERS);
        if (!this.workersOn) Dbg.log("image worker threads not started: loading images on the main thread");
    }

    // call after the main loop ends
    shutdown() => void {
        this.epoch.add(1);
        if (this.workersOn) { this.pool.shutdown(); this.workersOn = false; }
    }

    lockNet() => void { if (this.netOn) this.netM.lock(); }
    unlockNet() => void { if (this.netOn) this.netM.unlock(); }

    // hand every queued image to the workers (inline <svg> stays on the main thread)
    submitImages() => void {
        if (!this.workersOn) return;
        let ep: i64 = this.epoch.load();
        let i: int = 0;
        while (i < this.imgN) {
            let n: UI.Node* = this.imgQ[i];
            let s: ImgSlot* = &this.slots[i];
            s.epoch = ep;
            s.epochP = &this.epoch;
            s.netM = &this.netM;
            if (n.getAttr("data-osb-svg") != nullptr) s.st.store(K_SVG);
            else {
                s.url = U.dup(Net.resolve(this.url, imgSrc(n)));
                s.st.store(K_QUEUED);
                if (!this.pool.submit(imgJob, s as void*)) s.st.store(K_FAIL);
            }
            i = i + 1;
        }
        this.slotsUsed = this.imgN;
    }

    // main thread, once per frame: finish at most IMG_PER_FRAME images that the workers have ready
    consumeImages() => void {
        let budget: int = IMG_PER_FRAME;
        let i: int = 0;
        while (i < this.imgN && budget > 0) {
            let s: ImgSlot* = &this.slots[i];
            let st: i64 = s.st.load();
            if (st == K_SVG) {
                this.loadInlineSvg(this.imgQ[i], A.attr(this.imgQ[i], "data-osb-svg"));
                s.st.store(K_DONE); this.imgDone = this.imgDone + 1; budget = budget - 1;
            }
            else if (st == K_RASTER) {
                let n: UI.Node* = this.imgQ[i];
                let id: u32 = this.pen.createImage(s.im.pixels, s.im.width, s.im.height);
                let iw: float = s.im.width as float;
                let ih: float = s.im.height as float;
                s.im.free();                                     // Pen2D copied/uploaded the pixels
                if (id > 0) { n.imgId = id as int; n.imgW = iw; n.imgH = ih; n.markStyle(); }
                s.st.store(K_DONE); this.imgDone = this.imgDone + 1; budget = budget - 1;
            }
            else if (st == K_BYTES) {
                let n2: UI.Node* = this.imgQ[i];
                let iw2: float = 0.0;
                let ih2: float = 0.0;
                let id2: u32 = this.pen.createImageSvg(s.buf.data as u8*, s.buf.len as int, 2.0, 0x000000, &iw2, &ih2);
                if (id2 > 0) { n2.imgId = id2 as int; n2.imgW = iw2; n2.imgH = ih2; n2.markStyle(); }
                else Dbg.log(U.cat("image not decoded: ", s.url));
                s.im.free();
                s.buf.free();
                s.st.store(K_DONE); this.imgDone = this.imgDone + 1; budget = budget - 1;
            }
            else if (st == K_FAIL) {
                Dbg.log(U.cat("image not loaded: ", s.url));
                s.buf.free();
                s.st.store(K_DONE); this.imgDone = this.imgDone + 1;
            }
            else if (st == K_SKIP) { s.st.store(K_DONE); this.imgDone = this.imgDone + 1; }
            i = i + 1;
        }
    }

    // free whatever a finished-but-unconsumed slot still owns (only call when no job is running: after pool.wait())
    releaseSlots() => void {
        let i: int = 0;
        while (i < this.slotsUsed) {
            let s: ImgSlot* = &this.slots[i];
            let st: i64 = s.st.load();
            if (st == K_RASTER) s.im.free();
            else if (st == K_BYTES) { s.im.free(); s.buf.free(); }
            else if (st == K_FAIL) s.buf.free();
            s.st.store(K_IDLE);
            i = i + 1;
        }
        this.slotsUsed = 0;
    }

    startLoad(url: char*, kind: int, pos: int) => void {
        this.pendingUrl = url;
        this.imgN = 0; this.imgI = 0; this.imgDone = 0;   // drop the image queue of the page being left
        this.epoch.add(1);                     // queued/in-flight image jobs of the old page now skip themselves
        this.pendingBody = nullptr;
        this.navKind = kind;
        this.navPos = pos;
        this.state = S_FETCH;
        this.stageI = 0;
        this.loading = true;
        this.status = U.cat("Fetching ", url);
    }

    // <form> submission: GET puts the fields in the query string, POST sends them as the body. Queued like navigate().
    submitForm(form: UI.Node*) => void {
        if (form == nullptr || this.loading) return;
        let body: Str.String;
        collect(form, &body);
        let target: char* = this.url;
        let act: char* = form.getAttr("action");
        if (act != nullptr && U.slen(act) > 0) target = Net.resolve(this.url, act);
        if (target == nullptr) { body.free(); return; }
        let m: char* = U.lower(A.attr(form, "method"));
        if (U.streq(m, "post")) {
            this.startLoad(Net.stripFrag(target), 0, 0);
            this.pendingBody = U.dup(body.cstr());
        } else {
            let t: char* = Net.stripQuery(target);
            if (body.len > 0) t = U.cat(U.cat(t, "?"), body.cstr());
            this.startLoad(t, 0, 0);
        }
        body.free();
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
        let netSt: bool = (st == S_FETCH || st == S_CSS_LINK || st == S_JS_LINK);
        if (netSt) this.lockNet();                    // Net.* has global state shared with the image workers
        if (st == S_FETCH) {
            let html: char* = nullptr;
            if (this.pendingBody != nullptr) html = Net.post(this.pendingUrl, this.pendingBody);
            else html = Net.load(this.pendingUrl);
            this.pendingBody = nullptr;
            if (html != nullptr && U.slen(Net.lastFinal) > 0) this.pendingUrl = Net.lastFinal;     // after redirects
            if (html == nullptr) { this.fail(U.cat("Cannot load ", this.pendingUrl)); }
            else if (Net.lastDownload && !Net.lastText) {
                // a file that cannot be shown (pdf, zip, ...): save it and stay on the current page
                let saved: char* = Net.saveDownload(html, Net.lastName);
                if (saved == nullptr) this.status = U.cat("Could not save ", Net.lastName);
                else this.status = U.cat("Downloaded: ", saved);
                Dbg.log(this.status);
                this.loading = false;
                this.state = S_IDLE;
            }
            else {
                let asFile: bool = Net.lastDownload;                     // text attachment (csv...)
                let fname: char* = Net.lastName;
                if (!asFile && V.sniff(this.pendingUrl, Net.lastType)) { // .md/.json/.csv/... served inline or opened locally
                    asFile = true;
                    fname = V.nameOfUrl(this.pendingUrl);
                }
                if (asFile) {                                            // show it, with a Download link
                    this.dlData = html;
                    this.dlName = fname;
                    let shown: char* = V.render(html, fname);            // registry picks the viewer by extension
                    if (shown == nullptr) shown = viewPage(html, fname); // unknown type: plain <pre>
                    html = shown;
                }
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
            CF.reset();                                          // custom properties belong to one page
            this.cssReset = true;
            this.qN = 0;
            this.doc.cssN = 0;
            this.doc.jsN = 0;
            this.gen = this.gen + 1;
            H.parseHtml(this.htmlSrc, &this.doc);                // builds Ui nodes, not attached yet (no style cost)
            this.finishStage("parse html", S_CSS_INLINE, "Parsing CSS");
        }
        else if (st == S_CSS_INLINE) {
            this.queueCss(this.doc.css, this.url);
            this.stageI = 0;
            if (this.doc.cssN > 0) this.finishStage("inline css", S_CSS_LINK, "Fetching stylesheet 1");
            else { this.jsAcc = U.dup(""); this.finishStage("inline css", S_JS_LINK, "Loading scripts"); }
        }
        else if (st == S_CSS_LINK) {
            let sheet: char* = Net.load(Net.resolve(this.url, this.doc.cssLinks[this.stageI]));
            let cssUrl: char* = Net.resolve(this.url, this.doc.cssLinks[this.stageI]);
            Dbg.log(U.cat("stylesheet: ", cssUrl));
            if (sheet != nullptr) this.queueCss(sheet, cssUrl);
            else Dbg.log(U.cat("stylesheet failed: ", cssUrl));
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
                let jsUrl: char* = Net.resolve(this.url, this.doc.jsLinks[this.stageI]);
                let code: char* = Net.load(jsUrl);
                if (code != nullptr) this.jsAcc = U.cat(this.jsAcc, U.cat("\n/*@@", U.cat(jsUrl, U.cat("*/\n", U.cat(code, "\n")))));   // marker: error positions name the script
                this.stageI = this.stageI + 1;
                this.finishStage("script fetch", S_JS_LINK, "Loading scripts");
            }
            else { this.finishStage("scripts", S_JS_RUN, "Running scripts"); }
        }
        else if (st == S_JS_RUN) {
            let js: char* = U.cat("/*@@inline <script>*/\n", U.cat(this.doc.js, U.cat("\n", this.jsAcc)));
            if (U.slen(js) > JS_MAX) Dbg.log("scripts skipped: larger than JS_MAX");
            else if (U.slen(this.doc.js) + U.slen(this.jsAcc) > 0) {
                JS.setViewport(this.page.box.w as double, this.page.box.h as double);   // window.innerWidth / innerHeight
                JS.run(this.doc.root, js);                       // runs the scripts, then fires DOMContentLoaded + load
            }
            this.finishStage("run js", S_ATTACH, "Styling & layout");
        }
        else if (st == S_ATTACH) {
            // the heavy cascade + layout happens inside the next ui.frame(); the status above is already on screen
            this.page.append(this.doc.root);
            this.page.scrollTo(0.0, 0.0);
            if (this.workersOn) { this.pool.wait(); this.releaseSlots(); }   // old jobs skip instantly (epoch), so this is short
            this.imgN = 0; this.imgI = 0; this.imgDone = 0; this.bgN = 0; this.bgI = 0; this.bgEmpty = 0;
            this.collectImgs(this.doc.root);
            this.finishStage("attach", S_DONE, "Styling & layout");
        }
        else if (st == S_DONE) {
            this.loading = false;
            this.status = "";
            this.stageName = "style + layout";
            this.state = S_IDLE;
            if (this.pen != nullptr) this.collectBgs(this.page);          // the cascade has run, so st.bgUrl is known now
            if (this.pen != nullptr && (this.imgN > 0 || this.bgN > 0)) { this.state = S_IMG; this.status = "Loading images"; this.submitImages(); }
        }
        else if (st == S_IMG) {
            if (this.workersOn) {                                    // workers fetch + decode; we only upload, a few per frame
                this.consumeImages();
                if (this.imgDone >= this.imgN) { if (!this.stepBg()) { this.status = ""; this.state = S_IDLE; } }
                else this.status = U.cat("Loading images ", U.cat(U.fromInt(this.imgDone), U.cat("/", U.fromInt(this.imgN))));
            }
            else if (this.imgI >= this.imgN) { if (!this.stepBg()) { this.status = ""; this.state = S_IDLE; } }   // no threads: old one-per-frame path
            else {
                let n: UI.Node* = this.imgQ[this.imgI];
                this.imgI = this.imgI + 1;
                this.loadImage(n);
                this.status = U.cat("Loading images ", U.cat(U.fromInt(this.imgI), U.cat("/", U.fromInt(this.imgN))));
            }
        }
        if (netSt) this.unlockNet();
    }

    // <img src=...> and inline <svg> elements of the document, in order (at most IMG_MAX)
    collectImgs(n: UI.Node*) => void {
        let c: UI.Node* = n.firstEl();
        while (c != nullptr) {
            let isImg: bool = U.streq(A.tagOf(c), "img") && U.slen(imgSrc(c)) > 0;
            let isSvg: bool = c.hasAttr("data-osb-svg");
            if ((isImg || isSvg) && this.imgN < IMG_MAX) {
                this.imgQ[this.imgN] = c;
                this.imgN = this.imgN + 1;
            }
            this.collectImgs(c);
            c = c.nextEl();
        }
    }

    // elements whose computed style has a background-image url (at most 24, document order)
    collectBgs(n: UI.Node*) => void {
        let c: UI.Node* = n.firstEl();
        while (c != nullptr) {
            let u: char* = c.st.bgUrl;
            if (u != nullptr && U.slen(u) > 0) {
                if (c.bgImgUrl == nullptr || !U.streq(c.bgImgUrl, u)) {
                    if (this.bgN < BG_MAX) { this.bgQ[this.bgN] = c; this.bgN = this.bgN + 1; }
                    else Dbg.log(U.cat("background image skipped (BG_MAX): ", u));
                }
            }
            else if (u != nullptr && this.bgEmpty < 5) {              // url() with nothing in it: say so instead of failing silently
                this.bgEmpty = this.bgEmpty + 1;
                Dbg.log(U.cat("background-image has an EMPTY url() on <", U.cat(A.tagOf(c), ">")));
            }
            this.collectBgs(c);
            c = c.nextEl();
        }
    }

    // load the next queued background image (main thread, one per frame). Returns false when the queue is empty.
    stepBg() => bool {
        if (this.bgI >= this.bgN) return false;
        let n: UI.Node* = this.bgQ[this.bgI];
        this.bgI = this.bgI + 1;
        let u: char* = n.st.bgUrl;
        if (u != nullptr) {
            // 1. cache hit: the same image was already decoded (this page or an earlier one)
            let hit: int = 0 - 1;
            let ci: int = 0;
            while (ci < this.bgcN && hit < 0) { if (U.streq(this.bgcUrl[ci], u)) hit = ci; ci = ci + 1; }
            if (hit >= 0) {
                n.bgImgId = this.bgcId[hit] as int; n.bgImgW = this.bgcW[hit]; n.bgImgH = this.bgcH[hit];
                n.bgImgUrl = U.dup(u); n.markPaint();
            }
            else {
                // 2. get the bytes: data: URI, else the URL as rewritten by absUrls, else relative to the page
                //    (style="" attributes are not rewritten)
                let buf: Str.String;
                let got: bool = false;
                let abs: char* = u;
                if (U.startsAt(u, 0, "data:")) got = decodeDataUri(u, &buf);
                else {
                    got = Net.loadBytes(u, &buf);
                    if (!got) { abs = Net.resolve(this.url, u); got = Net.loadBytes(abs, &buf); }
                }
                if (got) {
                    let id: u32 = 0; let iw: float = 0.0; let ih: float = 0.0;
                    let im: Img.Image;
                    if (im.load_memory(buf.data as u8*, buf.len as int)) {
                        id = this.pen.createImage(im.pixels, im.width, im.height);
                        iw = im.width as float; ih = im.height as float;
                    }
                    else { id = this.pen.createImageSvg(buf.data as u8*, buf.len as int, 2.0, 0x000000, &iw, &ih); }
                    im.free();
                    if (id > 0) {
                        n.bgImgId = id as int; n.bgImgW = iw; n.bgImgH = ih; n.bgImgUrl = U.dup(u); n.markPaint();
                        if (this.bgcN < BGC_MAX) {
                            let e: int = this.bgcN;
                            this.bgcUrl[e] = U.dup(u); this.bgcId[e] = id; this.bgcW[e] = iw; this.bgcH[e] = ih;
                            this.bgcN = e + 1;
                        }
                    }
                    else Dbg.log(U.cat("background image not decoded: ", U.cat(abs, U.cat(" (", U.cat(U.fromInt(buf.len as int), " bytes)")))));
                }
                else Dbg.log(U.cat("background image not loaded: ", abs));
                buf.free();
            }
        }
        this.status = U.cat("Loading backgrounds ", U.cat(U.fromInt(this.bgI), U.cat("/", U.fromInt(this.bgN))));
        return true;
    }

    // inline <svg>: the parser stored its source text in data-osb-svg
    loadInlineSvg(n: UI.Node*, src: char*) => void {
        let iw: float = 0.0;
        let ih: float = 0.0;
        let id: u32 = this.pen.createImageSvg(src as u8*, U.slen(src), 2.0, 0x000000, &iw, &ih);
        if (id > 0) {
            n.imgId = id as int;
            n.imgW = iw;
            n.imgH = ih;
            n.markStyle();
        }
        else Dbg.log("inline svg not rasterized");
    }

    // fetch + decode one image and attach it to its node (Paint draws it once imgId is set; layout uses imgW/imgH)
    // raster formats go through the stb decoder; if that fails the bytes are tried as SVG (lunasvg)
    loadImage(n: UI.Node*) => void {
        let inl: char* = n.getAttr("data-osb-svg");
        if (inl != nullptr) { this.loadInlineSvg(n, inl); return; }
        let src: char* = imgSrc(n);
        if (U.slen(src) == 0) return;
        let abs: char* = Net.resolve(this.url, src);
        let buf: Str.String;
        if (Net.loadBytes(abs, &buf)) {
            let id: u32 = 0;
            let iw: float = 0.0;
            let ih: float = 0.0;
            let im: Img.Image;
            if (im.load_memory(buf.data as u8*, buf.len as int)) {
                id = this.pen.createImage(im.pixels, im.width, im.height);
                iw = im.width as float;
                ih = im.height as float;
            }
            else {
                id = this.pen.createImageSvg(buf.data as u8*, buf.len as int, 2.0, 0x000000, &iw, &ih);
                if (id == 0) Dbg.log(U.cat("image not decoded: ", abs));
            }
            im.free();                                           // Pen2D copied/uploaded the pixels
            if (id > 0) {
                n.imgId = id as int;
                n.imgW = iw;
                n.imgH = ih;
                n.markStyle();
            }
        }
        else Dbg.log(U.cat("image not loaded: ", abs));
        buf.free();
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

    // the Download link of a shown text attachment
    saveShown() => void {
        if (this.dlData == nullptr) return;
        this.lockNet();
        let saved: char* = Net.saveDownload(this.dlData, this.dlName);
        this.unlockNet();
        if (saved == nullptr) this.status = U.cat("Could not save ", this.dlName);
        else this.status = U.cat("Downloaded: ", saved);
        Dbg.log(this.status);
    }

    // after ui.frame(), when a click event was seen: link navigation + JS click listeners
    handleClicks(ms: double) => void {
        if (this.doc.root != nullptr && !this.loading) {
            let link: UI.Node* = clickedLink(this.doc.root);
            if (link != nullptr && U.streq(A.attr(link, "href"), "#save-download")) this.saveShown();
            else if (link != nullptr) this.navigate(Net.resolve(this.url, link.getAttr("href")));
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

    tick(ms: double) => void {
        if (this.loading) return;
        JS.setViewport(this.page.box.w as double, this.page.box.h as double);   // a change fires "resize"
        JS.tick(ms);
    }
}
