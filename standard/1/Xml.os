// standard/1/Xml.os
// Tier 4: minimal XML DOM (elements, attributes, text, comments skipped, entities &lt; &gt; &amp; &quot; &apos;). No DTD/namespaces.
module Str = import "standard/1/String.os";
module L = import "standard/1/List.os";
module CMem = import "standard/1/c/mem.os";
module CStr = import "standard/1/c/string.os";
module Text = import "standard/1/Text.os";

struct Attr { name: Str.String; value: Str.String; }

struct XmlNode {
    name: Str.String;
    text: Str.String;
    attrs: L.List<Attr>;
    children: L.List<XmlNode*>;

    free() => void {
        let i: u64 = 0;
        while (i < this.children.len) { let c = this.children.data[i]; c.free(); CMem.free(c as void*); i = i + 1; }
        this.children.free();
        i = 0;
        while (i < this.attrs.len) { this.attrs.data[i].name.free(); this.attrs.data[i].value.free(); i = i + 1; }
        this.attrs.free();
        this.name.free(); this.text.free();
    }

    attr(n: char*) => char* {
        let i: u64 = 0;
        while (i < this.attrs.len) { if (this.attrs.data[i].name.equalsC(n)) return this.attrs.data[i].value.cstr(); i = i + 1; }
        return nullptr;
    }
    child(n: char*) => XmlNode* {
        let i: u64 = 0;
        while (i < this.children.len) { if (this.children.data[i].name.equalsC(n)) return this.children.data[i]; i = i + 1; }
        return nullptr;
    }
}

struct P { s: char* = nullptr; i: u64 = 0; err: bool = false; }

function skipWs(p: P*) => void { while (Text.isSpace(p.s[p.i] as int)) p.i = p.i + 1; }
function startsWith(p: P*, lit: char*) => bool { return CStr.strncmp((p.s as u64 + p.i) as char*, lit, CStr.strlen(lit)) == 0; }

function readName(p: P*, out: Str.String*) => void {
    while (Text.isAlnum(p.s[p.i] as int) || (p.s[p.i] as int) == 95 || (p.s[p.i] as int) == 45 || (p.s[p.i] as int) == 58 || (p.s[p.i] as int) == 46) {
        out.appendChar(p.s[p.i]); p.i = p.i + 1;
    }
}

function decodeAppend(p: P*, n: u64, out: Str.String*) => void {
    let end: u64 = p.i + n;
    while (p.i < end) {
        if (startsWith(p, "&lt;")) { out.appendChar(60 as char); p.i = p.i + 4; }
        else if (startsWith(p, "&gt;")) { out.appendChar(62 as char); p.i = p.i + 4; }
        else if (startsWith(p, "&amp;")) { out.appendChar(38 as char); p.i = p.i + 5; }
        else if (startsWith(p, "&quot;")) { out.appendChar(34 as char); p.i = p.i + 6; }
        else if (startsWith(p, "&apos;")) { out.appendChar(39 as char); p.i = p.i + 6; }
        else { out.appendChar(p.s[p.i]); p.i = p.i + 1; }
    }
}

function parseElement(p: P*) => XmlNode* {
    // at '<'
    p.i = p.i + 1;
    let n = CMem.calloc(1, sizeof(XmlNode)) as XmlNode*;
    readName(p, &n.name);
    while (true) {
        skipWs(p);
        let c: int = p.s[p.i] as int;
        if (c == 0) { p.err = true; return n; }
        if (c == 47) { p.i = p.i + 2; return n; }        // "/>"
        if (c == 62) { p.i = p.i + 1; break; }
        let a: Attr;
        readName(p, &a.name);
        skipWs(p);
        if ((p.s[p.i] as int) == 61) {
            p.i = p.i + 1; skipWs(p);
            let q: int = p.s[p.i] as int;
            p.i = p.i + 1;
            let st: u64 = p.i;
            while ((p.s[p.i] as int) != q && (p.s[p.i] as int) != 0) p.i = p.i + 1;
            let save: u64 = p.i;
            p.i = st;
            decodeAppend(p, save - st, &a.value);
            p.i = save + 1;
        }
        n.attrs.push(a);
    }
    // content
    while (true) {
        let c: int = p.s[p.i] as int;
        if (c == 0) { p.err = true; return n; }
        if (c == 60) {
            if (startsWith(p, "</")) {
                while ((p.s[p.i] as int) != 62 && (p.s[p.i] as int) != 0) p.i = p.i + 1;
                p.i = p.i + 1;
                n.text.trim();
                return n;
            }
            if (startsWith(p, "<!--")) {
                while (!startsWith(p, "-->") && (p.s[p.i] as int) != 0) p.i = p.i + 1;
                p.i = p.i + 3; continue;
            }
            if (startsWith(p, "<![CDATA[")) {
                p.i = p.i + 9;
                while (!startsWith(p, "]]>") && (p.s[p.i] as int) != 0) { n.text.appendChar(p.s[p.i]); p.i = p.i + 1; }
                p.i = p.i + 3; continue;
            }
            let ch = parseElement(p);
            n.children.push(ch);
            if (p.err) return n;
        } else {
            let st: u64 = p.i;
            while ((p.s[p.i] as int) != 60 && (p.s[p.i] as int) != 0) p.i = p.i + 1;
            let save: u64 = p.i;
            p.i = st;
            decodeAppend(p, save - st, &n.text);
            p.i = save;
        }
    }
    return n;
}

// Returns root element or nullptr. Caller: root.free(); CMem.free(root).
function parse(text: char*) => XmlNode* {
    let p: P; p.s = text;
    while (true) {
        skipWs(&p);
        if (startsWith(&p, "<?")) { while (!startsWith(&p, "?>") && (p.s[p.i] as int) != 0) p.i = p.i + 1; p.i = p.i + 2; continue; }
        if (startsWith(&p, "<!--")) { while (!startsWith(&p, "-->") && (p.s[p.i] as int) != 0) p.i = p.i + 1; p.i = p.i + 3; continue; }
        if (startsWith(&p, "<!")) { while ((p.s[p.i] as int) != 62 && (p.s[p.i] as int) != 0) p.i = p.i + 1; p.i = p.i + 1; continue; }
        break;
    }
    if ((p.s[p.i] as int) != 60) return nullptr;
    let r = parseElement(&p);
    if (p.err) { r.free(); CMem.free(r as void*); return nullptr; }
    return r;
}

function escapeText(s: char*, out: Str.String*) => void {
    let i: u64 = 0;
    while ((s[i] as int) != 0) {
        let c: int = s[i] as int;
        if (c == 60) out.append("&lt;"); else if (c == 62) out.append("&gt;");
        else if (c == 38) out.append("&amp;"); else if (c == 34) out.append("&quot;");
        else out.appendChar(c as char);
        i = i + 1;
    }
}

function write(n: XmlNode*, out: Str.String*, depth: u64) => void {
    Text.repeat("  ", depth, out);
    out.append("<"); out.appendString(&n.name);
    let i: u64 = 0;
    while (i < n.attrs.len) {
        out.append(" "); out.appendString(&n.attrs.data[i].name); out.append("=\"");
        escapeText(n.attrs.data[i].value.cstr(), out); out.append("\"");
        i = i + 1;
    }
    if (n.children.len == 0 && n.text.len == 0) { out.append("/>\n"); return; }
    out.append(">");
    if (n.children.len == 0) { escapeText(n.text.cstr(), out); }
    else {
        out.append("\n");
        i = 0;
        while (i < n.children.len) { write(n.children.data[i], out, depth + 1); i = i + 1; }
        Text.repeat("  ", depth, out);
    }
    out.append("</"); out.appendString(&n.name); out.append(">\n");
}
