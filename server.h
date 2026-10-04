/* server.h - Partner B owns server.c */
#ifndef SERVER_H
#define SERVER_H
#include "common.h"

int  server_run(int port);   /* never returns unless the socket fails */

void send_response(sock_t c, int code, const char *content_type,
                   const char *extra_headers, const char *body, size_t len);
void send_redirect(sock_t c, const char *location, const char *set_cookie);

/* Reads key from "a=1&b=2" style data (query or POST body), URL-decoded.
 * Returns 1 if found, 0 if not. */
int  get_param(const char *data, const char *key, char *out, size_t outsz);
/* Reads one cookie value from the request. Returns 1 if found. */
int  get_cookie(const Request *req, const char *key, char *out, size_t outsz);
#endif
