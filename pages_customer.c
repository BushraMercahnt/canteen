/* pages_customer.c - what the students see. Owner: Partner A
 * Look at cust_token_page() to see how a page is built:
 *   page_begin -> pg_add / pg_puts ... -> page_end -> page_send            */
#include <stdio.h>
#include <stdlib.h>
#include "pages_customer.h"
#include "page.h"
#include "server.h"
#include "menu.h"
#include "queue.h"

/* ---------------- Cover page (the first page of the website) ---------------- */
/* The poster picture is the file cover.jpg (kept next to the .c files). cust_cover_image() sends it. */
void cust_cover_image(sock_t client, const Request *req)
{
    (void)req;
    FILE *f = fopen("cover.jpg", "rb");
    if (!f) { send_response(client, 404, "text/plain", NULL, "cover.jpg not found", 19); return; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *data = malloc((size_t)size);
    if (data && fread(data, 1, (size_t)size, f) == (size_t)size)
        send_response(client, 200, "image/jpeg", NULL, data, (size_t)size);
    else
        send_response(client, 404, "text/plain", NULL, "cover.jpg not readable", 22);
    free(data);
    fclose(f);
}

void cust_cover_page(sock_t client, const Request *req)
{
    Page p;
    page_begin(&p, req, "College Canteen", 0, 0);
    pg_puts(&p,
        "<style>.bar{display:none}body{background:#1FA89B}.body{max-width:560px;padding:0;gap:0}.cv{text-align:center;padding:.6em .6em 1.2em;background:linear-gradient(170deg,#F0703C 0%,#FF9A4A 35%,#26B7A8 100%)}.cv img{display:block;width:100%;height:auto;border-radius:1em;box-shadow:0 .5em 1.2em rgba(16,32,58,.35)}.cv .live{display:inline-flex;align-items:center;gap:.5em;margin-top:.9em;background:rgba(255,255,255,.85);color:#10203A;border-radius:999px;padding:.35em 1em;font-size:.85em;font-weight:bold}.cv .dot{width:.7em;height:.7em;border-radius:50%;background:#16B364;animation:pulse 1.6s infinite}@keyframes pulse{0%{box-shadow:0 0 0 0 rgba(22,179,100,.8)}70%,100%{box-shadow:0 0 0 .7em rgba(22,179,100,0)}}.cv .go{display:flex;flex-direction:column;align-items:center;margin-top:.9em;background:#10203A;color:#fff;border-radius:999px;padding:.55em 1em;text-decoration:none;font-weight:900;font-size:1.5em;border:3px solid #fff;animation:glow 2s infinite}.cv .go small{font-weight:normal;font-size:.5em;opacity:.95}@keyframes glow{0%,100%{box-shadow:0 0 0 0 rgba(255,255,255,.85)}50%{box-shadow:0 0 0 .5em rgba(255,255,255,0)}}.cv .pills{display:flex;gap:.4em;justify-content:center;flex-wrap:wrap;margin-top:1em}.cv .pills span{background:rgba(255,255,255,.85);border-radius:999px;padding:.25em .8em;font-size:.72em;font-weight:900;color:#0B7A70}.cv .staff{display:block;margin-top:.9em;color:#10203A;opacity:.8;font-size:.75em;text-decoration:none}@media (prefers-reduced-motion:reduce){.cv *{animation:none!important}}</style>");
    int now = queue_now_serving();
    pg_puts(&p,
        "<div class=\"cv\">"
        "<img src=\"/cover.jpg\" alt=\"Smart Canteen - No More Queues!\">");
    if (now > 0) pg_add(&p, "<div class=\"live\"><span class=\"dot\"></span>LIVE &middot; Now serving <b>#%d</b> &middot; %d waiting</div>", now, queue_waiting_count());
    else         pg_add(&p, "<div class=\"live\"><span class=\"dot\"></span>LIVE &middot; %d orders waiting</div>", queue_waiting_count());
    pg_puts(&p, "<a class=\"go\" href=\"/home\">Start &rarr;<small>Tap here to begin</small></a>"
                "<div class=\"pills\"><span>&#10003; Digital Queue</span><span>&#10003; Instant Token</span>"
                "<span>&#10003; Star Rating</span></div>"
                "<a class=\"staff\" href=\"/admin\">Canteen staff login</a></div>");
    page_end(&p);
    page_send(client, &p);
}

/* ---------------- Home ---------------- */
void cust_home_page(sock_t client, const Request *req)
{
    Page p;
    page_begin(&p, req, "College Canteen", 0, 0);
    int now = queue_now_serving();
    if (now > 0) pg_add(&p, "<div class=\"card row serving\"><span>Now serving</span><b>%d</b></div>", now);
    else         pg_puts(&p, "<div class=\"card row serving\"><span>Now serving</span><b>-</b></div>");

    /* "Quiet now / Busy now" badge, based on how many orders are waiting */
    int waiting = queue_waiting_count();
    const char *cls = waiting <= 3 ? "b-quiet" : (waiting <= 8 ? "b-busy" : "b-vbusy");
    const char *msg = waiting <= 3 ? "Quiet now" : (waiting <= 8 ? "Busy now" : "Very busy now");
    pg_add(&p, "<div class=\"busy %s\">%s &middot; %d orders waiting</div>", cls, msg, waiting);
    pg_puts(&p,
        "<h1>What would you like to do?</h1>"
        "<a class=\"btn pri\" href=\"/menu\">Order Food<small>See the menu and get a token</small></a>"
        "<a class=\"btn sec\" href=\"/check\">Check My Token</a>"
        "<a class=\"btn sec\" href=\"/queue\">Live Queue</a>"
        "<div class=\"sizes\"><span>Text size</span>"
        "<a href=\"/size?set=normal&amp;back=/home\">A</a>"
        "<a href=\"/size?set=big&amp;back=/home\" style=\"font-size:1.2em\">A</a>"
        "<a href=\"/size?set=huge&amp;back=/home\" style=\"font-size:1.5em\">A</a></div>");
    page_end(&p);
    page_send(client, &p);
}

/* ---------------- Text size (sets a cookie, then goes back) ---------------- */
void cust_size_set(sock_t client, const Request *req)
{
    char set[16] = "normal", back[64] = "/home", cookie[64];
    get_param(req->query, "set", set, sizeof set);
    get_param(req->query, "back", back, sizeof back);
    if (strcmp(set, "big") != 0 && strcmp(set, "huge") != 0) strcpy(set, "normal");
    if (back[0] != '/') strcpy(back, "/home");          /* only allow going to our own pages */
    snprintf(cookie, sizeof cookie, "size=%s; Path=/; Max-Age=31536000", set);
    send_redirect(client, back, cookie);
}

/* ---------------- Menu ---------------- */
static void render_menu(sock_t client, const Request *req, const char *error)
{
    /* find the food that sold the most today (-1 if nothing has been sold yet) */
    int popular = -1, best = 0;
    for (int i = 0; i < menu_count(); i++) {
        int sold = queue_item_sold(i);
        if (sold > best) { best = sold; popular = i; }
    }

    Page p;
    page_begin(&p, req, "Choose your food", 0, 1);
    if (error) pg_add(&p, "<div class=\"err\">%s</div>", error);
    pg_puts(&p, "<form method=\"post\" action=\"/order\" style=\"display:flex;flex-direction:column;gap:.7em\">");
    for (int i = 0; i < menu_count(); i++) {
        const MenuItem *m = menu_get(i);
        pg_add(&p, "<div class=\"card item%s\"><div class=\"t\"><span><span class=\"vd%s\" role=\"img\" aria-label=\"%s\"></span>%s",
               m->available ? "" : " off", m->veg ? "" : " nv", m->veg ? "Veg" : "Non-veg", m->name);
        if (i == popular) pg_puts(&p, "<span class=\"pop\">Popular</span>");
        pg_add(&p, "</span><span class=\"pr\">&#8377;%d</span></div>", m->price);
        if (!m->available) {
            pg_puts(&p, "<div class=\"finished\">Finished for today</div>");
        } else {
            pg_puts(&p, "<div class=\"qty\">");
            for (int q = 0; q <= 3; q++)
                pg_add(&p, "<label><input type=\"radio\" name=\"q%d\" value=\"%d\"%s><span>%d</span></label>",
                       i, q, q == 0 ? " checked" : "", q);
            pg_puts(&p, "</div>");
        }
        pg_puts(&p, "</div>");
    }
    pg_puts(&p, "<button class=\"btn pri\" type=\"submit\">Place Order<small>Pay at the counter</small></button></form>");
    page_end(&p);
    page_send(client, &p);
}

void cust_menu_page(sock_t client, const Request *req)
{
    render_menu(client, req, NULL);
}

/* ---------------- Order (POST from the menu form) ---------------- */
void cust_order_submit(sock_t client, const Request *req)
{
    int qty[MAX_MENU_ITEMS] = {0};
    for (int i = 0; i < menu_count(); i++) {
        char key[16], val[8];
        snprintf(key, sizeof key, "q%d", i);
        if (get_param(req->body, key, val, sizeof val)) qty[i] = atoi(val);
    }
    int token = queue_place_order(qty, menu_count());
    if (token == 0) {
        render_menu(client, req, "Please choose at least one item.");
        return;
    }
    char where[40];
    snprintf(where, sizeof where, "/token?id=%d", token);
    send_redirect(client, where, NULL);        /* redirect so a refresh does not order twice */
}

/* ---------------- Token ---------------- */
void cust_token_page(sock_t client, const Request *req)
{
    char idtext[16] = "";
    get_param(req->query, "id", idtext, sizeof idtext);
    const Order *o = queue_find(atoi(idtext));

    /* reload every 5 seconds while waiting; stop when the order is Ready so the rating form keeps its choice */
    int refresh = (o && (o->status == ST_READY || o->status == ST_COLLECTED)) ? 0 : 5;

    Page p;
    page_begin(&p, req, "Your Token", refresh, 1);
    if (!o) {
        pg_puts(&p, "<div class=\"err\">We could not find that token. Please check the number.</div>"
                    "<a class=\"btn pri\" href=\"/check\">Try again</a>");
    } else {
        int ahead = queue_orders_ahead(o->token);
        pg_add(&p, "<div class=\"ticket\"><div class=\"lab\">Your token</div><div class=\"no\">%d</div>"
                   "<div class=\"two\"><div><b>%d</b><span>orders before you</span></div>"
                   "<div><b>~%d min</b><span>estimated wait</span></div></div></div>",
               o->token, ahead, ahead * MINUTES_PER_ORDER);

        pg_puts(&p, "<div class=\"card row\"><span>");
        int first = 1;
        for (int i = 0; i < menu_count(); i++) {
            if (o->qty[i] > 0) {
                pg_add(&p, "%s%d &times; %s", first ? "" : ", ", o->qty[i], menu_get(i)->name);
                first = 0;
            }
        }
        pg_add(&p, "<br><b>&#8377;%d</b></span>", o->total);
        pg_status_chip(&p, o->status);
        pg_puts(&p, "</div>");
        if (o->status == ST_READY)
            pg_puts(&p, "<div class=\"card\"><b>Your order is ready. Please collect it at the counter.</b></div>");
        else
            pg_puts(&p, "<p style=\"text-align:center\">Remember this number. Pay at the counter when it is called.</p>");
        /* Star rating: only after the order is Ready or Collected, and only once */
        if (o->status == ST_READY || o->status == ST_COLLECTED) {
            if (o->rating > 0) {
                pg_puts(&p, "<div class=\"card\"><b>Thank you! Your rating:</b> <span class=\"gold\">");
                for (int k = 1; k <= 5; k++) pg_puts(&p, k <= o->rating ? "&#9733;" : "&#9734;");
                pg_puts(&p, "</span></div>");
            } else {
                pg_puts(&p, "<form class=\"card\" method=\"post\" action=\"/rate\" "
                            "style=\"display:flex;flex-direction:column;gap:.6em\">"
                            "<b>How was your order?</b><div class=\"stars\">");
                for (int k = 1; k <= 5; k++)
                    pg_add(&p, "<label><input type=\"radio\" name=\"stars\" value=\"%d\" required><span>%d &#9733;</span></label>", k, k);
                pg_add(&p, "</div><input type=\"hidden\" name=\"token\" value=\"%d\">"
                           "<button class=\"btn pri\" type=\"submit\">Send Rating</button></form>", o->token);
            }
        }
        pg_add(&p, "<a class=\"btn pri\" href=\"/queue?id=%d\">See Live Queue</a>", o->token);
    }
    page_end(&p);
    page_send(client, &p);
}

/* ---------------- Check my token (type a number) ---------------- */
void cust_check_page(sock_t client, const Request *req)
{
    Page p;
    page_begin(&p, req, "Check My Token", 0, 1);
    pg_puts(&p,
        "<form method=\"get\" action=\"/token\" style=\"display:flex;flex-direction:column;gap:.7em\">"
        "<label for=\"id\"><b>Type your token number</b></label>"
        "<input id=\"id\" name=\"id\" type=\"number\" min=\"1\" required>"
        "<button class=\"btn pri\" type=\"submit\">Show My Order</button></form>");
    page_end(&p);
    page_send(client, &p);
}

/* ---------------- Live queue ---------------- */
void cust_queue_page(sock_t client, const Request *req)
{
    char idtext[16] = "";
    get_param(req->query, "id", idtext, sizeof idtext);
    int myid = atoi(idtext);   /* 0 if no token was given */

    Page p;
    /* 5 = the page reloads every 5 seconds */
    page_begin(&p, req, "Live Queue", 5, 1);
    pg_add(&p, "<div class=\"card row serving\">"
               "<span>Now serving</span><b>%d</b></div>",
           queue_now_serving());

    /* Show at most 6 orders: Ready first, then Preparing, then Waiting */
    static const Status show_order[3] = { ST_READY, ST_PREPARING, ST_WAITING };
    int shown = 0;
    for (int s = 0; s < 3 && shown < 6; s++) {
        for (int i = 0; i < queue_count(); i++) {
            const Order *o = queue_at(i);
            if (o->status != show_order[s]) continue;
            pg_add(&p, "<div class=\"card row\"><b>%d", o->token);
            if (o->token == myid)
                pg_puts(&p, " <span style=\"background:var(--blue);color:#fff;border-radius:.4em;"
                            "padding:.1em .5em;font-size:.8em;margin-left:.4em\">You</span>");
            pg_puts(&p, "</b>");
            pg_status_chip(&p, o->status);
            pg_puts(&p, "</div>");
            if (++shown == 6) break;
        }
    }
    if (shown == 0)
        pg_puts(&p, "<div class=\"card\">No orders are waiting right now.</div>");
    pg_puts(&p, "<p style=\"text-align:center\">This page updates by itself every 5 seconds.</p>");
    page_end(&p);
    page_send(client, &p);
}

/* ---------------- Star rating (POST from the token page) ---------------- */
/* POST /rate   body: token=5&stars=4 */
void cust_rate_submit(sock_t client, const Request *req)
{
    char token[16] = "", stars[8] = "", where[40];
    get_param(req->body, "token", token, sizeof token);
    get_param(req->body, "stars", stars, sizeof stars);
    int t = atoi(token);
    queue_set_rating(t, atoi(stars));              /* ignores bad numbers and second ratings */
    snprintf(where, sizeof where, "/token?id=%d", t);
    send_redirect(client, where, NULL);
}
