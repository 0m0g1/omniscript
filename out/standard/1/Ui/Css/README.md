# Css engine layout

`standard/1/Ui/Css.os` is only an umbrella: it `include`s the files below in dependency order, so `Ui.os` is unchanged.
The tree mirrors Blink's pipeline (parse -> rule set -> match -> cascade -> apply property):

```
Css.os                       umbrella (include order = dependency order)
Css/
  Core/
    Constants.os             AP_* / SN_* / SRC_* codes, extra PSE_* bits
    State.os                 every Css global (parser flags, var scope, media environment) + cssSetRecord / cssSetMedia
    Strings.os               containsStr, trimDup, firstToken, nthToken (paren-aware), readIdent, skipCssWs ...
  Diagnostics/
    Notes.os                 StyleNote recording for the inspector
  Values/                    CSS value grammar (Blink: css/parser + CSSValue)
    Color.os  Length.os  Calc.os  Grid.os  Font.os  Keywords.os
    Var.os                   custom properties + var()
  Media/
    MediaQuery.os            @media evaluation          Supports.os   @supports evaluation
  Parser/                    text -> data (Blink: CSSParser, StyleSheetContents, RuleSet)
    Declarations.os          "a: b; c: d !important" -> Attr list
    Selectors.os             selector text -> Compound chain, specificity
    AtRules.os               which @-blocks are entered
    StyleSheet.os            rules, sheetParse / sheetParseBlock, sheetClear
  Selectors/                 Blink: SelectorChecker
    Pseudo.os                bit-flag pseudo-classes     Matcher.os    matchCompound / matchChain
  Properties/                Blink: CSSProperty::ApplyValue, one file per area
    Text Box Border Paint Display Flex Grid Misc .os    applyProp<Group>(...) -> AP_NOMATCH if the name is not theirs
    Property.os              applyProp dispatcher, cssSupportedProps, cssIsSupported
  Cascade/                   Blink: StyleCascade / StyleResolver
    Apply.os                 declaration lists -> Style (var() substitution, !important filtering)
    Defaults.os              tag-row defaults        Important.os  !important pass        Cascade.os  cascadeNode
```

## Adding things

* **A property**: add one `else if (strEq(name, "...")) { ... }` branch to the matching `Properties/<Group>.os`, add the name
  to `cssSupportedProps()` in `Property.os`. Nothing else changes. A new area (e.g. `Transform`) = new file + one line in
  `applyProp` + one `include` in `Css.os`.
* **A value type**: `Values/`. **A pseudo-class** that is a plain flag: `Selectors/Pseudo.os`.
* **An at-rule**: one branch in `Parser/AtRules.os`.
* Module rules: a file only calls files above it in the include list, except that functions are hoisted (as in the
  original single file). Globals live only in `Core/State.os`. Recursive functions only call themselves.

## What is new in this split

| Feature | Where | Notes |
|---|---|---|
| `var(--x, fallback)`, custom properties, inheritance | Values/Var.os, Cascade | scope chain per element, nested var(), cycle guard |
| `@media`, `@supports`, `@layer` | Media/, Parser/AtRules.os | evaluated at parse time; call `cssSetMedia(w, h, dark)` first, re-parse sheets on resize |
| `!important` | Parser/Declarations.os, Cascade/Important.os | author origin only |
| `min()` `max()` `clamp()` | Values/Calc.os | `%` mixed with px is approximated (px wins) |
| `repeat(auto-fill/auto-fit, minmax())` | Values/Grid.os | column count estimated from the viewport width |
| `padding: 0 clamp(...)` and other shorthands with function values | Values/Length.os (`side4`) | paren-aware tokens |
| units `dvh svh lvh dvw svw lvw vmin vmax ch ex` | Values/Length.os, Calc.os | |
| `:only-child :empty :first-of-type :last-of-type :link :focus-visible`, `:root` also matches `html` | Selectors/ | |
| `place-items`, `place-self` | Properties/Flex.os | align axis only |

## Hook-up checklist (files outside Css that I could not see)

1. Before parsing author sheets: `cssSetMedia(viewportWidthPx, viewportHeightPx, true)`; re-run `sheetClear` + `sheetParse` on resize.
2. Nothing else: `cascadeNode`, `sheetParse`, `applyDeclText`, `cssSetRecord` keep their signatures.
3. The code has not been compiled. If the compiler rejects a construct, the fix is local to one small function.

## Next steps that need changes outside Css

| Gap seen on godotengine.org | Needs |
|---|---|
| rounded corners, `box-shadow` | Paint: rounded rect in Pen2D (`Style.radius` is already stored) |
| `background-image`, gradients, `object-fit`, `aspect-ratio` | Style fields + Paint + Layout |
| `font-family`, `letter-spacing`, `text-transform` | Style fields + font backend |
| `:not()`, `:is()`, `:nth-child()` | `Compound` fields in Types.os (negated / list / a,b) |
| `gap: row col` (separate axes) | `Style.gapX/gapY` |
| `position: sticky`, `::before/::after`, `@container` | Layout / Tree |
| `(c) 2007` shows `??` | HTML parser: UTF-8 / entity decoding (not CSS) |
