# Ui.os: build guide (HTML-grade UI library for OmniScript)

Goal: one retained UI tree + style + layout + events library in std. The browser, your apps and a TUI all sit on it.
The browser shrinks to: HTML parser + JS engine + URL loader.

## 1. Where it lives and how it is split

Same pattern as Canvas: a small root file that `include`s parts (order matters).

```
standard/1/Ui.os                  root: module imports + includes
standard/1/Ui/
  Constants.os      node kinds, display modes, tag ids, key/event ids
  Types.os          Style, Box, Node, Event, Metrics
  Tags.os           the tag table (name -> kind, default display, default style)  <-- the 115 elements live here
  Tree.os           create/append/remove/find, createByTag("div")
  Css.os            CSS text parser, selector match, cascade, applyProp   (moved from browser/css/css.os)
  Measure.os        text width/height; one function, backend-provided via Metrics
  Layout/Block.os   block + inline flow                                   (moved from browser/layout/layout.os)
  Layout/Flex.os    row/column, gap, grow, align, justify, wrap
  Layout/Table.os   table, tr, td, th, colgroup
  Layout/Abs.os     position: absolute/relative/fixed, z-index
  Layout/Grid.os    later
  Paint.os          Node tree -> display list                             (moved from browser/paint/paint.os)
  Events.os         hit test, hover, focus, tab order, bubbling, click/key/input/change
  Controls/         Button, Input, Checkbox, Radio, Range, Select, Textarea, Progress, Meter, Details, Dialog
  Backend/Gui.os    display list -> Pen2D                                 (moved from browser/paint/backend.os)
  Backend/Tui.os    display list -> character grid on the console (ANSI)
  Backend/Soft.os   optional: headless screenshots for tests (Pen2D.initSoftware already gives you this)
  Api.os            sugar: ui.panel(), node.row(), node.column(), node.button("OK") ...
```

Rules copied from Canvas: Ui never touches GL; it only talks to `Pen2D`, `Win` and `Console`. Add it to `std.os` Tier 7 as
`public module Ui = import "standard/1/Ui.os";` and remove the commented-out `Gui.os` line (Ui replaces it; see section 9).
Layering: Canvas (draw + input) -> Ui (tree, style, layout, events, controls) -> Browser and your apps.

Browser after the move:

```
browser/net/loader.os     stays
browser/html/html.os      stays, but calls Ui.createByTag(name) instead of D.newElement
browser/js/js.os          stays, binds to Ui.Node
browser/browser.os        stays: load -> parse -> Ui.styleTree -> Ui.layout -> Ui.paint -> Ui.Backend
browser/dom, css, layout, paint   DELETED after their code moves into Ui/
```

## 2. Core design decisions (hard to change later, so decide now)

1. **Retained tree, dirty flags.** Node persists; `node.dirtyStyle`, `node.dirtyLayout`, `node.dirtyPaint`. Setting a field marks it.
2. **Two layout families, one engine.** `display` is one of: `none, block, inline, inline-block, flex, table, grid`.
   `Row`/`Column`/`Panel` are just flex boxes with a default style, so `display:flex` in CSS and `pen.row()` use the same code.
3. **Backend-neutral units + Metrics.** Layout runs in float units. `Metrics { cellW, cellH, lineH, unit }`.
   GUI: unit = 1px, text measured with `pen.textWidth`. TUI: unit = 1 cell, `cellW = cellH = 1`. Default paddings/margins scale by `Metrics.unit`.
   This is what makes a TUI mode possible: only `Measure` and `Backend` change.
4. **Display list is the seam.** Ui emits `Cmd{rect, text, border, clip push/pop, image}`; each backend executes it.
   Gui backend -> `pen.fillRect/textScaled/pushClip`. Tui backend -> write cells into a char+color grid, flush with ANSI.
5. **Text measurement is the foundation.** One `Measure.text(s, size)`. Back it with `pen.textWidth` for now, `Font.glyph().advance` later
   (Font gives `advance`, `lineHeight()`; that also enables bold/italic via extra Font files).
6. **Controls wrap Pen2D widgets first, get replaced later.**
   Pen2D widgets are absolute-positioned (`addWidget(kind,x,y,w,h)`), capped at `MAX_WIDGETS = 256`, hit-tested in creation order, and drawn by
   `drawWidgets()`. Phase 1: a control node owns a `Widget*` and each frame layout writes `wd.x/y/w/h` (and `wd.visible = false` when scrolled out
   or `display:none`). Phase 3: native controls drawn through the display list, so they work in TUI and clip properly. Keep the same
   field names (`pressed`, `changed`, `submitted`, `value`, `checked`, `text`) so app code does not change.
7. **Every tag exists on day one.** Unsupported tags map to a generic box/inline with a `// NEXT:` comment. Pages never crash on unknown tags.
8. **Language quirks to design around:** no `return` inside loop bodies, no negative literals (`0.0 - 1.0`), `include` order matters,
   `sizeof(T)` + `CMem.malloc/memset` for allocation, `as` for casts.

## 3. The tag table (`Ui/Tags.os`)

One row per tag: `{ name, kind, display, defaultStyle, controlKind }`. Node kinds: `BOX, TEXT, CONTROL, REPLACED, HIDDEN`.

| Group | Elements | Maps to | Layout | Phase |
|---|---|---|---|---|
| Root / metadata | html, head, title, base, link, meta, style, script, noscript, template, slot | HIDDEN (html = BOX root) | none | 0 (link/style/script consumed by the browser) |
| Sections | body, article, section, nav, aside, header, footer, main, address, hgroup, search | BOX | block | 0 |
| Headings | h1 h2 h3 h4 h5 h6 | BOX + font-size/margin defaults | block | 0 (h1-h3 exist; add h4-h6) |
| Grouping | div, p, pre, blockquote, hr, figure, figcaption | BOX (pre: white-space, hr: border line) | block | 0; pre/hr in 2 |
| Lists | ul, ol, menu, li, dl, dt, dd | BOX; li draws marker (`-`, number, term) | block | 0 (ul/li), 2 (ol numbering, dl) |
| Text semantics | span, a, em, strong, b, i, u, s, small, mark, code, kbd, samp, var, cite, q, dfn, abbr, time, data, sub, sup, bdi, bdo, ruby, rt, rp, wbr | TEXT-inline styles | inline | 0 basics; 2 bold/italic/underline/strike/mono need font + decoration support |
| Breaks | br, wbr | TEXT line-break | inline | 0 |
| Edits | ins, del | inline + underline / strike | inline | 2 |
| Tables | table, caption, colgroup, col, thead, tbody, tfoot, tr, th, td | BOX | table | 3 |
| Forms: text | input(text/password/search/email/number/url/tel), textarea | CONTROL (Textbox) | inline-block | 1 (input), 3 (textarea multi-line) |
| Forms: toggles | input(checkbox/radio), button, input(button/submit/reset) | CONTROL | inline-block | 1 |
| Forms: ranges | input(range), progress, meter | CONTROL (Slider / bar) | inline-block | 1 |
| Forms: choice | select, option, optgroup, datalist | CONTROL (dropdown overlay) | inline-block + popup | 3 |
| Forms: structure | form, label, fieldset, legend, output | BOX (label forwards clicks, form collects values, fieldset border) | block/inline | 3 |
| Forms: misc | input(file, color, date, time, ...) | stub: text box | inline-block | 4 |
| Interactive | details, summary, dialog | BOX with open/close state; dialog = overlay (Abs) | block | 3 |
| Embedded | img, picture, source | REPLACED (Image.os + `pen.createImage`) | inline-block | 3 |
| Embedded | canvas | REPLACED: a Pen2D target | inline-block | 4 |
| Embedded | svg, math, iframe, embed, object, video, audio, track, map, area | REPLACED placeholder box with size, draws "[video]" etc. | inline-block | 4+ |

That is roughly 115 tags. Almost all are the same five node kinds with different defaults, so full coverage costs one table row each,
not 115 widgets.

## 4. CSS properties by phase (`Ui/Css.os`)

- **0 (have now):** color, background-color, font-size, width, height, margin, padding, border (uniform), display (none/block/inline).
- **1:** display:flex, flex-direction, gap, flex-grow, flex-shrink, align-items, justify-content, align-self.
- **2:** per-side margin/padding/border (`margin-top`...), `rgb()/rgba()/hsl()`, full named colors, text-align, line-height, font-weight, font-style,
  text-decoration, white-space, min/max-width/height, box-sizing, overflow (hidden/auto with scroll), opacity, visibility, `%`/`em`/`rem`/`vw`/`vh`.
- **3:** position (relative/absolute/fixed), z-index, border-radius, table layout props, cursor, list-style.
- **4:** display:grid, transitions, `@media`, `var()`, pseudo-classes (`:hover :focus :active :checked :disabled`), `:nth-child`.
- Selectors: have tag/.class/#id/descendant/comma; add `>`, `+`, `~`, `[attr]`, pseudo-classes in phase 4.

## 5. Events (`Ui/Events.os`)

- Per-node handlers: `onClick`, `onHover`, `onKey`, `onInput`, `onChange`, `onFocus`, `onBlur` (function index/pointer + user data).
- Dispatch: hit test deepest node -> capture/bubble up the parent chain. Hover/focus state written to the node so `:hover`/`:focus` CSS can use it.
- Focus order: tab order = tree order (Pen2D already does creation order in `focusNext`); `tabindex` later.
- Browser `addEventListener` maps straight onto these.

## 6. App-facing API sketch (`Ui/Api.os`)

```omniscript
let ui: Ui.Ui;                       // owns root, metrics, backend
ui.init(win);                        // GUI;  ui.initTui() for terminal
let root = ui.root();
let bar  = root.row();   bar.gap = 8.0;  bar.padding = 8.0;
let ok   = bar.button("OK");
let name = bar.input();  name.grow = 1.0;
let list = root.column();
let item = list.panel(); item.style("padding: 4px; background: #333;");
item.label("hello");
while (app.frame()) { ui.frame(); app.present(); }   // layout if dirty, events, paint
if (ok.pressed) { ... }
```

Nodes expose the same flag names as Pen2D widgets (`pressed`, `changed`, `submitted`, `value`, `checked`, `text`).

## 7. Migration order (browser keeps working at every step)

1. **Move, don't change.** Create `Ui.os`, `Ui/Types.os`, `Ui/Tree.os`; move `dom.os` -> Types/Tree, `css.os` -> `Ui/Css.os`, `layout.os` -> `Ui/Layout/Block.os`,
   `paint.os` -> `Ui/Paint.os`, `backend.os` -> `Ui/Backend/Gui.os`. Point `browser.os` and `html.os` at `Ui`. Test page must look identical.
2. **Measure + Metrics.** Add `Ui/Measure.os`, replace the `CHAR_W` guess with `pen.textWidth`. Introduce `Metrics` (unit = 1px).
3. **Tag table.** Add `Ui/Tags.os`, make `html.os` call `Ui.createByTag`. Fill every group in section 3 with at least a default row.
4. **Flex.** `Ui/Layout/Flex.os`, CSS flex properties, `Api.os` row/column/panel. Write a pure-OS demo (no HTML) using the API.
5. **Controls.** Wrap Pen2D widgets (button, input, checkbox, range), layout-driven positions, events + focus. Wire JS `addEventListener` to Ui events.
6. **Text + box upgrades.** Per-side box model, text-align, bold/italic via Font variants, overflow + scrolling (`pushClip`).
7. **Tables, forms, details, dialog, images.**
8. **TUI backend.** `Metrics.unit = 1 cell`, `Backend/Tui.os`; reuse everything above it unchanged.
9. **Native controls, grid, media stubs, pseudo-classes.**

## 8. Testing

- Keep `browser/test/*.html` as regression pages; add one page per phase (flex.html, forms.html, tables.html).
- Headless: `Pen2D.initSoftware(w,h)` + compare pixel samples, like your `widgets_test.os` does for widgets.
- One OS-only demo per phase that builds the UI through `Api.os`.

## 9. Decisions (locked)

| Decision | Choice | Consequence |
|---|---|---|
| Module relationship | **Ui is its own module on top of Canvas**; Canvas stays low-level (windows, input, drawing, backends) | Ui imports Canvas, never the reverse. Games/3D import only Canvas. TUI and headless backends can replace Canvas without touching layout. |
| API style | **Methods on nodes** (`bar.button("OK")`, `root.row()`) | `Api.os` adds methods to `Node`; no free-function twin API. |
| Flex | **Early**, built in step 4 right after the move + Measure + tag table | `row()/column()/panel()` are flex boxes; CSS `display:flex` shares the code path. |
| Controls | **Wrap Pen2D widgets first**, replace with native Ui controls one at a time later (button, then checkbox/range, text box last) | Layout writes `wd.x/y/w/h/visible` each frame. Field names stay (`pressed`, `changed`, `submitted`, `value`, `checked`, `text`) so app code survives the swap. Known limits until replaced: Pen2D-only, 256 cap, no clipping in scroll areas, no TUI. |
| `Gui.os` | **Retired.** `std.os` gets `public module Ui = import "standard/1/Ui.os";` and the commented `Gui` line is removed | No alias file. |

### What Ui needs from Canvas (keep stable)

- `pen.textWidth(s, scale)`, `pen.textHeight(scale)`: used by `Ui/Measure.os`.
- `pen.fillRect`, `pen.rect`, `pen.textScaled`, `pen.pushClip/popClip`, `pen.image`: used by `Ui/Backend/Gui.os`.
- Widget API (`createButton/Checkbox/Slider/Label/TextBox`, `destroyWidget`, widget fields, `wantsMouse`): used by the wrapped controls.
- `Win` input (`mx`, `my`, `mousePressed`, `keyPressed`, `typedText`): used by `Ui/Events.os`.
- Next Canvas wish-list: a font-size-aware measure (or `Font.glyph().advance`) so layout stops guessing glyph width.

### Next action

Step 1 of section 7: move the browser's DOM/CSS/layout/paint/backend code into `standard/1/Ui.os` + `standard/1/Ui/*.os`, one file per message, browser test page must render identically afterwards.