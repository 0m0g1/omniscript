    // standard/1/Ui/Api.os   (fragment: pasted INSIDE `struct Node { ... }` by Tree.os)
    // App-facing sugar. Every method returns the node it created, so calls chain and fields can be set right away:
    //   let bar = root.row();  bar.gap = 8.0;  bar.padding = 8.0;
    //   let ok = bar.button("OK");  let name = bar.input();  name.grow = 1.0;
    // row/column/panel are flex boxes with a default style, the same code path as CSS `display: flex`.
    // gap / padding / grow / width / height are applied after the CSS cascade; assign them right after creation,
    // or call node.dirty() if you change them later. Controls expose pressed / changed / submitted / value / checked / text.

    tagged(name: char*) => Node* { return this.newChild(name); }

    panel() => Node* {
        let n: Node* = this.newChild("div");
        n.style("display: flex; flex-direction: column;");
        return n;
    }
    row() => Node* {
        let n: Node* = this.newChild("div");
        n.style("display: flex; flex-direction: row;");
        return n;
    }
    column() => Node* {
        let n: Node* = this.newChild("div");
        n.style("display: flex; flex-direction: column;");
        return n;
    }
    box() => Node* { return this.newChild("div"); }                 // plain block
    spacer(g: float) => Node* {
        let n: Node* = this.newChild("div");
        n.grow = g;
        return n;
    }
    separator() => Node* { return this.newChild("hr"); }

    label(s: char*) => Node* {
        let n: Node* = this.newChild("span");
        n.addText(s);
        return n;
    }
    heading(level: int, s: char*) => Node* {
        let name: char* = "h1";
        if (level == 2) name = "h2";
        else if (level == 3) name = "h3";
        else if (level == 4) name = "h4";
        else if (level == 5) name = "h5";
        else if (level >= 6) name = "h6";
        let n: Node* = this.newChild(name);
        n.addText(s);
        return n;
    }
    paragraph(s: char*) => Node* {
        let n: Node* = this.newChild("p");
        n.addText(s);
        return n;
    }
    link(s: char*, href: char*) => Node* {
        let n: Node* = this.newChild("a");
        n.setAttr("href", href);
        n.addText(s);
        return n;
    }
    image(id: int, w: float, h: float) => Node* {
        let n: Node* = this.newChild("img");
        n.imgId = id; n.imgW = w; n.imgH = h;
        return n;
    }

    // ---- controls ----
    button(s: char*) => Node* {
        let n: Node* = this.newChild("button");
        n.text = strDup(s); n.ownsText = true;
        return n;
    }
    input() => Node* {
        let n: Node* = this.newChild("input");
        n.setAttr("type", "text");
        return n;
    }
    password() => Node* {
        let n: Node* = this.newChild("input");
        n.setAttr("type", "password");
        return n;
    }
    // returns the <input>; the caption is a text run after it, inside a <label> so clicking the text toggles it
    checkbox(s: char*) => Node* {
        let l: Node* = this.newChild("label");
        let n: Node* = l.newChild("input");
        n.setAttr("type", "checkbox");
        l.addText(s);
        return n;
    }
    radio(group: char*, s: char*) => Node* {
        let l: Node* = this.newChild("label");
        let n: Node* = l.newChild("input");
        n.setAttr("type", "radio");
        n.setAttr("name", group);
        l.addText(s);
        return n;
    }
    range(lo: float, hi: float) => Node* {
        let n: Node* = this.newChild("input");
        n.setAttr("type", "range");
        n.min = lo; n.max = hi; n.value = lo; n.valueDirty = true;
        return n;
    }
    progress(v: float) => Node* {
        let n: Node* = this.newChild("progress");
        n.min = 0.0; n.max = 1.0; n.value = v;
        return n;
    }
    meter(lo: float, hi: float, v: float) => Node* {
        let n: Node* = this.newChild("meter");
        n.min = lo; n.max = hi; n.value = v;
        return n;
    }
    // <select> showing option labels separated by '|' ("Red|Green|Blue"); clicking cycles the choice (NEXT: popup list)
    select(options: char*) => Node* {
        let n: Node* = this.newChild("select");
        n.setAttr("options", options);
        return n;
    }
    details(summary: char*) => Node* {
        let n: Node* = this.newChild("details");
        let s: Node* = n.newChild("summary");
        s.addText(summary);
        return n;
    }
    dialog() => Node* { return this.newChild("dialog"); }
    show() => void { this.open = true; this.markStyle(); }
    hide() => void { this.open = false; this.markStyle(); }
