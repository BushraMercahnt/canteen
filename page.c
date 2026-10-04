#include "page.h"
#include "server.h"
#include "queue.h"
#include <stdarg.h>

/* The look of the whole site lives in this one string. */
static const char PAGE_CSS[] =
":root{--ink:#10203A;--blue:#1F4FD8;--bg:#F5F7FB;--line:#C6CFE0}"
"*{box-sizing:border-box}"
"body{margin:0;background:var(--bg);color:var(--ink);font-family:Verdana,Arial,sans-serif;font-size:20px;line-height:1.35}"
"body.big{font-size:25px}body.huge{font-size:30px}"
".bar{background:var(--ink);color:#fff;padding:.7em .9em;display:flex;justify-content:space-between;align-items:center;gap:.5em;font-weight:bold}"
".bar a{background:#fff;color:var(--ink);border-radius:999px;padding:.25em .8em;text-decoration:none;font-size:.85em}"
".body{max-width:520px;margin:0 auto;padding:.9em;display:flex;flex-direction:column;gap:.7em}"
"h1{font-size:1.6em;line-height:1.15;margin:.2em 0}"
".btn{display:flex;flex-direction:column;align-items:center;justify-content:center;min-height:3.2em;border-radius:.8em;font-weight:bold;font-size:1.1em;text-decoration:none;text-align:center;padding:.4em .8em;border:3px solid var(--blue);cursor:pointer;font-family:inherit}"
".btn small{font-weight:normal;font-size:.75em}"
".pri{background:var(--blue);color:#fff}.sec{background:#fff;color:var(--blue)}"
".card{background:#fff;border:2px solid var(--line);border-radius:.7em;padding:.6em .8em}"
".row{display:flex;justify-content:space-between;align-items:center;gap:.5em}"
".serving b{font-size:1.7em;color:var(--blue)}"
".item .t{display:flex;justify-content:space-between;font-weight:bold;font-size:1.1em}"
".item .t .pr{color:var(--blue)}"
".item .t>span:first-child{white-space:nowrap}"
".qty{display:grid;grid-template-columns:repeat(4,1fr);gap:.4em;margin-top:.4em}"
".qty label{position:relative;display:block}"
".qty input{position:absolute;opacity:0;top:0;left:0;width:100%;height:100%;margin:0;cursor:pointer}"
".qty span{display:flex;align-items:center;justify-content:center;height:2.4em;border:2px solid var(--line);border-radius:.5em;background:var(--bg);font-weight:bold}"
".qty input:checked+span{background:var(--blue);border-color:var(--blue);color:#fff}"
".qty input:focus-visible+span{outline:3px solid var(--ink);outline-offset:2px}"
".vd{display:inline-block;width:.8em;height:.8em;border:2px solid #0A7A3B;border-radius:3px;position:relative;margin-right:.4em;vertical-align:-.05em}"
".vd::after{content:'';position:absolute;top:.12em;left:.12em;right:.12em;bottom:.12em;border-radius:50%;background:#0A7A3B}"
".vd.nv{border-color:#B3261E}.vd.nv::after{background:#B3261E}"
".pop{background:#FFE3B8;color:#7A3B00;border-radius:999px;padding:.05em .6em;font-size:.7em;font-weight:bold;margin-left:.4em;white-space:nowrap}"
".busy{border-radius:.7em;padding:.5em .8em;font-weight:bold;text-align:center}"
".b-quiet{background:#D4F0DF;color:#0A5528}.b-busy{background:#FFE3B8;color:#7A3B00}.b-vbusy{background:#FFD6D2;color:#8A1C14}"
".stars{display:grid;grid-template-columns:repeat(5,1fr);gap:.4em}"
".stars label{position:relative;display:block}"
".stars input{position:absolute;opacity:0;top:0;left:0;width:100%;height:100%;margin:0;cursor:pointer}"
".stars span{display:flex;align-items:center;justify-content:center;height:2.4em;border:2px solid var(--line);border-radius:.5em;background:#fff;font-weight:bold}"
".stars input:checked+span{background:var(--blue);border-color:var(--blue);color:#fff}"
".stars input:focus-visible+span{outline:3px solid var(--ink);outline-offset:2px}"
".gold{color:#B45F00;letter-spacing:.1em}"
".off{opacity:.7}"
".finished{color:#7A3B00;font-weight:bold}"
".ticket{background:var(--ink);color:#fff;border-radius:1em;padding:1em;text-align:center}"
".ticket .lab{letter-spacing:.14em;text-transform:uppercase;font-size:.75em;font-weight:bold}"
".ticket .no{font-size:6em;font-weight:bold;line-height:1;margin:.05em 0 .15em}"
".ticket .two{display:flex;justify-content:space-around;border-top:3px dashed rgba(255,255,255,.4);padding-top:.6em}"
".ticket .two b{display:block;font-size:1.4em}.ticket .two span{font-size:.75em}"
".chip{display:inline-block;border-radius:999px;padding:.15em .7em;font-weight:bold;font-size:.9em;white-space:nowrap}"
".c0{background:#E3E7F0;color:#27324A}.c1{background:#FFE3B8;color:#7A3B00}"
".c2{background:#D4F0DF;color:#0A5528}.c3{background:#E3E7F0;color:#27324A}.c4{background:#E3E7F0;color:#27324A}"
".err{background:#FFE3B8;color:#7A3B00;border-radius:.6em;padding:.6em .8em;font-weight:bold}"
".sizes{display:flex;align-items:center;gap:.4em;font-weight:bold;margin-top:.5em}"
".sizes a{border:2px solid var(--line);background:#fff;color:var(--ink);border-radius:.5em;min-width:2.4em;height:2.4em;display:flex;align-items:center;justify-content:center;text-decoration:none}"
"input[type=number]{height:2.6em;font-size:1em;border:3px solid var(--ink);border-radius:.5em;padding:0 .6em;width:100%}"
".todo{border:3px dashed var(--line);border-radius:.8em;padding:1em;background:#fff}";

static void pg_grow(Page *p, size_t need)
{
    if (p->len + need + 1 <= p->cap) return;
    size_t cap = p->cap ? p->cap : 4096;
    while (cap < p->len + need + 1) cap *= 2;
    char *d = realloc(p->data, cap);
    if (!d) { fprintf(stderr, "out of memory\n"); exit(1); }
    p->data = d;
    p->cap = cap;
}

void pg_puts(Page *p, const char *text)
{
    size_t n = strlen(text);
    pg_grow(p, n);
    memcpy(p->data + p->len, text, n + 1);
    p->len += n;
}

void pg_add(Page *p, const char *fmt, ...)
{
    va_list ap, ap2;
    va_start(ap, fmt);
    va_copy(ap2, ap);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n > 0) {
        pg_grow(p, (size_t)n);
        vsnprintf(p->data + p->len, (size_t)n + 1, fmt, ap2);
        p->len += (size_t)n;
    }
    va_end(ap2);
}

void pg_free(Page *p)
{
    free(p->data);
    p->data = NULL;
    p->len = p->cap = 0;
}

void page_begin(Page *p, const Request *req, const char *title, int refresh_secs, int show_home)
{
    char size[16] = "";
    get_cookie(req, "size", size, sizeof size);
    if (strcmp(size, "big") != 0 && strcmp(size, "huge") != 0) size[0] = '\0';

    p->data = NULL; p->len = p->cap = 0;
    pg_puts(p, "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\">"
               "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">");
    if (refresh_secs > 0)
        pg_add(p, "<meta http-equiv=\"refresh\" content=\"%d\">", refresh_secs);
    pg_add(p, "<title>%s</title><style>", title);
    pg_puts(p, PAGE_CSS);
    pg_add(p, "</style></head><body class=\"%s\"><div class=\"bar\"><span>%s</span>", size, title);
    if (show_home) pg_puts(p, "<a href=\"/home\">Home</a>");
    pg_puts(p, "</div><div class=\"body\">");
}

void page_end(Page *p)
{
    pg_puts(p, "</div></body></html>");
}

void page_send_status(sock_t client, Page *p, int code)
{
    send_response(client, code, "text/html; charset=utf-8", NULL, p->data, p->len);
    pg_free(p);
}

void page_send(sock_t client, Page *p)
{
    page_send_status(client, p, 200);
}

void page_todo(sock_t client, const Request *req, const char *title, const char *who, const char *what)
{
    Page p;
    page_begin(&p, req, title, 0, 1);
    pg_add(&p, "<div class=\"todo\"><h1>Coming soon</h1><p><b>%s</b> will build this page.</p><p>%s</p></div>", who, what);
    page_end(&p);
    page_send(client, &p);
}

void pg_status_chip(Page *p, int status)
{
    pg_add(p, "<span class=\"chip c%d\">%s</span>", status, status_text((Status)status));
}
