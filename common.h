/* common.h - THE CONTRACT. Shared by Partner A and Partner B.
 * Change this file only after telling your partner. */
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #include <winsock2.h>
  typedef SOCKET sock_t;
#else
  #include <unistd.h>
  typedef int sock_t;
#endif
#define SOCK_BAD ((sock_t)-1)

#define MAX_MENU_ITEMS 20
#define MAX_ORDERS     1000
#define NAME_LEN       40
#define MINUTES_PER_ORDER 2   /* wait estimate = orders ahead x this */

typedef enum {
    ST_WAITING = 0,
    ST_PREPARING,
    ST_READY,
    ST_COLLECTED,
    ST_CANCELLED
} Status;

typedef struct {
    char name[NAME_LEN];
    int  price;       /* in rupees */
    int  available;   /* 1 = can be ordered, 0 = finished */
    int  veg;         /* 1 = vegetarian, 0 = non-vegetarian */
} MenuItem;

typedef struct {
    int    token;                    /* 1, 2, 3 ... unique per day */
    int    qty[MAX_MENU_ITEMS];      /* quantity per menu item index */
    int    total;                    /* total price in rupees */
    Status status;
    long   placed;                   /* time() when the order was placed */
    int    rating;                   /* 0 = not rated yet, 1 to 5 stars */
} Order;

/* One web request, filled in by server.c */
typedef struct {
    char method[8];     /* "GET" or "POST" */
    char path[128];     /* "/menu" */
    char query[256];    /* "id=28" (after the ?) */
    char body[2048];    /* form data of a POST, "q0=1&q1=0" */
    char cookie[256];   /* raw Cookie header */
    char host[128];     /* Host header, e.g. "localhost:8090" or "abc.trycloudflare.com" */
} Request;

/* A page handler: reads the request, writes a page to the client */
typedef void (*Handler)(sock_t client, const Request *req);

#endif
