/* queue.c - tokens and the waiting line. Owner: Partner A */
#include "queue.h"
#include "menu.h"
#include "storage.h"

static Order orders[MAX_ORDERS];
static int order_count = 0;
static int next_token = 1;
static int now_serving = 0;

static void save(void) { storage_save_orders(orders, order_count, next_token, now_serving); }

void queue_init(void)
{
    storage_load_orders(orders, MAX_ORDERS, &order_count, &next_token, &now_serving);
}

int queue_place_order(const int qty[], int nitems)
{
    if (order_count >= MAX_ORDERS) return 0;
    int total = 0, items = 0;
    for (int i = 0; i < nitems && i < MAX_MENU_ITEMS; i++) {
        const MenuItem *m = menu_get(i);
        if (m && m->available && qty[i] > 0) { total += qty[i] * m->price; items += qty[i]; }
    }
    if (items == 0) return 0;

    Order *o = &orders[order_count];
    memset(o, 0, sizeof *o);
    for (int i = 0; i < nitems && i < MAX_MENU_ITEMS; i++) {
        const MenuItem *m = menu_get(i);
        o->qty[i] = (m && m->available && qty[i] > 0) ? qty[i] : 0;
    }
    o->token = next_token++;
    o->total = total;
    o->status = ST_WAITING;
    o->placed = (long)time(NULL);
    order_count++;
    save();
    return o->token;
}

const Order *queue_find(int token)
{
    for (int i = 0; i < order_count; i++)
        if (orders[i].token == token) return &orders[i];
    return NULL;
}

int queue_count(void) { return order_count; }

const Order *queue_at(int index)
{
    if (index < 0 || index >= order_count) return NULL;
    return &orders[index];
}

int queue_set_status(int token, Status s)
{
    for (int i = 0; i < order_count; i++) {
        if (orders[i].token == token) { orders[i].status = s; save(); return 1; }
    }
    return 0;
}

int queue_orders_ahead(int token)
{
    int n = 0;
    for (int i = 0; i < order_count; i++)
        if (orders[i].token < token &&
            (orders[i].status == ST_WAITING || orders[i].status == ST_PREPARING)) n++;
    return n;
}

int queue_now_serving(void) { return now_serving; }

int queue_call_next(void)
{
    for (int i = 0; i < order_count; i++) {
        if (orders[i].status == ST_WAITING) {
            orders[i].status = ST_PREPARING;
            now_serving = orders[i].token;
            save();
            return now_serving;
        }
    }
    return 0;
}

int queue_set_rating(int token, int stars)
{
    if (stars < 1 || stars > 5) return 0;
    for (int i = 0; i < order_count; i++) {
        if (orders[i].token != token) continue;
        if (orders[i].status != ST_READY && orders[i].status != ST_COLLECTED) return 0;
        if (orders[i].rating != 0) return 0;          /* one rating per order */
        orders[i].rating = stars;
        save();
        return 1;
    }
    return 0;
}

int queue_waiting_count(void)
{
    int n = 0;
    for (int i = 0; i < order_count; i++)
        if (orders[i].status == ST_WAITING || orders[i].status == ST_PREPARING) n++;
    return n;
}

int queue_item_sold(int menu_index)
{
    if (menu_index < 0 || menu_index >= MAX_MENU_ITEMS) return 0;
    int n = 0;
    for (int i = 0; i < order_count; i++)
        if (orders[i].status != ST_CANCELLED) n += orders[i].qty[menu_index];
    return n;
}

const char *status_text(Status s)
{
    switch (s) {
        case ST_WAITING:   return "Waiting";
        case ST_PREPARING: return "Preparing";
        case ST_READY:     return "Ready";
        case ST_COLLECTED: return "Collected";
        case ST_CANCELLED: return "Cancelled";
    }
    return "Unknown";
}
