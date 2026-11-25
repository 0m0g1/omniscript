// pharmacy/server/html.os -- page skeleton and small HTML helpers. Everything printed goes through esc().
module Str = import "standard/1/String.os";
module HS = import "standard/1/HttpServer.os";
module Text = import "standard/1/Text.os";
module F = import "pharmacy/server/fmt.os";

function esc(s: char*, out: Str.String*) => void {
    let i: u64 = 0;
    while ((s[i] as int) != 0) {
        let c: int = s[i] as int;
        if (c == 38) out.append("&amp;");
        else if (c == 60) out.append("&lt;");
        else if (c == 62) out.append("&gt;");
        else if (c == 34) out.append("&quot;");
        else if (c == 39) out.append("&#39;");
        else out.appendChar(s[i]);
        i = i + 1;
    }
}

function has(s: char*) => bool { return (s[0] as int) != 0; }

function csrf(out: Str.String*, token: char*) => void {
    out.append("<input type=\"hidden\" name=\"csrf\" value=\"");
    esc(token, out);
    out.append("\">");
}

function link(out: Str.String*, href: char*, label: char*) => void {
    out.append("<a href=\"");
    esc(href, out);
    out.append("\">");
    esc(label, out);
    out.append("</a>");
}

function td(out: Str.String*, s: char*) => void { out.append("<td>"); esc(s, out); out.append("</td>"); }
function tdI(out: Str.String*, v: i64) => void { out.append("<td>"); out.appendInt(v); out.append("</td>"); }
function tdM(out: Str.String*, cents: i64) => void { out.append("<td class=\"num\">"); F.money(cents, out); out.append("</td>"); }

function flashOf(out: Str.String*, req: HS.Request*) => void {
    let v: Str.String;
    if (req.queryParam("ok", &v)) { out.append("<div class=\"flash ok\">"); esc(v.cstr(), out); out.append("</div>"); }
    v.clear();
    if (req.queryParam("err", &v)) { out.append("<div class=\"flash err\">"); esc(v.cstr(), out); out.append("</div>"); }
    v.free();
}

// rank: 3 admin, 2 pharmacist, 1 cashier
function start(out: Str.String*, req: HS.Request*, title: char*, rank: int, username: char*, token: char*) => void {
    out.append("<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>");
    esc(title, out);
    out.append(" - Pharmacy</title><link rel=\"stylesheet\" href=\"/static/app.css\"></head><body>");
    out.append("<div class=\"top\"><span class=\"brand\">Pharmacy Manager</span><span class=\"nav\">");
    link(out, "/", "Dashboard");
    if (rank >= 2) { link(out, "/drugs", "Drugs"); link(out, "/stock/receive", "Receive stock"); link(out, "/stock/movements", "Movements"); }
    link(out, "/sales/new", "New sale");
    link(out, "/sales", "Sales");
    if (rank >= 2) link(out, "/reports", "Reports");
    if (rank >= 3) link(out, "/users", "Users");
    out.append("</span><form class=\"who\" method=\"post\" action=\"/logout\">");
    esc(username, out);
    out.append(" ");
    csrf(out, token);
    out.append("<button type=\"submit\">Log out</button></form></div><div class=\"page\"><h1>");
    esc(title, out);
    out.append("</h1>");
    flashOf(out, req);
}

function finish(out: Str.String*) => void { out.append("</div></body></html>"); }

// 303 redirect to path?key=message
function redirectMsg(res: HS.Response*, path: char*, key: char*, msg: char*) => void {
    let u: Str.String;
    u.append(path);
    u.append("?");
    u.append(key);
    u.append("=");
    Text.urlEncode(msg, &u);
    res.redirect(u.cstr(), 303);
    u.free();
}
