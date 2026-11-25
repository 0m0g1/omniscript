// standard/1/Ui/Css/Values/Keywords.os
// Keyword -> enum helpers shared by several property groups.

function setDisplay(st: Style*, v: char*) => bool {
    let ok: bool = true;
    if (strEqI(v, "none")) st.display = DP_NONE;
    else if (strEqI(v, "block") || strEqI(v, "list-item") || strEqI(v, "contents")) st.display = DP_BLOCK;   // NEXT: contents
    else if (strEqI(v, "inline")) st.display = DP_INLINE;
    else if (strEqI(v, "inline-block")) st.display = DP_INLINE_BLOCK;
    else if (strEqI(v, "flex") || strEqI(v, "inline-flex")) st.display = DP_FLEX;
    else if (strEqI(v, "table") || strEqI(v, "inline-table")) st.display = DP_TABLE;
    else if (strEqI(v, "table-row")) st.display = DP_TABLE_ROW;
    else if (strEqI(v, "table-cell")) st.display = DP_TABLE_CELL;
    else if (strEqI(v, "table-row-group") || strEqI(v, "table-header-group") || strEqI(v, "table-footer-group")) st.display = DP_TABLE_GROUP;
    else if (strEqI(v, "grid") || strEqI(v, "inline-grid")) st.display = DP_GRID;
    else ok = false;
    return ok;
}

function alignKeyword(v: char*, def: int) => int {
    let r: int = def;
    if (strEqI(v, "stretch") || strEqI(v, "normal")) r = AL_STRETCH;
    else if (strEqI(v, "flex-start") || strEqI(v, "start") || strEqI(v, "baseline") || strEqI(v, "self-start")) r = AL_START;
    else if (strEqI(v, "flex-end") || strEqI(v, "end") || strEqI(v, "self-end")) r = AL_END;
    else if (strEqI(v, "center")) r = AL_CENTER;
    else if (strEqI(v, "auto")) r = AL_AUTO;
    return r;
}

function overflowKeyword(v: char*, def: int) => int {
    let r: int = def;
    if (strEqI(v, "visible")) r = OV_VISIBLE;
    else if (strEqI(v, "hidden") || strEqI(v, "clip")) r = OV_HIDDEN;
    else if (strEqI(v, "scroll")) r = OV_SCROLL;
    else if (strEqI(v, "auto")) r = OV_AUTO;
    return r;
}
