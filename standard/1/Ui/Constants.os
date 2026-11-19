// standard/1/Ui/Constants.os
// Node kinds, display modes, tag specials, control kinds, CSS enums, event ids, display-list ops.

let UNSET: float = 0.0 - 1.0;          // "not set" sentinel for floats (no negative literals in the language)

// ---- node kinds ----
let NK_BOX: int = 0;                   // element laid out by the engine
let NK_TEXT: int = 1;                  // text run
let NK_CONTROL: int = 2;               // button, input, ... (wraps a Pen2D widget or is drawn natively)
let NK_REPLACED: int = 3;              // img, canvas, video ... (intrinsic size)
let NK_HIDDEN: int = 4;                // head, script, style ... (never laid out)

// ---- display ----
let DP_NONE: int = 0;
let DP_BLOCK: int = 1;
let DP_INLINE: int = 2;
let DP_INLINE_BLOCK: int = 3;
let DP_FLEX: int = 4;
let DP_TABLE: int = 5;
let DP_GRID: int = 6;                  // NEXT: grid layout; falls back to block
let DP_TABLE_ROW: int = 7;
let DP_TABLE_CELL: int = 8;
let DP_TABLE_GROUP: int = 9;           // thead / tbody / tfoot

// ---- tag specials (what a tag needs beyond its row defaults) ----
let SP_NONE: int = 0;     let SP_BR: int = 1;       let SP_WBR: int = 2;      let SP_HR: int = 3;
let SP_LI: int = 4;       let SP_UL: int = 5;       let SP_OL: int = 6;       let SP_PRE: int = 7;
let SP_A: int = 8;        let SP_IMG: int = 9;      let SP_INPUT: int = 10;   let SP_BUTTON: int = 11;
let SP_TEXTAREA: int = 12; let SP_SELECT: int = 13; let SP_OPTION: int = 14;  let SP_LABEL: int = 15;
let SP_FORM: int = 16;    let SP_FIELDSET: int = 17; let SP_DETAILS: int = 18; let SP_SUMMARY: int = 19;
let SP_DIALOG: int = 20;  let SP_PROGRESS: int = 21; let SP_METER: int = 22;  let SP_CANVAS: int = 23;
let SP_TABLE: int = 24;   let SP_HTML: int = 25;    let SP_BODY: int = 26;

// ---- control kinds ----
let CK_NONE: int = 0;     let CK_BUTTON: int = 1;   let CK_TEXT: int = 2;     let CK_CHECK: int = 3;
let CK_RADIO: int = 4;    let CK_RANGE: int = 5;    let CK_PROGRESS: int = 6; let CK_METER: int = 7;
let CK_SELECT: int = 8;   let CK_TEXTAREA: int = 9; let CK_STUB: int = 10;

// ---- input types ----
let IT_TEXT: int = 0;     let IT_PASSWORD: int = 1; let IT_SEARCH: int = 2;   let IT_EMAIL: int = 3;
let IT_NUMBER: int = 4;   let IT_URL: int = 5;      let IT_TEL: int = 6;      let IT_CHECKBOX: int = 7;
let IT_RADIO: int = 8;    let IT_RANGE: int = 9;    let IT_BUTTON: int = 10;  let IT_SUBMIT: int = 11;
let IT_RESET: int = 12;   let IT_HIDDEN: int = 13;  let IT_STUB: int = 14;    // file, color, date, time ...

// ---- length units ----
let LU_AUTO: int = 0;     let LU_PX: int = 1;       let LU_PCT: int = 2;      let LU_NONE: int = 3;

// ---- css enums ----
let PS_STATIC: int = 0;   let PS_RELATIVE: int = 1; let PS_ABSOLUTE: int = 2; let PS_FIXED: int = 3;
let TA_LEFT: int = 0;     let TA_CENTER: int = 1;   let TA_RIGHT: int = 2;    let TA_JUSTIFY: int = 3;
let WS_NORMAL: int = 0;   let WS_NOWRAP: int = 1;   let WS_PRE: int = 2;      let WS_PREWRAP: int = 3;   let WS_PRELINE: int = 4;
let OV_VISIBLE: int = 0;  let OV_HIDDEN: int = 1;   let OV_SCROLL: int = 2;   let OV_AUTO: int = 3;
let FD_ROW: int = 0;      let FD_COLUMN: int = 1;   let FD_ROW_REV: int = 2;  let FD_COL_REV: int = 3;
let AL_STRETCH: int = 0;  let AL_START: int = 1;    let AL_END: int = 2;      let AL_CENTER: int = 3;    let AL_AUTO: int = 9;
let JC_START: int = 0;    let JC_END: int = 1;      let JC_CENTER: int = 2;   let JC_BETWEEN: int = 3;
let JC_AROUND: int = 4;   let JC_EVENLY: int = 5;
let CU_DEFAULT: int = 0;  let CU_POINTER: int = 1;  let CU_TEXT: int = 2;
let LS_DISC: int = 0;     let LS_NONE: int = 1;     let LS_DECIMAL: int = 2;  let LS_SQUARE: int = 3;    let LS_CIRCLE: int = 4;

// ---- selector pieces ----
let COMB_DESC: int = 0;   let COMB_CHILD: int = 1;  let COMB_ADJ: int = 2;    let COMB_SIB: int = 3;     let COMB_NONE: int = 4;
let PSE_HOVER: int = 1;   let PSE_FOCUS: int = 2;   let PSE_ACTIVE: int = 4;  let PSE_CHECKED: int = 8;
let PSE_DISABLED: int = 16; let PSE_ENABLED: int = 32; let PSE_FIRST: int = 64; let PSE_LAST: int = 128; let PSE_ROOT: int = 256;
let AOP_EXISTS: int = 0;  let AOP_EQ: int = 1;      let AOP_WORD: int = 2;    let AOP_PREFIX: int = 3;
let AOP_SUFFIX: int = 4;  let AOP_SUB: int = 5;

// ---- events ----
let EV_CLICK: int = 1;    let EV_HOVER: int = 2;    let EV_LEAVE: int = 3;    let EV_KEY: int = 4;
let EV_INPUT: int = 5;    let EV_CHANGE: int = 6;   let EV_FOCUS: int = 7;    let EV_BLUR: int = 8;
let EV_SUBMIT: int = 9;   let EV_MOUSEDOWN: int = 10; let EV_MOUSEUP: int = 11;
let EV_QCAP: int = 128;

// ---- display list ops ----
let CM_FILL: int = 1;     let CM_BORDER: int = 2;   let CM_TEXT: int = 3;     let CM_PUSHCLIP: int = 4;
let CM_POPCLIP: int = 5;  let CM_IMAGE: int = 6;    let CM_MARK: int = 7;
let TF_BOLD: int = 1;     let TF_ITALIC: int = 2;   let TF_UNDERLINE: int = 4; let TF_STRIKE: int = 8;

// ---- backends ----
let BK_GUI: int = 0;      let BK_TUI: int = 1;

// ---- default theme (dark, to match the Canvas widgets). Override with CSS or ui.setTheme(). ----
let DEF_TEXT: u32 = 0xE8E8F0FF;
let DEF_BG: u32 = 0x1E1E26FF;
let DEF_LINK: u32 = 0x7FA6FFFF;
let DEF_RULE: u32 = 0x55556AFF;
let DEF_MARK_BG: u32 = 0xFFE066FF;
let DEF_MARK_FG: u32 = 0x111111FF;
let DEF_DIM: u32 = 0x6A6A7AFF;
let DEF_ACCENT: u32 = 0x5B7CFAFF;
