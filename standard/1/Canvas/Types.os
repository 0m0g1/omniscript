struct DrawCmd {
    tex: u32; prog: u32;
    clipOn: int; cx: int; cy: int; cw: int; ch: int;
    first: u32; count: u32;
    rw: float; rh: float;
}


// Filled by Win every frame, so Pen2D never needs to know about Win (and still works headless).
struct PenInput {
    mx: float = 0.0; my: float = 0.0;
    down: bool = false; pressed: bool = false; released: bool = false;
    typed: char* = nullptr;             // NUL-terminated chars typed this frame
    backspace: bool = false;            // backspace/del/arrows/home/end are key-repeat events
    enter: bool = false;
    kDel: bool = false;
    left: bool = false; right: bool = false; up: bool = false; kDown: bool = false;   // (mouse button is `down`)
    kHome: bool = false; kEnd: bool = false;
    tab: bool = false; space: bool = false; esc: bool = false;
    shift: bool = false; ctrl: bool = false;
    selAll: bool = false; copy: bool = false; cut: bool = false; paste: bool = false;
    dbl: bool = false;                  // the mouse press this frame was a double click
    time: double = 0.0;                 // seconds (caret blink)
}

// One struct for every widget kind (no polymorphism needed). Read the state fields, call setText to change text.
struct Widget {
    kind: int = 0;
    x: float = 0.0; y: float = 0.0; w: float = 0.0; h: float = 0.0;
    text: char* = nullptr;              // label/button/checkbox caption; textbox: its own editable buffer
    visible: bool = true;
    enabled: bool = true;
    // state, refreshed every frame
    hovered: bool = false;
    down: bool = false;                 // held
    pressed: bool = false;              // click completed this frame (button, checkbox)
    released: bool = false;
    changed: bool = false;              // checkbox toggled / slider moved / textbox edited this frame
    submitted: bool = false;            // textbox: Enter pressed this frame
    focused: bool = false;              // textbox
    checked: bool = false;              // checkbox
    value: float = 0.0; min: float = 0.0; max: float = 1.0;    // slider
    len: int = 0;                       // textbox length
    owned: bool = false;                // text is a malloc'd buffer this widget frees
    cur: int = 0; anc: int = 0;         // textbox caret and selection anchor (selection = [min, max) of the two)
    scrollX: float = 0.0;               // textbox horizontal scroll in px
    blinkT: double = 0.0;               // last caret activity (keeps the caret solid while typing)
    hint: char* = nullptr;              // textbox placeholder shown while empty

    // Textbox: copies s into its own buffer. Other kinds: stores the pointer (s must outlive the widget).
    setText(s: char*) => void {
        if (this.owned) {
            let i: int = 0;
            while (s[i] as int != 0 && i < TEXTBOX_CAP - 1) { this.text[i] = s[i]; i = i + 1; }
            this.text[i] = 0 as char;
            this.len = i; this.cur = i; this.anc = i; this.scrollX = 0.0;
        } else { this.text = s; }
    }
    getText() => char* { return this.text; }
    selectAll() => void { this.anc = 0; this.cur = this.len; }
    show() => void { this.visible = true; }
    hide() => void { this.visible = false; }
}
