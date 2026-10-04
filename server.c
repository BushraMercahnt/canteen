/* server.c - a tiny web server written only with C sockets. Owner: Partner B */
#include "server.h"
#include "router.h"

#ifdef _WIN32
  #define strncasecmp _strnicmp
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <signal.h>
  #include <strings.h>
  #define closesocket close
#endif

#define REQ_MAX 8192

/* ---------- small helpers ---------- */

static int hex_val(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static void url_decode(char *dst, const char *src, size_t n)
{
    size_t i = 0;
    while (*src && i + 1 < n) {
        if (*src == '+') { dst[i++] = ' '; src++; }
        else if (*src == '%' && hex_val(src[1]) >= 0 && hex_val(src[2]) >= 0) {
            dst[i++] = (char)(hex_val(src[1]) * 16 + hex_val(src[2]));
            src += 3;
        } else dst[i++] = *src++;
    }
    dst[i] = '\0';
}

int get_param(const char *data, const char *key, char *out, size_t outsz)
{
    size_t klen = strlen(key);
    const char *p = data;
    while (*p) {
        if (strncmp(p, key, klen) == 0 && p[klen] == '=') {
            const char *v = p + klen + 1;
            size_t len = strcspn(v, "&");
            char raw[512];
            if (len >= sizeof raw) len = sizeof raw - 1;
            memcpy(raw, v, len);
            raw[len] = '\0';
            url_decode(out, raw, outsz);
            return 1;
        }
        p += strcspn(p, "&");
        if (*p == '&') p++;
    }
    return 0;
}

int get_cookie(const Request *req, const char *key, char *out, size_t outsz)
{
    size_t klen = strlen(key);
    const char *p = req->cookie;
    while (*p) {
        while (*p == ' ' || *p == ';') p++;
        if (strncmp(p, key, klen) == 0 && p[klen] == '=') {
            const char *v = p + klen + 1;
            size_t len = strcspn(v, ";");
            if (len >= outsz) len = outsz - 1;
            memcpy(out, v, len);
            out[len] = '\0';
            return 1;
        }
        p += strcspn(p, ";");
    }
    return 0;
}

/* ---------- sending ---------- */

static void send_all(sock_t c, const char *data, size_t len)
{
    size_t sent = 0;
    while (sent < len) {
        int n = send(c, data + sent, (int)(len - sent), 0);
        if (n <= 0) return;
        sent += (size_t)n;
    }
}

void send_response(sock_t c, int code, const char *content_type,
                   const char *extra_headers, const char *body, size_t len)
{
    char head[512];
    const char *msg = code == 200 ? "OK" : code == 302 ? "Found"
                    : code == 404 ? "Not Found" : "Error";
    int n = snprintf(head, sizeof head,
        "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
        "Cache-Control: no-store\r\nConnection: close\r\n%s\r\n",
        code, msg, content_type, len, extra_headers ? extra_headers : "");
    send_all(c, head, (size_t)n);
    if (len > 0) send_all(c, body, len);
}

void send_redirect(sock_t c, const char *location, const char *set_cookie)
{
    char extra[400];
    if (set_cookie)
        snprintf(extra, sizeof extra, "Location: %s\r\nSet-Cookie: %s\r\n", location, set_cookie);
    else
        snprintf(extra, sizeof extra, "Location: %s\r\n", location);
    send_response(c, 302, "text/plain", extra, "", 0);
}

/* ---------- reading a request ---------- */

static int find_header(const char *buf, const char *name, char *out, size_t outsz)
{
    size_t nlen = strlen(name);
    const char *line = strstr(buf, "\r\n");
    while (line) {
        line += 2;
        if (line[0] == '\r') break;              /* end of headers */
        if (strncasecmp(line, name, nlen) == 0 && line[nlen] == ':') {
            const char *v = line + nlen + 1;
            while (*v == ' ') v++;
            size_t len = strcspn(v, "\r\n");
            if (len >= outsz) len = outsz - 1;
            memcpy(out, v, len);
            out[len] = '\0';
            return 1;
        }
        line = strstr(line, "\r\n");
    }
    return 0;
}

static void handle_client(sock_t c)
{
    char buf[REQ_MAX];
    int len = 0;
    char tmp[32];
    Request req;
    memset(&req, 0, sizeof req);

    /* read until we have the headers and the whole body */
    while (len < REQ_MAX - 1) {
        int n = recv(c, buf + len, REQ_MAX - 1 - len, 0);
        if (n <= 0) break;
        len += n;
        buf[len] = '\0';
        char *hend = strstr(buf, "\r\n\r\n");
        if (hend) {
            int hlen = (int)(hend + 4 - buf);
            int cl = 0;
            if (find_header(buf, "Content-Length", tmp, sizeof tmp)) cl = atoi(tmp);
            if (len >= hlen + cl) break;
        }
    }
    if (len == 0) return;

    /* first line: METHOD /path?query HTTP/1.1 */
    char uri[300] = "";
    if (sscanf(buf, "%7s %299s", req.method, uri) != 2) return;
    char *q = strchr(uri, '?');
    if (q) {
        *q = '\0';
        snprintf(req.query, sizeof req.query, "%s", q + 1);
    }
    snprintf(req.path, sizeof req.path, "%.127s", uri);
    find_header(buf, "Cookie", req.cookie, sizeof req.cookie);
    find_header(buf, "Host", req.host, sizeof req.host);

    char *hend = strstr(buf, "\r\n\r\n");
    if (hend) snprintf(req.body, sizeof req.body, "%s", hend + 4);

    router_dispatch(c, &req);
}

/* ---------- main loop ---------- */

int server_run(int port)
{
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { fprintf(stderr, "WSAStartup failed\n"); return 1; }
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == SOCK_BAD) { perror("socket"); return 1; }

#ifndef _WIN32
    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof yes);
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((unsigned short)port);

    if (bind(s, (struct sockaddr *)&addr, sizeof addr) < 0) { perror("bind"); return 1; }
    if (listen(s, 16) < 0) { perror("listen"); return 1; }

    printf("Canteen server running. Open http://localhost:%d in your browser.\n", port);
    printf("Other devices on the same Wi-Fi: use http://<this-computer-IP>:%d\n", port);

    for (;;) {
        sock_t c = accept(s, NULL, NULL);
        if (c == SOCK_BAD) continue;
        handle_client(c);          /* one request at a time: keeps file writes safe */
        closesocket(c);
    }
    return 0;
}
