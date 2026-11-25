// standard/1/Ui/Css/Cascade/Defaults.os
// Tag defaults (Tags.os row) applied on top of inherited values.

// ---- cascade ----
// Tag defaults (Tags.os row) applied on top of inherited values.
function applyTagDefaults(st: Style*, row: TagRow*, parentPx: float, ctx: CssCtx*) => void {
    st.display = row.display;
    let px: float = parentPx;
    if (!ctx.tui) px = parentPx * row.scale;
    st.fontPx = px;
    if (row.bold) st.bold = true;
    if (row.italic) st.italic = true;
    if (row.underline) st.underline = true;
    if (row.strike) st.strike = true;
    if (row.ws != WS_NORMAL) st.whiteSpace = row.ws;
    if (row.taSet) st.textAlign = row.ta;
    if (row.color != 0) st.color = row.color;
    if (row.bg != 0) st.bg = row.bg;
    let u: float = ctx.unit;
    lenSet(&st.mt, LU_PX, row.mtEm * px + row.mtPx * u);
    lenSet(&st.mb, LU_PX, row.mbEm * px + row.mbPx * u);
    lenSet(&st.ml, LU_PX, row.mlPx * u);
    lenSet(&st.mr, LU_PX, row.mrPx * u);
    lenSet(&st.pl, LU_PX, (row.plPx + row.padPx) * u);
    lenSet(&st.pt, LU_PX, row.padPx * u);
    lenSet(&st.pr, LU_PX, row.padPx * u);
    lenSet(&st.pb, LU_PX, row.padPx * u);
    if (row.bPx > 0.0) {
        st.bt = row.bPx * u; st.br = row.bPx * u; st.bb = row.bPx * u; st.bl = row.bPx * u;
        st.borderColor = DEF_RULE;
    }
}
