let API_AUTO: int = 0 - 1;
let API_GL: int = 0;
let API_VULKAN: int = 1;
let API_SOFTWARE: int = 2;
let MAX_IMAGES: int = 64;
let MAX_WINDOWS: int = 8;

let WK_BUTTON: int = 0;
let WK_CHECKBOX: int = 1;
let WK_SLIDER: int = 2;
let WK_LABEL: int = 3;
let WK_TEXTBOX: int = 4;
let MAX_WIDGETS: int = 256;
let TEXTBOX_CAP: int = 128;

let UI_IDLE: u32 = 0x3A3A46FF;
let UI_HOT: u32 = 0x4A4A5CFF;
let UI_ACTIVE: u32 = 0x5B7CFAFF;
let UI_PANEL: u32 = 0x2A2A33FF;
let UI_BORDER: u32 = 0x55556AFF;
let UI_TEXT: u32 = 0xE8E8F0FF;
let UI_DIM: u32 = 0x6A6A7AFF;
let UI_SEL: u32 = 0x3A5BC8FF;       // text selection background
let UI_FOCUS: u32 = 0x8FA8FFFF;     // keyboard focus ring

let KEY_FIRST: int = 32;
let KEY_LAST: int = 348;
let KEY_COUNT: int = 349;
// GLFW key codes used by the widget input
let GK_ESC: int = 256;      let GK_TAB: int = 258;     let GK_DELETE: int = 261;
let GK_RIGHT: int = 262;    let GK_LEFT: int = 263;    let GK_DOWN: int = 264;   let GK_UP: int = 265;
let GK_HOME: int = 268;     let GK_END: int = 269;     let GK_KPENTER: int = 335;
let GK_LALT: int = 342;     let GK_RALT: int = 346;
let GK_A: int = 65;         let GK_C: int = 67;        let GK_V: int = 86;       let GK_X: int = 88;
let REPEAT_DELAY: double = 0.4;
let REPEAT_RATE: double = 0.035;
let DBLCLICK_TIME: double = 0.35;
