#ifndef PAGES_CUSTOMER_H
#define PAGES_CUSTOMER_H
#include "common.h"
/* Customer pages. Owner: Partner A */
void cust_cover_image(sock_t client, const Request *req);   /* DONE  GET  /cover.jpg */
void cust_cover_page(sock_t client, const Request *req);    /* DONE  GET  /        */
void cust_home_page(sock_t client, const Request *req);     /* DONE  GET  /home    */
void cust_menu_page(sock_t client, const Request *req);     /* DONE  GET  /menu    */
void cust_order_submit(sock_t client, const Request *req);  /* DONE  POST /order   */
void cust_token_page(sock_t client, const Request *req);    /* DONE  GET  /token?id=N */
void cust_check_page(sock_t client, const Request *req);    /* DONE  GET  /check   */
void cust_size_set(sock_t client, const Request *req);      /* DONE  GET  /size?set=big&back=/ */
void cust_queue_page(sock_t client, const Request *req);    /* TODO  GET  /queue   */
void cust_rate_submit(sock_t client, const Request *req);   /* TODO  POST /rate    */
#endif
