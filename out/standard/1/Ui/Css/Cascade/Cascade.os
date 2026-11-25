// standard/1/Ui/Css/Cascade/Cascade.os
// Cascade order: inherited props -> tag defaults -> sheet rules by (specificity, source order) -> inline style ->
// !important pass (Cascade/Important.os) -> API fields (assigned in app code).
// Custom-property scopes (Values/Var.os) are pushed before the element's own declarations and popped after its children.

function cascadeNode(n: Node*, parent: Style*, sheet: Sheet*, ctx: CssCtx*, force: bool) => void {
    let need: bool = force || n.dirtyStyle;
    // custom properties: a scope is pushed for every element that declares any (see Values/Var.os)
    let ownsChain: bool = false;
    if (gSheetHasVars && gScope == nullptr && gCascadeDepth == 0 && n.parent != nullptr) { gScope = varChain(n.parent, sheet); ownsChain = true; }
    gCascadeDepth = gCascadeDepth + 1;
    let pushed: bool = false;
    if (gSheetHasVars && (need || n.subStyle) && n.kind != NK_TEXT) pushed = varPush(n, sheet);
    if (need) {
        if (n.styleNotes != nullptr) { freeStyleNotes(n.styleNotes); n.styleNotes = nullptr; }     // refilled below when recording
        n.st.reset();
        n.st.inheritFrom(parent);
        if (n.kind == NK_TEXT) {
            n.st.display = DP_INLINE;
        } else {
            let nn: Node* = nullptr;                       // non-null only while ui.recordStyles is on
            if (gRecordStyles) nn = n;
            applyTagDefaults(&n.st, &n.row, parent.fontPx, ctx);
            if (nn != nullptr) noteTagDefaults(n);
            let r: Rule* = sheet.head;
            while (r != nullptr) {
                if (matchChain(n, r.sel)) {
                    ctx.curPx = parent.fontPx; applyDeclsRec(&n.st, r.decls, ctx, 0, nn, SRC_AUTHOR, r.selText);
                    ctx.curPx = n.st.fontPx;   applyDeclsRec(&n.st, r.decls, ctx, 1, nn, SRC_AUTHOR, r.selText);
                }
                r = r.next;
            }
            if (n.inlineCss != nullptr) applyDeclTextRec(&n.st, n.inlineCss, ctx, parent.fontPx, nn, SRC_INLINE, nullptr);
            if (gSawImportant) cascadeImportant(n, sheet, ctx, parent.fontPx, nn);   // Cascade/Important.os
            if (nn != nullptr) noteResolveOverrides(n);
            ctx.curPx = n.st.fontPx;
            // API fields (assigned in app code) win over CSS
            let u: float = ctx.unit;
            if (n.gap >= 0.0) n.st.gap = n.gap * u;
            if (n.padding >= 0.0) {
                lenSet(&n.st.pt, LU_PX, n.padding * u); lenSet(&n.st.pr, LU_PX, n.padding * u);
                lenSet(&n.st.pb, LU_PX, n.padding * u); lenSet(&n.st.pl, LU_PX, n.padding * u);
            }
            if (n.grow >= 0.0) n.st.grow = n.grow;
            if (n.width >= 0.0) lenSet(&n.st.width, LU_PX, n.width * u);
            if (n.height >= 0.0) lenSet(&n.st.height, LU_PX, n.height * u);
            // element rules that CSS cannot override
            if (n.kind == NK_HIDDEN) n.st.display = DP_NONE;
            if (n.hasAttr("hidden")) n.st.display = DP_NONE;
            if (n.row.spec == SP_DIALOG) {
                if (!n.open) n.st.display = DP_NONE;
                else if (n.st.position == PS_STATIC) n.st.position = PS_FIXED;      // open dialog = centred overlay (Abs.os)
            }
            if (n.row.spec == SP_INPUT && n.itype == IT_HIDDEN) n.st.display = DP_NONE;
        }
        n.dirtyStyle = false;
    }
    if (need || n.subStyle) {
        let c: Node* = n.first;
        while (c != nullptr) { cascadeNode(c, &n.st, sheet, ctx, force || need); c = c.next; }
    }
    n.subStyle = false;
    if (pushed) varPop();
    gCascadeDepth = gCascadeDepth - 1;
    if (ownsChain) { varFreeChain(gScope); gScope = nullptr; }
}
