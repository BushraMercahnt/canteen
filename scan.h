/* scan.h - the "Scan to order" page (QR code). Owner: Partner B */
#ifndef SCAN_H
#define SCAN_H
#include "common.h"

void scan_set_port(int port);                         /* main.c tells us which port the server uses */
void scan_page(sock_t client, const Request *req);    /* GET /scan */
#endif
