#include "report.h"
#include "queue.h"

/* Adds up all orders that were not cancelled */
void report_totals(int *orders_count, int *sales_total)
{
    *orders_count = 0;
    *sales_total = 0;
    for (int i = 0; i < queue_count(); i++) {
        const Order *o = queue_at(i);
        if (o->status == ST_CANCELLED) continue;
        (*orders_count)++;
        *sales_total += o->total;
    }
}

/* Counts the rated orders and works out the average number of stars */
void report_ratings(int *rated_count, double *average)
{
    int sum = 0;
    *rated_count = 0;
    *average = 0.0;
    for (int i = 0; i < queue_count(); i++) {
        const Order *o = queue_at(i);
        if (o->rating > 0) {
            (*rated_count)++;
            sum += o->rating;
        }
    }
    if (*rated_count > 0)
        *average = (double)sum / *rated_count;     /* (double) so we do not lose the decimals */
}
