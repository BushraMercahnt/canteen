/* page.h - shared page helpers (same look on every page). Owner: both, edit CSS together. */
#ifndef PAGE_H
#define PAGE_H
#include "common.h"

typedef struct { char *data; size_t len, cap; } Page;

void pg_add(Page *p, const char *fmt, ...);   /* like printf, appends to the page */
void pg_puts(Page *p, const char *text);      /* append raw text (use for text containing %) */
void pg_free(Page *p);

/* Starts a page: html head, css, dark top bar with the title, and the content box.
 * refresh_secs > 0 makes the browser reload the page by itself. */
void page_begin(Page *p, const Request *req, const char *title, int refresh_secs, int show_home);
void page_end(Page *p);                                   /* closes the page */
void page_send(sock_t client, Page *p);                   /* sends with 200 and frees */
void page_send_status(sock_t client, Page *p, int code);  /* same with another code */

/* Prints a "coming soon" page. Use it while your real page is not written yet. */
void page_todo(sock_t client, const Request *req, const char *title, const char *who, const char *what);

/* Prints a coloured status pill: Waiting / Preparing / Ready ... */
void pg_status_chip(Page *p, int status);
#endif
