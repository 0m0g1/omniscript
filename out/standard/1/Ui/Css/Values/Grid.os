// standard/1/Ui/Css/Values/Grid.os
// grid-template-columns track lists.

// ---- grid-template-columns ----
// Tracks are stored on the Style (Style.gn / g0..g11): px and % are fixed, `fr` and `auto` are flexible (LU_AUTO, v = weight).
function gridPush(st: Style*, u: int, v: float) => void {
    let t: Len; lenSet(&t, u, v);
    let i: int = st.gn;
    if (i == 0) copyLen(&st.g0, &t);
    else if (i == 1) copyLen(&st.g1, &t);
    else if (i == 2) copyLen(&st.g2, &t);
    else if (i == 3) copyLen(&st.g3, &t);
    else if (i == 4) copyLen(&st.g4, &t);
    else if (i == 5) copyLen(&st.g5, &t);
    else if (i == 6) copyLen(&st.g6, &t);
    else if (i == 7) copyLen(&st.g7, &t);
    else if (i == 8) copyLen(&st.g8, &t);
    else if (i == 9) copyLen(&st.g9, &t);
    else if (i == 10) copyLen(&st.g10, &t);
    else copyLen(&st.g11, &t);
    st.gn = i + 1;
}

function gridTrack(st: Style*, i: int, out: Len*) => void {
    if (i == 0) copyLen(out, &st.g0);
    else if (i == 1) copyLen(out, &st.g1);
    else if (i == 2) copyLen(out, &st.g2);
    else if (i == 3) copyLen(out, &st.g3);
    else if (i == 4) copyLen(out, &st.g4);
    else if (i == 5) copyLen(out, &st.g5);
    else if (i == 6) copyLen(out, &st.g6);
    else if (i == 7) copyLen(out, &st.g7);
    else if (i == 8) copyLen(out, &st.g8);
    else if (i == 9) copyLen(out, &st.g9);
    else if (i == 10) copyLen(out, &st.g10);
    else copyLen(out, &st.g11);
}

// "1fr 200px repeat(2, 1fr) minmax(100px, 1fr) auto" -> tracks appended to st. false when a form is not understood
// (repeat(auto-fill / auto-fit, ...), named lines, fit-content, more than 12 tracks).
function gridParseTracks(v: char*, ctx: CssCtx*, st: Style*) => bool {
    let n: int = strLen(v);
    let i: int = 0; let ok: bool = true;
    while (i < n && ok) {
        let c: int = v[i] as int;
        if (c == 32 || c == 9 || c == 10 || c == 13) { i = i + 1; }
        else {
            let a: int = i; let depth: int = 0; let scan: bool = true;
            while (i < n && scan) {
                let d: int = v[i] as int;
                if (d == 40) depth = depth + 1;
                else if (d == 41) depth = depth - 1;
                if (depth == 0 && (d == 32 || d == 9 || d == 10 || d == 13)) scan = false;
                else i = i + 1;
            }
            let tok: char* = trimDup(v, a, i);
            let tl: int = strLen(tok);
            let isRep: bool = startsWith(tok, "repeat(");
            let isMm: bool = startsWith(tok, "minmax(");
            if (isRep || isMm) {
                let cm: int = 0 - 1; let k: int = 7;
                while (k < tl && cm < 0) { if ((tok[k] as int) == 44) cm = k; k = k + 1; }
                if (cm < 0 || (tok[tl - 1] as int) != 41) ok = false;
                else if (isMm) {                                              // minmax(min, max): the max decides
                    let mx: char* = trimDup(tok, cm + 1, tl - 1);
                    if (!gridParseTracks(mx, ctx, st)) ok = false;
                    strFree(mx);
                }
                else {
                    let ns: char* = trimDup(tok, 7, cm);
                    let cnt: int = parseFloatStr(ns, 0.0) as int;             // auto-fill / auto-fit parse as 0
                    let inner: char* = trimDup(tok, cm + 1, tl - 1);
                    if (strEqI(ns, "auto-fill") || strEqI(ns, "auto-fit")) cnt = gridAutoCount(inner, ctx, st);   // see Values/Grid.os
                    strFree(ns);
                    if (cnt < 1) ok = false;
                    else {
                        let r: int = 0;
                        while (r < cnt && ok) { if (!gridParseTracks(inner, ctx, st)) ok = false; r = r + 1; }
                    }
                    strFree(inner);
                }
            }
            else if (st.gn >= 12) { ok = false; }
            else if (tl > 2 && (tok[tl - 2] as int) == 102 && (tok[tl - 1] as int) == 114) {     // "<n>fr"
                let num: char* = trimDup(tok, 0, tl - 2);
                gridPush(st, LU_AUTO, parseFloatStr(num, 1.0));
                strFree(num);
            }
            else if (strEqI(tok, "auto") || strEqI(tok, "min-content") || strEqI(tok, "max-content")) { gridPush(st, LU_AUTO, 1.0); }
            else {
                let gl: Len; lenSet(&gl, LU_AUTO, 0.0);
                if (parseLen(tok, ctx, &gl) && (gl.u == LU_PX || gl.u == LU_PCT)) gridPush(st, gl.u, gl.v);
                else ok = false;
            }
            strFree(tok);
        }
    }
    return ok;
}

// repeat(auto-fill | auto-fit, <track>): the container width is unknown at cascade time, so the column count is
// estimated from the viewport: floor((viewport - 32px + gap) / (min track + gap)), clamped to 1..12. `inner` is the
// repeated track list ("minmax(280px, 1fr)", "240px"). 0 when the minimum cannot be read as px.
// NEXT: resolve against the real container in layout (Layout/Grid) and drop this estimate.
function gridAutoCount(inner: char*, ctx: CssCtx*, st: Style*) => int {
    let cols: int = 0;
    let minTok: char* = nullptr;
    if (startsWith(inner, "minmax(")) {
        let il: int = strLen(inner); let cm: int = 0 - 1; let k: int = 7;
        while (k < il && cm < 0) { if ((inner[k] as int) == 44) cm = k; k = k + 1; }
        if (cm > 0) minTok = trimDup(inner, 7, cm);
    }
    else { minTok = firstToken(inner); }
    if (minTok != nullptr) {
        let ml: Len; lenSet(&ml, LU_AUTO, 0.0);
        if (parseLen(minTok, ctx, &ml) && ml.u == LU_PX && ml.v > 0.0) {
            let gp: float = st.gap;
            if (gp <= 0.0) gp = 16.0 * ctx.unit;
            let avail: float = ctx.vw - 32.0 * ctx.unit + gp;
            cols = (avail / (ml.v + gp)) as int;
            if (cols < 1) cols = 1;
            if (cols > 12) cols = 12;
        }
        strFree(minTok);
    }
    return cols;
}
