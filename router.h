#ifndef ROUTER_H
#define ROUTER_H
#include "common.h"
void router_dispatch(sock_t client, const Request *req);
#endif
