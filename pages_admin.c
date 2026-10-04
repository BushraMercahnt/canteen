/* pages_admin.c - the canteen staff panel. Owner: Partner B. */
#include "pages_admin.h"
#include "page.h"
#include "server.h"
#include "auth.h"
#include "queue.h"
#include "menu.h"
#include "report.h"

/* Style for the text boxes on the login form */
#define INPUT_STYLE "style=\"height:2.6em;font-size:1em;border:3px solid #10203A;" \
                    "border-radius:.5em;padding:0 .6em\""

/* If the visitor is not logged in, send them to the login page.
 * Returns 1 when they were sent away (so the caller should return at once). */
static int need_login(sock_t client, const Request *req)
{
    if (auth_is_staff(req)) return 0;
    send_redirect(client, "/admin/login", NULL);
    return 1;
}

/* Prints "2 x Samosa, 1 x Tea" for an order */
static void print_items(Page *p, const Order *o)
{
    int first = 1;
    for (int i = 0; i < menu_count(); i++) {
        if (o->qty[i] > 0) {
            pg_add(p, "%s%d &times; %s", first ? "" : ", ", o->qty[i], menu_get(i)->name);
            first = 0;
        }
    }
}

/* What is the next step for an order? Returns the next status, or -1 if it is finished. */
static int next_step(Status s, const char **label)
{
    switch (s) {
        case ST_WAITING:   *label = "Start Preparing"; return ST_PREPARING;
        case ST_PREPARING: *label = "Mark Ready";      return ST_READY;
        case ST_READY:     *label = "Mark Collected";  return ST_COLLECTED;
        default:           return -1;
    }
}

/* ---------------- Login ---------------- */
void adm_login_page(sock_t client, const Request *req)
{
    char bad[8] = "";
    get_param(req->query, "bad", bad, sizeof bad);

    Page p;
    page_begin(&p, req, "Staff Login", 0, 1);
    pg_puts(&p, "<h1>Canteen staff only</h1>");
    if (bad[0] == '1')
        pg_puts(&p, "<div class=\"err\">Wrong username or password. Please try again.</div>");
    pg_puts(&p, "<form method=\"post\" action=\"/admin/login\" "
                "style=\"display:flex;flex-direction:column;gap:.7em\">"
                "<label for=\"user\"><b>Username</b></label>"
                "<input id=\"user\" name=\"user\" type=\"text\" required " INPUT_STYLE ">"
                "<label for=\"pass\"><b>Password</b></label>"
                "<input id=\"pass\" name=\"pass\" type=\"password\" required " INPUT_STYLE ">"
                "<button class=\"btn pri\" type=\"submit\">Login</button></form>");
    page_end(&p);
    page_send(client, &p);
}

void adm_login_submit(sock_t client, const Request *req)
{
    char user[40] = "", pass[40] = "";
    get_param(req->body, "user", user, sizeof user);
    get_param(req->body, "pass", pass, sizeof pass);

    if (auth_check_login(user, pass))
        send_redirect(client, "/admin", AUTH_COOKIE);          /* good: log in */
    else
        send_redirect(client, "/admin/login?bad=1", NULL);     /* bad: try again */
}

void adm_logout(sock_t client, const Request *req)
{
    (void)req;
    send_redirect(client, "/admin/login", "staff=; Path=/; Max-Age=0");   /* Max-Age=0 deletes the cookie */
}

/* ---------------- Dashboard ---------------- */
void adm_dashboard_page(sock_t client, const Request *req)
{
    if (need_login(client, req)) return;

    Page p;
    page_begin(&p, req, "Staff Panel", 10, 0);      /* reloads itself every 10 seconds */

    pg_puts(&p, "<div class=\"row\">"
                "<a class=\"btn sec\" style=\"flex:1\" href=\"/scan\">Scan QR</a>"
                "<a class=\"btn sec\" style=\"flex:1\" href=\"/admin/report\">Report</a>"
                "<a class=\"btn sec\" style=\"flex:1\" href=\"/admin/stock\">Stock</a>"
                "<a class=\"btn sec\" style=\"flex:1\" href=\"/admin/logout\">Logout</a></div>");

    int now = queue_now_serving();
    if (now > 0) pg_add(&p, "<div class=\"card row serving\"><span>Now serving</span><b>%d</b></div>", now);
    else         pg_puts(&p, "<div class=\"card row serving\"><span>Now serving</span><b>-</b></div>");

    pg_puts(&p, "<form method=\"post\" action=\"/admin/next\">"
                "<button class=\"btn pri\" type=\"submit\" style=\"width:100%\">Call Next</button></form>");

    int shown = 0;
    for (int i = 0; i < queue_count(); i++) {
        const Order *o = queue_at(i);
        const char *label = "";
        int next = next_step(o->status, &label);
        if (next < 0) continue;                      /* skip Collected and Cancelled */
        shown++;

        pg_add(&p, "<div class=\"card\"><div class=\"row\"><b style=\"font-size:1.8em\">%d</b>", o->token);
        pg_status_chip(&p, o->status);
        pg_puts(&p, "</div><div>");
        print_items(&p, o);
        pg_add(&p, " &middot; &#8377;%d</div>", o->total);
        pg_add(&p, "<form method=\"post\" action=\"/admin/status\" style=\"margin-top:.5em\">"
                   "<input type=\"hidden\" name=\"token\" value=\"%d\">"
                   "<input type=\"hidden\" name=\"to\" value=\"%d\">"
                   "<button class=\"btn pri\" type=\"submit\" style=\"width:100%%\">%s</button>"
                   "</form></div>", o->token, next, label);
    }
    if (shown == 0) pg_puts(&p, "<div class=\"card\">No active orders right now.</div>");

    page_end(&p);
    page_send(client, &p);
}

/* POST /admin/status   body: token=5&to=1   (to = the new status number) */
void adm_status_change(sock_t client, const Request *req)
{
    if (need_login(client, req)) return;

    char token[16] = "", to[8] = "";
    get_param(req->body, "token", token, sizeof token);
    get_param(req->body, "to", to, sizeof to);
    int t = atoi(token), s = atoi(to);
    if (t > 0 && s >= ST_WAITING && s <= ST_CANCELLED)
        queue_set_status(t, (Status)s);
    send_redirect(client, "/admin", NULL);           /* back to the dashboard */
}

/* POST /admin/next */
void adm_call_next(sock_t client, const Request *req)
{
    if (need_login(client, req)) return;
    queue_call_next();
    send_redirect(client, "/admin", NULL);
}

/* ---------------- Menu stock (Finished / Available) ---------------- */
void adm_stock_page(sock_t client, const Request *req)
{
    if (need_login(client, req)) return;

    Page p;
    page_begin(&p, req, "Menu Stock", 0, 0);
    pg_puts(&p, "<a class=\"btn sec\" href=\"/admin\">Back to Staff Panel</a>");
    for (int i = 0; i < menu_count(); i++) {
        const MenuItem *m = menu_get(i);
        pg_add(&p, "<div class=\"card\"><div class=\"row\"><b><span class=\"vd%s\" role=\"img\" aria-label=\"%s\"></span>%s</b>"
                   "<span class=\"chip %s\">%s</span></div>",
               m->veg ? "" : " nv", m->veg ? "Veg" : "Non-veg", m->name,
               m->available ? "c2" : "c1", m->available ? "Available" : "Finished");
        pg_add(&p, "<form method=\"post\" action=\"/admin/stock\" style=\"margin-top:.5em\">"
                   "<input type=\"hidden\" name=\"item\" value=\"%d\">"
                   "<input type=\"hidden\" name=\"to\" value=\"%d\">"
                   "<button class=\"btn %s\" type=\"submit\" style=\"width:100%%\">%s</button></form></div>",
               i, m->available ? 0 : 1, m->available ? "sec" : "pri",
               m->available ? "Mark Finished" : "Mark Available");
    }
    page_end(&p);
    page_send(client, &p);
}

/* POST /admin/stock   body: item=2&to=0   (to = 1 available, 0 finished) */
void adm_stock_toggle(sock_t client, const Request *req)
{
    if (need_login(client, req)) return;

    char item[8] = "", to[8] = "";
    get_param(req->body, "item", item, sizeof item);
    get_param(req->body, "to", to, sizeof to);
    if (item[0] != '\0') menu_set_available(atoi(item), atoi(to));
    send_redirect(client, "/admin/stock", NULL);
}

/* ---------------- Daily report ---------------- */
void adm_report_page(sock_t client, const Request *req)
{
    if (need_login(client, req)) return;

    int orders = 0, sales = 0, rated = 0;
    double average = 0.0;
    report_totals(&orders, &sales);
    report_ratings(&rated, &average);

    Page p;
    page_begin(&p, req, "Daily Report", 0, 0);
    pg_puts(&p, "<a class=\"btn sec\" href=\"/admin\">Back to Staff Panel</a>");
    pg_add(&p, "<div class=\"card row serving\"><span>Total orders</span><b>%d</b></div>", orders);
    pg_add(&p, "<div class=\"card row serving\"><span>Total sales</span><b>&#8377;%d</b></div>", sales);
    if (rated > 0)
        pg_add(&p, "<div class=\"card row serving\"><span>Average rating (%d orders rated)</span><b>%.1f &#9733;</b></div>", rated, average);
    else
        pg_puts(&p, "<div class=\"card\">No ratings yet.</div>");
    page_end(&p);
    page_send(client, &p);
}
