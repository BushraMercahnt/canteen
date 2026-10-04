/* router.c - THE MEETING POINT of both partners' work. Owner: Partner B.
 * Every page already has its line in the table below, so you do not need to change this file. */
#include "router.h"
#include "server.h"
#include "page.h"
#include "pages_customer.h"
#include "pages_admin.h"
#include "scan.h"

typedef struct {
    const char *method;
    const char *path;
    Handler     handler;
} Route;

static const Route routes[] = {
    /* ---- customer side (Partner A) ---- */
    { "GET",  "/",             cust_cover_page   },
    { "GET",  "/cover.jpg",    cust_cover_image  },
    { "GET",  "/home",         cust_home_page    },
    { "GET",  "/menu",         cust_menu_page    },
    { "POST", "/order",        cust_order_submit },
    { "GET",  "/token",        cust_token_page   },
    { "GET",  "/check",        cust_check_page   },
    { "GET",  "/queue",        cust_queue_page   },
    { "GET",  "/size",         cust_size_set     },
    { "POST", "/rate",         cust_rate_submit  },
    { "GET",  "/scan",         scan_page         },   /* QR code to open the canteen (Partner B) */

    /* ---- staff side (Partner B) ---- */
    { "GET",  "/admin/login",  adm_login_page    },
    { "POST", "/admin/login",  adm_login_submit  },
    { "GET",  "/admin/logout", adm_logout        },
    { "GET",  "/admin",        adm_dashboard_page },
    { "POST", "/admin/status", adm_status_change },
    { "POST", "/admin/next",   adm_call_next     },
    { "GET",  "/admin/stock",  adm_stock_page    },
    { "POST", "/admin/stock",  adm_stock_toggle  },
    { "GET",  "/admin/report", adm_report_page   },
    /* All routes are already here, so nobody has to edit this file. */
};

void router_dispatch(sock_t client, const Request *req)
{
    size_t n = sizeof routes / sizeof routes[0];
    for (size_t i = 0; i < n; i++) {
        if (strcmp(routes[i].method, req->method) == 0 &&
            strcmp(routes[i].path, req->path) == 0) {
            routes[i].handler(client, req);
            return;
        }
    }
    Page p;
    page_begin(&p, req, "Page not found", 0, 1);
    pg_puts(&p, "<h1>Sorry, we could not find that page.</h1>"
                "<a class=\"btn pri\" href=\"/home\">Go to Home</a>");
    page_end(&p);
    page_send_status(client, &p, 404);
}
