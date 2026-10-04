#ifndef PAGES_ADMIN_H
#define PAGES_ADMIN_H
#include "common.h"
/* Staff pages. Owner: Partner B. All of these are TODO. */
void adm_login_page(sock_t client, const Request *req);      /* GET  /admin/login  */
void adm_login_submit(sock_t client, const Request *req);    /* POST /admin/login  */
void adm_logout(sock_t client, const Request *req);          /* GET  /admin/logout */
void adm_dashboard_page(sock_t client, const Request *req);  /* GET  /admin        */
void adm_status_change(sock_t client, const Request *req);   /* POST /admin/status */
void adm_call_next(sock_t client, const Request *req);       /* POST /admin/next   */
void adm_stock_page(sock_t client, const Request *req);      /* GET  /admin/stock  */
void adm_stock_toggle(sock_t client, const Request *req);    /* POST /admin/stock  */
void adm_report_page(sock_t client, const Request *req);     /* GET  /admin/report */
#endif
