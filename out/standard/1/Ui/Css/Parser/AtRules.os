// standard/1/Ui/Css/Parser/AtRules.os
// Which at-rule blocks sheetParseBlock descends into (Blink: CSSParserImpl::ConsumeAtRule).
//   @media     entered when the query matches now          @supports  entered when the condition holds
//   @layer     entered (flattened: layer order is NOT modelled, unlayered rules do not beat layered ones yet)
//   @font-face @keyframes @property @page @container @scope @starting-style ... skipped
//   @import @charset @namespace  statements are skipped
// To support another conditional group rule, add one branch here.
function atRuleEnter(prelude: char*) => bool {
    let r: bool = false;
    let n: int = strLen(prelude);
    if (eqAtI(prelude, 0, "@media")) { let c: char* = strSub(prelude, 6, n - 6); r = mediaMatches(c); strFree(c); }
    else if (eqAtI(prelude, 0, "@supports")) { let c2: char* = strSub(prelude, 9, n - 9); r = supportsMatches(c2); strFree(c2); }
    else if (eqAtI(prelude, 0, "@layer")) { r = true; }
    return r;
}
