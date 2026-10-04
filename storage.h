#ifndef STORAGE_H
#define STORAGE_H
#include "common.h"
/* Saves/loads the whole order list in orders.dat so nothing is lost on restart. Owner: Partner A */
int storage_load_orders(Order *orders, int max, int *count, int *next_token, int *now_serving);
int storage_save_orders(const Order *orders, int count, int next_token, int now_serving);
#endif
