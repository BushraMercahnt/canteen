#ifndef QUEUE_H
#define QUEUE_H
#include "common.h"
/* Token and queue logic. Owner: Partner A. Partner B calls these functions from the staff pages. */
void  queue_init(void);                                 /* load saved orders */
int   queue_place_order(const int qty[], int nitems);   /* returns the new token, 0 if nothing ordered */
const Order *queue_find(int token);                     /* NULL if there is no such token */
int   queue_count(void);
const Order *queue_at(int index);                       /* orders in token order, 0 .. count-1 */
int   queue_set_status(int token, Status s);            /* 1 = ok */
int   queue_orders_ahead(int token);                    /* orders before this one still waiting/preparing */
int   queue_now_serving(void);                          /* 0 = nobody yet */
int   queue_call_next(void);                            /* moves on to the next waiting token, returns it (0 = none) */
int   queue_set_rating(int token, int stars);           /* 1 = saved. Only for Ready/Collected orders, only once */
int   queue_waiting_count(void);                        /* orders that are Waiting or Preparing */
int   queue_item_sold(int menu_index);                  /* how many of this food were ordered (not cancelled) */
const char *status_text(Status s);
#endif
