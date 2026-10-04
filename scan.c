/* scan.c - the "Scan to order" page. Owner: Partner B.
 * It draws a QR code of the canteen address (http://<this-computer-IP>:<port>/home)
 * so a customer can open the website by pointing a phone camera at the screen.
 * The QR code is made here in plain C (no library): byte mode, error level L, versions 1 to 5. */
#include "scan.h"
#include "page.h"
#include "server.h"
#ifndef _WIN32
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
#endif

#define QR_MAX_VER  5
#define QR_MAX_SIZE (17 + 4 * QR_MAX_VER)

/* data and error-correction codewords for versions 1..5 at level L (always 1 block) */
static const int DATA_CW[QR_MAX_VER + 1] = { 0, 19, 34, 55, 80, 108 };
static const int EC_CW[QR_MAX_VER + 1]   = { 0,  7, 10, 15, 20,  26 };

static int g_port = 8080;
void scan_set_port(int port) { g_port = port; }

typedef struct {
    int size;
    unsigned char m[QR_MAX_SIZE][QR_MAX_SIZE];    /* m[y][x] = 1 for a dark square */
    unsigned char fn[QR_MAX_SIZE][QR_MAX_SIZE];   /* 1 = fixed pattern, not data */
} Qr;

/* ---------- error correction (Reed-Solomon over GF(256)) ---------- */
static int gf_mul(int a, int b)
{
    int r = 0;
    while (b) {
        if (b & 1) r ^= a;
        a <<= 1;
        if (a & 0x100) a ^= 0x11D;
        b >>= 1;
    }
    return r;
}

static void rs_ecc(const unsigned char *data, int dlen, unsigned char *ecc, int elen)
{
    unsigned char gen[32];
    memset(gen, 0, sizeof gen);
    gen[elen - 1] = 1;
    int root = 1;
    for (int i = 0; i < elen; i++) {
        for (int j = 0; j < elen; j++) {
            gen[j] = (unsigned char)gf_mul(gen[j], root);
            if (j + 1 < elen) gen[j] ^= gen[j + 1];
        }
        root = gf_mul(root, 2);
    }
    memset(ecc, 0, (size_t)elen);
    for (int i = 0; i < dlen; i++) {
        int factor = data[i] ^ ecc[0];
        memmove(ecc, ecc + 1, (size_t)elen - 1);
        ecc[elen - 1] = 0;
        for (int j = 0; j < elen; j++) ecc[j] ^= (unsigned char)gf_mul(gen[j], factor);
    }
}

/* ---------- drawing the fixed parts ---------- */
static void set_fn(Qr *q, int x, int y, int dark)
{
    if (x < 0 || y < 0 || x >= q->size || y >= q->size) return;
    q->m[y][x] = (unsigned char)dark;
    q->fn[y][x] = 1;
}

static void draw_finder(Qr *q, int cx, int cy)
{
    for (int dy = -4; dy <= 4; dy++)
        for (int dx = -4; dx <= 4; dx++) {
            int d = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
            set_fn(q, cx + dx, cy + dy, d != 2 && d != 4);
        }
}

static void draw_alignment(Qr *q, int cx, int cy)
{
    for (int dy = -2; dy <= 2; dy++)
        for (int dx = -2; dx <= 2; dx++) {
            int d = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
            set_fn(q, cx + dx, cy + dy, d != 1);
        }
}

/* format bits = error level L (01) + mask number, protected by a BCH code */
static void draw_format(Qr *q, int mask)
{
    int data = (1 << 3) | mask;
    int rem = data;
    for (int i = 0; i < 10; i++) rem = (rem << 1) ^ ((rem >> 9) * 0x537);
    int bits = ((data << 10) | rem) ^ 0x5412;
    int s = q->size;
    for (int i = 0; i <= 5; i++)  set_fn(q, 8, i, (bits >> i) & 1);
    set_fn(q, 8, 7, (bits >> 6) & 1);
    set_fn(q, 8, 8, (bits >> 7) & 1);
    set_fn(q, 7, 8, (bits >> 8) & 1);
    for (int i = 9; i < 15; i++)  set_fn(q, 14 - i, 8, (bits >> i) & 1);
    for (int i = 0; i < 8; i++)   set_fn(q, s - 1 - i, 8, (bits >> i) & 1);
    for (int i = 8; i < 15; i++)  set_fn(q, 8, s - 15 + i, (bits >> i) & 1);
    set_fn(q, 8, s - 8, 1);                      /* the always-dark square */
}

static int mask_hit(int mask, int x, int y)
{
    switch (mask) {
        case 0: return (x + y) % 2 == 0;
        case 1: return y % 2 == 0;
        case 2: return x % 3 == 0;
        case 3: return (x + y) % 3 == 0;
        case 4: return (x / 3 + y / 2) % 2 == 0;
        case 5: return x * y % 2 + x * y % 3 == 0;
        case 6: return (x * y % 2 + x * y % 3) % 2 == 0;
        default: return ((x + y) % 2 + x * y % 3) % 2 == 0;
    }
}

static void apply_mask(Qr *q, int mask)          /* flips the squares; calling it twice undoes it */
{
    for (int y = 0; y < q->size; y++)
        for (int x = 0; x < q->size; x++)
            if (!q->fn[y][x] && mask_hit(mask, x, y)) q->m[y][x] ^= 1;
}

/* how ugly is this mask? (lower is better; the rules come from the QR standard) */
static int penalty(const Qr *q)
{
    int s = q->size, score = 0, dark = 0;
    for (int y = 0; y < s; y++)
        for (int x = 0; x < s; x++) dark += q->m[y][x];

    for (int pass = 0; pass < 2; pass++) {               /* rule 1: long runs, rule 3: finder look-alikes */
        for (int a = 0; a < s; a++) {
            int run = 1;
            unsigned char hist[QR_MAX_SIZE];
            for (int b = 0; b < s; b++) {
                int c = pass ? q->m[b][a] : q->m[a][b];
                hist[b] = (unsigned char)c;
                if (b > 0) {
                    int prev = hist[b - 1];
                    if (c == prev) { run++; if (run == 5) score += 3; else if (run > 5) score++; }
                    else run = 1;
                }
            }
            for (int b = 0; b + 11 <= s; b++) {
                static const unsigned char p1[11] = {1,0,1,1,1,0,1,0,0,0,0};
                static const unsigned char p2[11] = {0,0,0,0,1,0,1,1,1,0,1};
                int m1 = 1, m2 = 1;
                for (int k = 0; k < 11; k++) {
                    if (hist[b + k] != p1[k]) m1 = 0;
                    if (hist[b + k] != p2[k]) m2 = 0;
                }
                score += 40 * (m1 + m2);
            }
        }
    }
    for (int y = 0; y + 1 < s; y++)                      /* rule 2: 2x2 blocks */
        for (int x = 0; x + 1 < s; x++) {
            int c = q->m[y][x];
            if (c == q->m[y][x + 1] && c == q->m[y + 1][x] && c == q->m[y + 1][x + 1]) score += 3;
        }
    int total = s * s;                                   /* rule 4: dark / light balance */
    int k = (abs(dark * 20 - total * 10) + total - 1) / total - 1;
    if (k > 0) score += k * 10;
    return score;
}

/* ---------- text -> QR code. Returns 1 on success, 0 if the text is too long. ---------- */
static int qr_encode(const char *text, Qr *q)
{
    int len = (int)strlen(text), ver = 0;
    for (int v = 1; v <= QR_MAX_VER; v++)
        if (len <= DATA_CW[v] - 2) { ver = v; break; }
    if (!ver) return 0;

    unsigned char cw[160];
    memset(cw, 0, sizeof cw);
    int nbits = 0, cap = DATA_CW[ver] * 8;
    #define PUT(val, count) do { \
        for (int _i = (count) - 1; _i >= 0; _i--) { \
            if (((val) >> _i) & 1) { cw[nbits >> 3] |= (unsigned char)(0x80 >> (nbits & 7)); } \
            nbits++; \
        } \
    } while (0)
    PUT(4, 4);                                           /* mode: bytes */
    PUT(len, 8);
    for (int i = 0; i < len; i++) PUT((unsigned char)text[i], 8);
    int term = cap - nbits < 4 ? cap - nbits : 4;
    PUT(0, term);
    while (nbits % 8) PUT(0, 1);
    for (int pad = 0xEC; nbits < cap; pad ^= 0xEC ^ 0x11) PUT(pad, 8);
    #undef PUT
    rs_ecc(cw, DATA_CW[ver], cw + DATA_CW[ver], EC_CW[ver]);

    memset(q, 0, sizeof *q);
    q->size = 17 + 4 * ver;
    int s = q->size;
    for (int i = 0; i < s; i++) { set_fn(q, 6, i, i % 2 == 0); set_fn(q, i, 6, i % 2 == 0); }
    draw_finder(q, 3, 3); draw_finder(q, s - 4, 3); draw_finder(q, 3, s - 4);
    if (ver >= 2) draw_alignment(q, s - 7, s - 7);
    draw_format(q, 0);                                   /* reserves the format squares */

    int total_bits = (DATA_CW[ver] + EC_CW[ver]) * 8, i = 0;
    for (int right = s - 1; right >= 1; right -= 2) {    /* zig-zag over the data squares */
        if (right == 6) right = 5;
        for (int vert = 0; vert < s; vert++)
            for (int j = 0; j < 2; j++) {
                int x = right - j;
                int upward = ((right + 1) & 2) == 0;
                int y = upward ? s - 1 - vert : vert;
                if (!q->fn[y][x] && i < total_bits) {
                    q->m[y][x] = (cw[i >> 3] >> (7 - (i & 7))) & 1;
                    i++;
                }
            }
    }

    int best = 0, best_score = 1 << 30;
    for (int mask = 0; mask < 8; mask++) {
        apply_mask(q, mask);
        draw_format(q, mask);
        int sc = penalty(q);
        if (sc < best_score) { best_score = sc; best = mask; }
        apply_mask(q, mask);
    }
    apply_mask(q, best);
    draw_format(q, best);
    return 1;
}

/* draws the QR code as a small picture inside the page */
static void qr_svg(Page *p, const Qr *q)
{
    int quiet = 4, n = q->size + 2 * quiet;
    pg_add(p, "<svg viewBox=\"0 0 %d %d\" role=\"img\" aria-label=\"QR code to open the canteen\" "
              "shape-rendering=\"crispEdges\" style=\"width:100%%;max-width:320px;border-radius:.5em\">", n, n);
    pg_add(p, "<rect width=\"%d\" height=\"%d\" fill=\"#fff\"/><path fill=\"#000\" d=\"", n, n);
    for (int y = 0; y < q->size; y++) {
        int x = 0;
        while (x < q->size) {
            if (q->m[y][x]) {
                int start = x;
                while (x < q->size && q->m[y][x]) x++;
                pg_add(p, "M%d %dh%dv1h-%dz", start + quiet, y + quiet, x - start, x - start);
            } else x++;
        }
    }
    pg_puts(p, "\"/></svg>");
}

/* the address other phones should use: this computer's Wi-Fi address */
static void lan_ip(char *out, size_t n)
{
    snprintf(out, n, "localhost");
    sock_t s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == SOCK_BAD) return;
    struct sockaddr_in to, me;
    memset(&to, 0, sizeof to);
    to.sin_family = AF_INET;
    to.sin_port = htons(53);
    to.sin_addr.s_addr = htonl(0x08080808);              /* nothing is sent: this only picks the network card */
    if (connect(s, (struct sockaddr *)&to, sizeof to) == 0) {
#ifdef _WIN32
        int ml = (int)sizeof me;
#else
        socklen_t ml = sizeof me;
#endif
        if (getsockname(s, (struct sockaddr *)&me, &ml) == 0 && me.sin_addr.s_addr != 0)
            snprintf(out, n, "%s", inet_ntoa(me.sin_addr));
    }
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}

/* ---------- GET /scan ---------- */

/* 1 if the page was opened through a public web name (like abc.trycloudflare.com),
 * 0 if it was opened as localhost or with a number address like 192.168.0.5 */
static int is_public_host(const char *h)
{
    if (!h[0]) return 0;
    if (strncmp(h, "localhost", 9) == 0) return 0;
    if (h[0] >= '0' && h[0] <= '9') return 0;
    return 1;
}

void scan_page(sock_t client, const Request *req)
{
    char url[160];
    int is_public = is_public_host(req->host);
    if (is_public) {
        /* a public link works from anywhere, also on mobile data */
        snprintf(url, sizeof url, "https://%s/home", req->host);
    } else {
        char ip[64];
        lan_ip(ip, sizeof ip);
        snprintf(url, sizeof url, "http://%s:%d/home", ip, g_port);
    }

    static Qr q;
    int ok = qr_encode(url, &q);

    Page p;
    page_begin(&p, req, "Scan to Order", 0, 1);
    pg_puts(&p, "<h1>Scan. Chill. Eat. Repeat.</h1>");
    if (ok) {
        pg_puts(&p, "<div style=\"display:flex;justify-content:center;margin:.6em 0\">");
        qr_svg(&p, &q);
        pg_puts(&p, "</div>");
    }
    if (is_public)
        pg_puts(&p, "<p>Point your phone camera at the square to open the canteen. "
                    "It works from anywhere, also on mobile data.</p>");
    else
        pg_puts(&p, "<p>Point your phone camera at the square to open the canteen. "
                    "Your phone must be on the same Wi-Fi as this computer. "
                    "To make a code that works from anywhere, open this page through your public link "
                    "(the link + /scan).</p>");
    pg_add(&p, "<div class=\"card\"><b>Or type this address:</b><br>%s</div>", url);
    pg_puts(&p, "<p><a class=\"btn pri\" href=\"/menu\">Order Food</a></p>");
    page_end(&p);
    page_send(client, &p);
}
