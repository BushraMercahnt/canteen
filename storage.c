#include "storage.h"

#define ORDERS_FILE "orders.dat"

/* The magic number changes whenever the Order struct changes, so an old orders.dat is ignored */
#define ORDERS_MAGIC 20261001
typedef struct { int magic, count, next_token, now_serving; } Header;

int storage_load_orders(Order *orders, int max, int *count, int *next_token, int *now_serving)
{
    *count = 0; *next_token = 1; *now_serving = 0;
    FILE *f = fopen(ORDERS_FILE, "rb");
    if (!f) return 0;                       /* first run: nothing saved yet */
    Header h;
    if (fread(&h, sizeof h, 1, f) != 1 || h.magic != ORDERS_MAGIC || h.count < 0 || h.count > max) { fclose(f); return 0; }
    if ((int)fread(orders, sizeof(Order), (size_t)h.count, f) != h.count) { fclose(f); return 0; }
    fclose(f);
    *count = h.count; *next_token = h.next_token; *now_serving = h.now_serving;
    return 1;
}

int storage_save_orders(const Order *orders, int count, int next_token, int now_serving)
{
    FILE *f = fopen(ORDERS_FILE, "wb");
    if (!f) return 0;
    Header h = { ORDERS_MAGIC, count, next_token, now_serving };
    fwrite(&h, sizeof h, 1, f);
    fwrite(orders, sizeof(Order), (size_t)count, f);
    fclose(f);
    return 1;
}
