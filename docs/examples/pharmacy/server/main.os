// pharmacy/server/main.os -- start the pharmacy site. Run from the project root:
//   .\bin\Release-windows-x86_64\Osengine.exe docs\examples\pharmacy\server\main.os  (from the project root)
// then open http://localhost:8080 in a browser (OmniBrowser: set START or type the address).
// First run creates pharmacy/data/pharmacy.db with sample drugs and three accounts:
//   admin / admin123    pharmacist / pharm123    cashier / cashier123   (change them from Users > Add user)
module Console = import "standard/1/Console.os";
module HS = import "standard/1/HttpServer.os";
module CFG = import "pharmacy/server/config.os";
module D = import "pharmacy/server/db.os";
module A = import "pharmacy/server/auth.os";
module PA = import "pharmacy/server/pages_auth.os";
module PD = import "pharmacy/server/pages_drugs.os";
module PS = import "pharmacy/server/pages_stock.os";
module PL = import "pharmacy/server/pages_sales.os";
module PR = import "pharmacy/server/pages_reports.os";
module PU = import "pharmacy/server/pages_users.os";

// Route handlers must be passed as local function names, so each page function gets a one-line wrapper here.
function h_loginPage(req: HS.Request*, res: HS.Response*) => void { PA.loginPage(req, res); }
function h_loginPost(req: HS.Request*, res: HS.Response*) => void { PA.loginPost(req, res); }
function h_logoutPost(req: HS.Request*, res: HS.Response*) => void { PA.logoutPost(req, res); }
function h_dashboard(req: HS.Request*, res: HS.Response*) => void { PA.dashboard(req, res); }
function h_drugsList(req: HS.Request*, res: HS.Response*) => void { PD.drugsList(req, res); }
function h_drugNewPage(req: HS.Request*, res: HS.Response*) => void { PD.drugNewPage(req, res); }
function h_drugNewPost(req: HS.Request*, res: HS.Response*) => void { PD.drugNewPost(req, res); }
function h_drugDetail(req: HS.Request*, res: HS.Response*) => void { PD.drugDetail(req, res); }
function h_drugEditPage(req: HS.Request*, res: HS.Response*) => void { PD.drugEditPage(req, res); }
function h_drugEditPost(req: HS.Request*, res: HS.Response*) => void { PD.drugEditPost(req, res); }
function h_drugDeletePost(req: HS.Request*, res: HS.Response*) => void { PD.drugDeletePost(req, res); }
function h_receivePage(req: HS.Request*, res: HS.Response*) => void { PS.receivePage(req, res); }
function h_receivePost(req: HS.Request*, res: HS.Response*) => void { PS.receivePost(req, res); }
function h_adjustPage(req: HS.Request*, res: HS.Response*) => void { PS.adjustPage(req, res); }
function h_adjustPost(req: HS.Request*, res: HS.Response*) => void { PS.adjustPost(req, res); }
function h_movementsPage(req: HS.Request*, res: HS.Response*) => void { PS.movementsPage(req, res); }
function h_salesHistory(req: HS.Request*, res: HS.Response*) => void { PL.salesHistory(req, res); }
function h_salePage(req: HS.Request*, res: HS.Response*) => void { PL.salePage(req, res); }
function h_cartAddPost(req: HS.Request*, res: HS.Response*) => void { PL.cartAddPost(req, res); }
function h_cartRemovePost(req: HS.Request*, res: HS.Response*) => void { PL.cartRemovePost(req, res); }
function h_checkoutPost(req: HS.Request*, res: HS.Response*) => void { PL.checkoutPost(req, res); }
function h_receiptPage(req: HS.Request*, res: HS.Response*) => void { PL.receiptPage(req, res); }
function h_reportsIndex(req: HS.Request*, res: HS.Response*) => void { PR.reportsIndex(req, res); }
function h_lowStockPage(req: HS.Request*, res: HS.Response*) => void { PR.lowStockPage(req, res); }
function h_lowStockCsv(req: HS.Request*, res: HS.Response*) => void { PR.lowStockCsv(req, res); }
function h_expiringPage(req: HS.Request*, res: HS.Response*) => void { PR.expiringPage(req, res); }
function h_salesReport(req: HS.Request*, res: HS.Response*) => void { PR.salesReport(req, res); }
function h_usersPage(req: HS.Request*, res: HS.Response*) => void { PU.usersPage(req, res); }
function h_userNewPost(req: HS.Request*, res: HS.Response*) => void { PU.userNewPost(req, res); }
function h_userTogglePost(req: HS.Request*, res: HS.Response*) => void { PU.userTogglePost(req, res); }

function main() => int {
    if (!D.init()) {
        Console.error("Cannot open the database. Check DB_PATH in pharmacy/server/config.os and that its folder exists.");
        return 1;
    }
    A.init();
    A.seedUsers();

    let srv: HS.HttpServer;
    srv.get("/login", h_loginPage);
    srv.post("/login", h_loginPost);
    srv.post("/logout", h_logoutPost);
    srv.get("/", h_dashboard);
    srv.get("/drugs", h_drugsList);
    srv.get("/drugs/new", h_drugNewPage);
    srv.post("/drugs/new", h_drugNewPost);
    srv.get("/drugs/:id", h_drugDetail);
    srv.get("/drugs/:id/edit", h_drugEditPage);
    srv.post("/drugs/:id/edit", h_drugEditPost);
    srv.post("/drugs/:id/delete", h_drugDeletePost);
    srv.get("/stock/receive", h_receivePage);
    srv.post("/stock/receive", h_receivePost);
    srv.get("/stock/adjust/:id", h_adjustPage);
    srv.post("/stock/adjust/:id", h_adjustPost);
    srv.get("/stock/movements", h_movementsPage);
    srv.get("/sales", h_salesHistory);
    srv.get("/sales/new", h_salePage);
    srv.post("/sales/cart/add", h_cartAddPost);
    srv.post("/sales/cart/remove", h_cartRemovePost);
    srv.post("/sales/checkout", h_checkoutPost);
    srv.get("/sales/:id", h_receiptPage);
    srv.get("/reports", h_reportsIndex);
    srv.get("/reports/low-stock", h_lowStockPage);
    srv.get("/reports/low-stock.csv", h_lowStockCsv);
    srv.get("/reports/expiring", h_expiringPage);
    srv.get("/reports/sales", h_salesReport);
    srv.get("/users", h_usersPage);
    srv.post("/users/new", h_userNewPost);
    srv.post("/users/:id/toggle", h_userTogglePost);

    srv.serveStatic("/static", CFG.STATIC_DIR);

    Console.println("Pharmacy site running at http://localhost:8090  (Ctrl+C to stop)");
    if (!srv.listen(CFG.PORT)) {
        Console.error("Could not listen on the port (already in use?).");
        return 1;
    }
    let dbh = D.handle();
    dbh.close();
    return 0;
}
