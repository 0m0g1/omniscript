// standard/1/Ui/Css.os
// CSS engine umbrella. `include` pastes each file in place, so the order below is the dependency order.
// Layout and how to extend it: standard/1/Ui/Css/README.md
//
// Pipeline (Blink names in brackets):
//   Parser/   text -> declarations, selectors, rules           [CSSParser, StyleSheetContents, RuleSet]
//   Values/   value grammar: colours, lengths, calc/min/max/clamp, grid tracks, var()   [CSSValue]
//   Media/    @media, @supports evaluation                      [MediaQueryEvaluator]
//   Selectors/ matching                                         [SelectorChecker]
//   Properties/ one file per property area; Property.os dispatches   [CSSProperty::ApplyValue]
//   Cascade/  tag defaults -> rules -> inline -> !important     [StyleCascade, StyleResolver]
//   Diagnostics/ StyleNote recording for the inspector
//
// Properties: see Properties/*.os. Selectors: tag .class #id * [attr ops] descendant > + ~, :hover :focus :focus-visible
//   :active :checked :disabled :enabled :first-child :last-child :only-child :empty :first-of-type :last-of-type :link :root.
// At-rules: @media @supports @layer (entered), everything else skipped.   NEXT: :not() :nth-child() ::before.

include "standard/1/Ui/Css/Core/Constants.os";
include "standard/1/Ui/Css/Core/State.os";
include "standard/1/Ui/Css/Core/Strings.os";
include "standard/1/Ui/Css/Diagnostics/Notes.os";
include "standard/1/Ui/Css/Values/Color.os";
include "standard/1/Ui/Css/Values/Calc.os";
include "standard/1/Ui/Css/Values/Length.os";
include "standard/1/Ui/Css/Values/Keywords.os";
include "standard/1/Ui/Css/Values/Grid.os";
include "standard/1/Ui/Css/Values/Font.os";
include "standard/1/Ui/Css/Values/Var.os";
include "standard/1/Ui/Css/Media/MediaQuery.os";
include "standard/1/Ui/Css/Media/Supports.os";
include "standard/1/Ui/Css/Parser/Declarations.os";
include "standard/1/Ui/Css/Parser/Selectors.os";
include "standard/1/Ui/Css/Parser/AtRules.os";
include "standard/1/Ui/Css/Parser/StyleSheet.os";
include "standard/1/Ui/Css/Selectors/Pseudo.os";
include "standard/1/Ui/Css/Selectors/Matcher.os";
include "standard/1/Ui/Css/Properties/Text.os";
include "standard/1/Ui/Css/Properties/Box.os";
include "standard/1/Ui/Css/Properties/Border.os";
include "standard/1/Ui/Css/Properties/Paint.os";
include "standard/1/Ui/Css/Properties/Display.os";
include "standard/1/Ui/Css/Properties/Flex.os";
include "standard/1/Ui/Css/Properties/Grid.os";
include "standard/1/Ui/Css/Properties/Misc.os";
include "standard/1/Ui/Css/Properties/Property.os";
include "standard/1/Ui/Css/Cascade/Apply.os";
include "standard/1/Ui/Css/Cascade/Defaults.os";
include "standard/1/Ui/Css/Cascade/Important.os";
include "standard/1/Ui/Css/Cascade/Cascade.os";
