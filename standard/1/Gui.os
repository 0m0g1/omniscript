// standard/1/Gui.os  -- STUB (not implemented yet)
// Tier 7: immediate-mode GUI (buttons, labels, text input, sliders, layout) drawn through Canvas/OpenGL + Font + Input.
// All functions return false / do nothing until implemented.
struct Gui {
    begin() => void { }                                           // TODO: reset layout, read Input
    label(text: char*) => void { }
    button(text: char*) => bool { return false; }
    slider(label: char*, v: float*, lo: float, hi: float) => bool { return false; }
    textInput(buf: char*, cap: u64) => bool { return false; }
    end() => void { }                                             // TODO: flush draw lists
}
