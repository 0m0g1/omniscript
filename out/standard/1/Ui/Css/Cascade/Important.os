// standard/1/Ui/Css/Cascade/Important.os
// !important: a second pass over the matching rules and the inline style, run only when some declaration used it
// (gSawImportant). Important declarations are stored as "!name" by parseDecls and skipped by the normal pass.
// Order inside the pass follows the normal one (specificity, source order, inline last). Author-only: there is no
// user-agent / user origin yet, so the CSS rule that important UA beats important author is not modelled.
function cascadeImportant(n: Node*, sheet: Sheet*, ctx: CssCtx*, parentPx: float, nn: Node*) => void {
    gImportantPass = true;
    let r: Rule* = sheet.head;
    while (r != nullptr) {
        if (matchChain(n, r.sel)) {
            ctx.curPx = parentPx; applyDeclsRec(&n.st, r.decls, ctx, 0, nn, SRC_AUTHOR, r.selText);
            ctx.curPx = n.st.fontPx; applyDeclsRec(&n.st, r.decls, ctx, 1, nn, SRC_AUTHOR, r.selText);
        }
        r = r.next;
    }
    if (n.inlineCss != nullptr) applyDeclTextRec(&n.st, n.inlineCss, ctx, parentPx, nn, SRC_INLINE, nullptr);
    gImportantPass = false;
}
